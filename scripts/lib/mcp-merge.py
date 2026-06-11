#!/usr/bin/env python3
"""合并 MCP server 配置到各 Agent 的 JSON 文件（幂等、不覆盖已有其他 server）。"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any


def load_json(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {}
    try:
        with path.open(encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, dict) else {}
    except json.JSONDecodeError:
        return {}


def save_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")


def merge_servers(
    target: Path,
    servers: dict[str, dict[str, Any]],
    *,
    claude_settings: bool = False,
    force: bool = False,
) -> list[str]:
    data = load_json(target)
    changed: list[str] = []

    if claude_settings:
        existing = data.get("mcpServers")
        if not isinstance(existing, dict):
            existing = {}
        for name, entry in servers.items():
            if name in existing and not force:
                continue
            existing[name] = entry
            changed.append(name)
        data["mcpServers"] = existing
    else:
        existing = data.get("mcpServers")
        if not isinstance(existing, dict):
            existing = {}
        for name, entry in servers.items():
            if name in existing and not force:
                continue
            existing[name] = entry
            changed.append(name)
        data["mcpServers"] = existing

    if changed:
        save_json(target, data)
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description="Merge MCP servers into JSON config")
    parser.add_argument("--target", required=True, help="Path to mcp.json or settings.json")
    parser.add_argument(
        "--servers-json",
        required=True,
        help="JSON object of server name -> config",
    )
    parser.add_argument(
        "--claude-settings",
        action="store_true",
        help="Target is Claude Code settings.json (mcpServers key)",
    )
    parser.add_argument("--force", action="store_true", help="Overwrite existing servers")
    args = parser.parse_args()

    target = Path(args.target)
    try:
        servers = json.loads(args.servers_json)
    except json.JSONDecodeError as exc:
        print(f"Invalid servers JSON: {exc}", file=sys.stderr)
        return 1

    if not isinstance(servers, dict):
        print("servers-json must be a JSON object", file=sys.stderr)
        return 1

    changed = merge_servers(
        target,
        servers,
        claude_settings=args.claude_settings,
        force=args.force,
    )
    if changed:
        print("merged:" + ",".join(changed))
    else:
        print("unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main())
