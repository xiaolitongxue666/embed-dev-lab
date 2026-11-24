# Agent 分工

没有 Super Agent，也没有自动路由器。谁打开仓库，谁就读同一套 [`.agent/`](../README.md) 和 [`AGENTS.md`](../../AGENTS.md)。

| 宿主 | 怎么读到项目知识 | 适合 |
|------|------------------|------|
| Cursor | `.cursor/rules/*.mdc` 始终注入；Skill 在 `.cursor/skills/`；也读 `AGENTS.md` | 改代码、编译、烧写、读串口 |
| Claude Code | 仓库根 `CLAUDE.md` 指向 `AGENTS.md` | 与 Cursor 相同的仓库任务 |
| CodeWhale / DeepSeek（DSH） | `scripts/lib/agent-detect.sh` 把 `deepseek` 当成 `codewhale`。`--global` 写入该宿主的 skills 目录。仓库知识仍以 `AGENTS.md` 与 `.agent/` 为准 | 方案和审查。操作步骤用同一批 Skill，不要另写一套命令 |
| Codex | 无 MCP。读 `AGENTS.md` 与 `.agent/skills/` | 不能靠 embedded-debugger 烧录或读寄存器 |
| OpenCode | 读仓库根 `AGENTS.md`。本次不新增 `opencode.json` | 按 `AGENTS.md` 里的 Skill 路径做独立任务 |

安装脚本只负责把 `.agent/skills/` 登记到 Cursor，以及 `--global` 时登记到 Claude Code / CodeWhale。它不探测 OpenCode，也不改 `agent-detect.sh` 的目录表。

并行时各自改不同文件。同一份固件的烧录不要两边同时 `flash`。串口不要两边同时打开。
