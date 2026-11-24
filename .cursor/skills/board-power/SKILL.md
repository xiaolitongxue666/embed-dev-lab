---
name: board-power
description: 烧录或看串口之前确认蓝板供电、ST-Link、面包板共地和 SWD 接线时使用。MicroUSB、PA13、PA14、CH341 GND。
---

# 上电、共地与 SWD

烧录（[firmware-flash](../firmware-flash/SKILL.md)）或看日志（[serial-console](../serial-console/SKILL.md)）之前核对台架。不要改接线文档去迁就另一种拓扑。

## 核对

1. 蓝板用自己的 MicroUSB 给 MCU 供电。烧录前这一路必须已经插上。
2. ST-Link 只做 SWD。SWDIO→PA13，SWCLK→PA14。NRST 可选。
3. ST-Link 3.3V 接到面包板外设轨。方案 A：标称 5V 的输出闲置。只有器件明确要 5V 才使用 5V 轨。
4. ST-Link 的 3.3V 和 5V 都不接到蓝板的电源脚。不要两套电源并联。
5. 面包板 GND 轨是唯一汇集点。ST-Link GND、CH341 GND、蓝板 SWD GND、外设 GND 各自接到这根轨。不要把地线串成一条链。
6. CH341 GND 直接进面包板 GND 轨，不经蓝板串过去。CH341 不给 MCU 或外设供电。
7. 本仓库模块用面包板 3.3V，不接 VIN/5V。
8. GPIO 不当电源。PA13、PA14 不当 GPIO，也不当串口。

## 负载

ST-Link 3.3V 大约 150 mA，5V 大约 300 mA。超了就给外设单独供电，只把 GND 并到面包板轨。

## 失败时回到这里

| 现象 | 含义 |
|------|------|
| `JtagGetIdcodeError` | MCU 没电，或 SWD 没接上 |
| `probe-rs list` 为空 | 先按烧写 Skill 区分：`VID_0000&PID_0002` 描述符失败要重启该设备，不要对它装 WinUSB。已经是 `0483:3748` 仍打不开才查驱动或线。这不是串口问题 |
| 串口 `bytes=0`，烧录却成功 | 串口与 SWD 是分开的。先按串口 Skill 看 USART 是否已经 TC。TC 已置位就不要改接线 |

文档与注释里不要出现这些说法：核心板 USB 给外设供电、ST-Link 3.3V 悬空、ST-Link 3.3V 接到蓝板、ST-Link GND 经蓝板再串到面包板。

详解：[`doc/hardware/power-and-common-ground.md`](../../../doc/hardware/power-and-common-ground.md)。规则：[`.agent/rules/hardware.md`](../../rules/hardware.md)。
