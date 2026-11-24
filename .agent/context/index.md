# 上下文：这次任务读什么

Context 是这一次任务要看到的材料。Memory 是跨任务保存的事实，只在下表点名时打开。

| 任务 | 打开 |
|------|------|
| 还没判断该做哪一步 | [`.agent/skills/embed-dev-lab/SKILL.md`](../skills/embed-dev-lab/SKILL.md)、[`AGENTS.md`](../../AGENTS.md) |
| 改源码 | 对应工程规则 [`.agent/rules/f103-manual-reg.md`](../rules/f103-manual-reg.md) 或 [`.agent/rules/f103-cmsis-hal.md`](../rules/f103-cmsis-hal.md)，加上 [`coding.md`](../rules/coding.md)。两边行为要一起改时再看 [`architecture.md`](../rules/architecture.md) |
| 编译 | [firmware-build](../skills/firmware-build/SKILL.md) |
| 烧录 | [board-power](../skills/board-power/SKILL.md)、[firmware-build](../skills/firmware-build/SKILL.md)、[firmware-flash](../skills/firmware-flash/SKILL.md) |
| 只看日志 | [serial-console](../skills/serial-console/SKILL.md)。乱码或没输出时再看 [`.agent/memory/failures.md`](../memory/failures.md) |
| 接线、供电、探针 ID 失败 | [board-power](../skills/board-power/SKILL.md)、[`.agent/rules/hardware.md`](../rules/hardware.md) |
| 查以前为什么这样定 | [`.agent/memory/decisions.md`](../memory/decisions.md)。台架现状看 [`progress.md`](../memory/progress.md) |

不要把 `doc/` 整目录读进上下文。Skill 里的命令够完成编译、烧写和串口时，长文只在对不上现象时再打开对应一篇。
