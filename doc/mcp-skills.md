# MCP 与项目 Skill

embed-dev-lab 通过脚本安装 **embedded-debugger-mcp**（probe-rs 调试）与项目专属 Skill **embed-dev-lab**。

## 快速安装

```bash
./scripts/install-mcp-skills.sh
./scripts/install-mcp-skills.sh --verify-only
./scripts/install-mcp-skills.sh --global          # 合并到 ~/.cursor / ~/.claude / ~/.codewhale
./scripts/install-mcp-skills.sh --global --agents cursor,claude-code
./scripts/install-mcp-skills.sh --force           # 强制重新 cargo build
./scripts/install-mcp-skills.sh --no-proxy
```

bootstrap 可选一步（默认不安装，避免拖慢首次环境）：

```bash
./scripts/bootstrap.sh --with-mcp
```

## 前置条件

| 依赖 | 说明 |
|------|------|
| **Rust / cargo** | 构建 embedded-debugger-mcp（`rustup.rs`） |
| **probe-rs** | 与 MCP 底层一致；`./scripts/bootstrap.sh` 已装 |
| **git** | 克隆 MCP 源码到 `.tools/embedded-debugger-mcp/` |

ST-Link WinUSB 见 [probe-rs.md](probe-rs.md)。

## 安装产物

| 路径 | 说明 |
|------|------|
| `.tools/embedded-debugger-mcp/` | cargo 构建缓存（gitignore） |
| `.cursor/mcp.json` | 项目级 MCP（`embedded-debugger` server） |
| `.cursor/skills/embed-dev-lab/` | Cursor 项目 Skill 链接 |
| `skills/embed-dev-lab/SKILL.md` | Skill 源文件（可提交） |
| `AGENTS.md` | Codex 无 MCP 时追加 Skill 摘要（首次安装） |

## 各 Agent 支持

| Agent | MCP | Skill |
|-------|-----|-------|
| **Cursor** | 项目 `.cursor/mcp.json`；`--global` → `~/.cursor/mcp.json` | `.cursor/skills/` |
| **Claude Code** | `--global` → `~/.claude/settings.json` | `~/.claude/skills/` |
| **CodeWhale** | `--global` → `~/.codewhale/mcp.json` | `~/.codewhale/skills/` |
| **Codex** | **不支持 MCP**（跳过） | 读 `AGENTS.md` + `skills/embed-dev-lab/SKILL.md` |

与全局 [agent-config](https://github.com/) 的 codebase-memory / graphify **合并而非覆盖**（`mcp-merge.py` 仅新增 `embedded-debugger` 键）。

## embedded-debugger MCP 能力

基于 [embedded-debugger-mcp](https://github.com/Adancurusul/embedded-debugger-mcp) + probe-rs：

- 探测 ST-Link / J-Link 等 probe  
- 烧录、复位、RTT  
- 寄存器 / 内存读写（开发调试用）

**安全约束**（见 Skill）：禁止未经确认的全片擦除；寄存器操作需对照 RM0008 topic MD。

## 排错

| 问题 | 处理 |
|------|------|
| `cargo required` | 安装 Rust：`https://rustup.rs/` |
| MCP 面板无 server | Cursor 重载窗口；检查 `.cursor/mcp.json` |
| `embedded-debugger-mcp not built` | 运行 `./scripts/install-mcp-skills.sh` |
| probe 列表为空 | WinUSB：`stlink-winusb-windows.sh --install` |
| `--global` 未生效 | 确认对应 Agent CLI 在 PATH；用 `--agents` 指定 |
| 与 agent-config 冲突 | 两者可共存；勿手动删除其他 `mcpServers` 键 |

## 后续扩展（未默认安装）

`scripts/install/assets/agent/mcp-manifest.yaml` 预留 optional：`serial-mcp`、`esp-mcp`、`zephyr-mcp`、`platformio-mcp`。需要时再扩展 install 脚本 flag。
