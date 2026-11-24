---
name: firmware-build
description: 在 embed-dev-lab 编译 STM32F103 固件时使用。包括 build.sh 的 all、configure、build、clean，clean 之后，以及 f103-cmsis-hal 的 fetch。不包含烧录。
---

# 编译

Windows 在 Git Bash 中执行。CLI 输出为英文。本 Skill 不烧录。烧录见 [firmware-flash](../firmware-flash/SKILL.md)。

`<module>` 为 `f103-manual-reg`、`f103-cmsis-hal` 或 `both`。

## 先确认动作

| 动作 | 命令 | 行为 |
|------|------|------|
| 首次，或 `clean` 之后，或没有 `build/` | `./scripts/build.sh <module>` | 默认 `all`：configure + build |
| 日常只编译（`build/` 已在） | `./scripts/build.sh <module> build` | 只 ninja build，不 configure |
| 只 configure | `./scripts/build.sh <module> configure` | 不编译 |
| 清掉构建目录 | `./scripts/build.sh <module> clean` | 删除该模块 `build/`。清完必须再跑不带动作的 `./scripts/build.sh <module>`，不要接着只用 `build` |

`build` 与 `build-flash.sh` 都不 configure。`build/` 不存在时它们会失败。

## cmsis-hal 首次

缺头文件或还没有 third_party 时，按这个顺序：

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
./scripts/build.sh f103-cmsis-hal
```

`fetch-f103-cmsis-hal-deps.sh` 依赖已初始化的 CMSIS submodule。不要跳过 `fetch-cmsis.sh`。

## 两条路线一起编译

```bash
./scripts/build.sh both build
```

`both` 对 `configure`、`build`、`clean`、`all` 会按顺序跑两个工程。不要用 `both flash`（烧写 Skill 会拒绝）。

## 产物

- `projects/f103-manual-reg/build/f103-manual-reg.elf`
- `projects/f103-cmsis-hal/build/f103-cmsis-hal.elf`

编译失败就停。不要进入烧写。gcc/ninja 的英文报错原样保留。

## 失败分支

| 现象 | 处理 |
|------|------|
| `build` 或 `build-flash` 报没有 build 目录 | `./scripts/build.sh <module>` |
| cmsis-hal 缺头文件 | 按上面的 fetch 顺序重做，再 `./scripts/build.sh f103-cmsis-hal` |
| 刚执行过 `clean` | 不要只跑 `build`。先 `./scripts/build.sh <module>` |
| cmsis-hal 链接出 `_close` / `_write` 等 `not implemented and will always fail` | newlib nosys 的警告。退出码仍为 0 时编译通过，不要当成失败 |

环境与脚本说明：[`doc/workflow-write-build-flash.md`](../../../doc/workflow-write-build-flash.md)、[`doc/scripts-reference.md`](../../../doc/scripts-reference.md)。
