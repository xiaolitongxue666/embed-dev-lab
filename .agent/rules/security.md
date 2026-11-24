# 安全约束

这些约束写在规则里。真正拒绝 `both flash` 的是 `scripts/build.sh`。全片擦除没有自动拦截，Agent 必须先得到用户确认。

- 禁止未经用户确认的全片 Flash 擦除或 mass erase（含 embedded-debugger MCP）
- 烧录前确认已 `build` 出最新 ELF，且蓝板 MicroUSB 已供电
- PA13 / PA14 只做 SWD，禁止当 GPIO 或串口
- 不要在 Windows COM 与 WSL 两端同时占用同一 CH341
- Agent 读串口用仓库脚本，不要驱动 SecureCRT 等 GUI
- 寄存器读写须注明来源（RM0008 topic MD 或 PDF 页码），不要臆造位域
- 不要把密钥、cookie 写进仓库或 MCP 配置

执行层说明： [`.agent/policies/guardrails.md`](../policies/guardrails.md)。
