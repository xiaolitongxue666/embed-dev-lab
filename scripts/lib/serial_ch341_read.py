#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# Windows: open CH341 COM (VID:PID 1A86:5523), read UART for N seconds.
# Baud and COM are arguments — do not assume fixed values across sessions.
# Invoked by scripts/serial-ch341-read.sh (CLI log in English).
# -----------------------------------------------------------------------------
from __future__ import annotations

import argparse
import sys
import time

CH341_VID = 0x1A86
CH341_PID = 0x5523


def find_ch341_port() -> str | None:
    import serial.tools.list_ports

    for port in serial.tools.list_ports.comports():
        if port.vid == CH341_VID and port.pid == CH341_PID:
            return port.device
        desc = (port.description or "").upper()
        hwid = (port.hwid or "").upper()
        if "CH341" in desc or "VID:PID=1A86:5523" in hwid:
            return port.device
    return None


def list_ports() -> None:
    import serial.tools.list_ports

    for port in serial.tools.list_ports.comports():
        mark = ""
        if port.vid == CH341_VID and port.pid == CH341_PID:
            mark = "  <-- CH341"
        print(f"{port.device}\t{port.description}\t{port.hwid}{mark}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Read CH341 UART on Windows")
    parser.add_argument("--port", default="", help="COMx (default: auto-detect CH341)")
    parser.add_argument(
        "--baud",
        type=int,
        default=1500000,
        help="baud matching firmware (default 1500000; override when firmware changes)",
    )
    parser.add_argument("--seconds", type=float, default=5.0, help="capture duration")
    parser.add_argument("--list", action="store_true", help="list COM ports and exit")
    args = parser.parse_args()

    try:
        import serial  # noqa: F401
        import serial.tools.list_ports  # noqa: F401
    except ImportError:
        print(
            "ERROR: pyserial required. Try: uv run --with pyserial python ... "
            "or: python -m pip install pyserial",
            file=sys.stderr,
        )
        return 2

    if args.list:
        list_ports()
        return 0

    port = args.port.strip() or (find_ch341_port() or "")
    if not port:
        print(
            "ERROR: CH341 COM not found (VID:PID 1A86:5523). "
            "Run: ./scripts/serial-ch341-switch.sh to-win",
            file=sys.stderr,
        )
        list_ports()
        return 1

    print(f"INFO: port={port} baud={args.baud} seconds={args.seconds}", flush=True)
    ser = serial.Serial(port, args.baud, timeout=0.2)
    buf = bytearray()
    try:
        time.sleep(0.05)
        ser.reset_input_buffer()
        t0 = time.time()
        while time.time() - t0 < args.seconds:
            chunk = ser.read(4096)
            if chunk:
                buf.extend(chunk)
                sys.stdout.buffer.write(chunk)
                sys.stdout.buffer.flush()
    finally:
        ser.close()

    has_crlf = b"\r\n" in buf
    print(file=sys.stderr)
    print(f"INFO: bytes={len(buf)} has_CRLF={has_crlf}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
