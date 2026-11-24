# Hooks

本仓库没有安装会在会话开始、工具调用或提交时自动执行的 Agent Hook，也没有为这套配置新增 Git hook。

生命周期若以后要接，对应关系是：

| 时机 | 现在靠什么 |
|------|------------|
| 会话开始 | 宿主自己加载 `AGENTS.md`、`.cursor/rules` 或 `CLAUDE.md` |
| 烧录前 | [firmware-flash](../skills/firmware-flash/SKILL.md) 里的步骤，不是拦截器 |
| 提交前 | 用户另行要求时才提交。本目录不挂 pre-commit |

不要把 [`.agent/policies/guardrails.md`](../policies/guardrails.md) 当成已经生效的 Hook。
