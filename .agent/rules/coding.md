# 编码

- `src/`、`startup/`、`linker/` 注释用中文
- `third_party/**` 保持 vendor 英文
- 终端、CLI、日志输出用英文
- 只改注释时，不得改动代码逻辑
- `fetch-f103-cmsis-hal-deps.sh` 拷贝后由 `scripts/lib/apply-f103-cmsis-hal-comments.sh` 恢复模板中文注释
- Windows 上跑仓库脚本用 Git Bash

手写寄存器工程不要 `#include` 或链接官方 CMSIS Device / HAL。CMSIS+HAL 工程保持 Cube 分层，不要为本 demo 加 `printf` / `syscalls.c`。细节见两条路线的规则。
