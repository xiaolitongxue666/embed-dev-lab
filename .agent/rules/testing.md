# 验证

本仓库验收固件的方式是编译和串口日志，不是另写一套主机测试。

1. 编译：`./scripts/build.sh <module> build`，或回归 `./scripts/build.sh both build`
2. 烧录后按串口 Skill 看日志

命令、顺序和失败分支在 Skill 里：

- [`.agent/skills/firmware-build/SKILL.md`](../skills/firmware-build/SKILL.md)
- [`.agent/skills/firmware-flash/SKILL.md`](../skills/firmware-flash/SKILL.md)
- [`.agent/skills/serial-console/SKILL.md`](../skills/serial-console/SKILL.md)

怎样算通过： [`.agent/evaluators/firmware.md`](../evaluators/firmware.md)。
