# Agent Harness（embed-dev-lab）

本目录是 Cursor、Claude Code、CodeWhale / DeepSeek、Codex、OpenCode 共用的项目知识。各宿主仍用自己的入口加载它：仓库根 [`AGENTS.md`](../AGENTS.md)、[`CLAUDE.md`](../CLAUDE.md)、[`.cursor/rules/`](../.cursor/rules/)。

这里没有独立 Runtime，也不会自动调度多个 Agent。编译、烧写、串口仍由仓库脚本执行；Agent 按 Skill 里的步骤调用这些脚本。

## 知识

| 路径 | 作用 |
|------|------|
| [`rules/`](rules/) | 约束。Cursor 的 `.mdc` 仍整段注入，并指向这里的共用正文 |
| [`memory/`](memory/) | 已发生的事实与决定。入口索引仍是仓库根 [`PROJECT_MEMORY.md`](../PROJECT_MEMORY.md) |
| [`skills/`](skills/) | 嵌入式操作步骤：编译、烧写、串口、上电。正文含命令和失败分支 |

## 声明式模块

这些文件只说明「现在由谁承担」，不会在会话里自动触发。

| 路径 | 现在由谁承担 |
|------|----------------|
| [`context/index.md`](context/index.md) | 按任务列出该打开的 Skill 与规则 |
| [`workflows/write-build-flash.md`](workflows/write-build-flash.md) | 上电 → 编译 → 烧写 → 串口的顺序和回路 |
| [`tools/index.md`](tools/index.md) | `scripts/` 与 embedded-debugger MCP |
| [`policies/guardrails.md`](policies/guardrails.md) | 允许和禁止。脚本已拒绝的行为以脚本为准 |
| [`evaluators/firmware.md`](evaluators/firmware.md) | `build` 与串口日志是否符合现有 demo |
| [`orchestration/agents.md`](orchestration/agents.md) | 各 Agent 读什么、适合做什么 |
| [`hooks/README.md`](hooks/README.md) | 未安装会自动执行的 Hook |
| [`state/README.md`](state/README.md) | 不保存任务检查点 |
| [`observability/README.md`](observability/README.md) | 构建 CLI 与串口输出。不做 Token 记账 |

人类长文档仍在 [`doc/`](../doc/)。Skill 不搬运教程全文。
