# 安全边界

规则告诉 Agent 应当遵守什么。下面这些条目里，只有脚本已经实现的拒绝才算执行层拦截。其余靠确认，不靠本文件自动挡住。

| 场景 | 现在怎么控制 |
|------|----------------|
| `both flash` / `build-flash.sh both` | `scripts/build.sh` 拒绝。一块板只能驻留一份固件 |
| 全片擦除 / mass erase | 没有自动拦截。CLI 和 embedded-debugger MCP 都必须先得到用户确认 |
| 改代码后直接 `flash` | 脚本仍会烧旧 ELF。Skill 要求先编译。脚本不会代替你编译 |
| Windows 与 WSL 同时占用 CH341 | 没有锁。Skill 要求先 `status`，只切到正在读的一侧 |
| PA13/PA14 当 GPIO 或串口 | 没有编译期禁令。规则禁止 |
| ST-Link 电源并到蓝板 | 没有硬件互锁。接线以 [`../rules/hardware.md`](../rules/hardware.md) 为准 |
| Git 推送、改生产环境 | 本仓库的 Agent 配置不授权。用户没有要求时不要推送 |

不要把「文档里写了不要」当成已经有 Hook。见 [`../hooks/README.md`](../hooks/README.md)。
