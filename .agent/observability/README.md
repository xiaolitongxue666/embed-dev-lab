# 可观测性

失败时先看这些已经存在的输出。本配置不记录 Token、模型名或跨 Agent 的 Trace ID。

| 来源 | 看什么 |
|------|--------|
| `./scripts/build.sh` | 英文编译器与链接器报错。退出码非 0 即未通过 |
| `probe-rs` | download / reset / `list` 的英文错误。`JtagGetIdcodeError`、列表为空 |
| `serial-ch341-read.sh` 或 picocom | 固件 `LOG_*` 行。形态见 [evaluators](../evaluators/firmware.md) |
| embedded-debugger MCP | 与 probe-rs 相同的探针和烧录结果 |

一次改动涉及的文件以版本库 diff 为准，不在这里另写一份清单。
