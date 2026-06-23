# 应用层文档（固件模块）

本目录存放各 **`modules/<name>/`** 固件模块的人类说明；源码在仓库根 [`modules/`](../../modules/)。

| 模块 | 芯片 | 文档 | 源码 |
|------|------|------|------|
| `f103-blink` | STM32F103C8T6 | [f103-blink.md](f103-blink.md) | [`modules/f103-blink/`](../../modules/f103-blink/) |

## 与 `modules/` 的关系

```text
modules/<name>/          # 可独立 Preset 构建的固件工程（CMake + 源码）
doc/modules/<name>.md    # 模块说明、硬件要点、排错（本目录）
```

代码侧入口：[`modules/README.md`](../../modules/README.md)

## 相关学习笔记

| 主题 | 文档 |
|------|------|
| 编译与链接流程 | [f103-module-build-flow.md](../learn/f103-module-build-flow.md) |
| 链接器 map | [linker-map-file.md](../learn/linker-map-file.md) |
| CMSIS 与手写边界 | [cmsis-overview.md](../learn/cmsis-overview.md) |
| vendor-pack 三层参照 | [stm32-cmsis-component-repos.md](../learn/stm32-cmsis-component-repos.md) §5 |

## 新增模块 checklist

1. 复制 [`modules/f103-blink/`](../../modules/f103-blink/) 目录结构（`CMakeLists.txt`、`CMakePresets.json`、`src/`、`startup/`、`linker/`）
2. 在 [`cmake/mcu-config.cmake`](../../cmake/mcu-config.cmake) 使用 `embed_mcu_add_executable()`
3. 在根 [`CMakeLists.txt`](../../CMakeLists.txt) 增加 `add_subdirectory(modules/<name>)`
4. 在本目录新增 `<name>.md`，并更新本 README 模块表
5. 验证：`./scripts/build.sh <name> build`
