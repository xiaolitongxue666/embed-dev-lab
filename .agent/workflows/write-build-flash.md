# 编写 → 编译 → 烧写 → 串口

步骤细节在 Skill 里。本文件只规定顺序和回路。人类长文：[`doc/workflow-write-build-flash.md`](../../doc/workflow-write-build-flash.md)。

```text
board-power → firmware-build → firmware-flash → serial-console
```

1. [board-power](../skills/board-power/SKILL.md)：蓝板 MicroUSB 已供电，SWD 与共地符合规则。
2. [firmware-build](../skills/firmware-build/SKILL.md)：改过的那条路线先编译。demo 行为两边都改时，再 `./scripts/build.sh both build`。用户明确只改一边时不要改另一边。
3. [firmware-flash](../skills/firmware-flash/SKILL.md)：只烧当前要跑的那一个模块。
4. [serial-console](../skills/serial-console/SKILL.md)：需要看日志时再读串口。要接住启动行用 `./scripts/serial-ch341-read.sh --reset`。`bytes=0` 不算日志正常。

## 回路

| 结果 | 回到 |
|------|------|
| 编译失败 | firmware-build。不要烧录 |
| `JtagGetIdcode`，或探针列表为空 | board-power（未供电或 ST-Link）。列表为空时先按烧写 Skill 区分描述符失败和 WinUSB |
| 串口乱码或没有输出 | serial-console。`bytes=0` 且 SWD 上 USART 已报 TC 时，不要改波特，也不要改接线文档 |
| 烧进去仍是旧行为 | firmware-build：`flash` 不会编译 |

通过条件见 [`.agent/evaluators/firmware.md`](../evaluators/firmware.md)。
