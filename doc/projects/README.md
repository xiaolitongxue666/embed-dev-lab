# 应用层文档（固件小工程）

本目录存放各 **`projects/<name>/`** 固件小工程的人类说明；源码在仓库根 [`projects/`](../../projects/)。

| 工程 | 芯片 | 文档 | 源码 |
|------|------|------|------|
| `f103-manual-reg` | STM32F103C8T6 | [f103-manual-reg.md](f103-manual-reg.md) | [`projects/f103-manual-reg/`](../../projects/f103-manual-reg/) |
| `f103-cmsis-hal` | STM32F103C8T6 | [f103-cmsis-hal.md](f103-cmsis-hal.md) | [`projects/f103-cmsis-hal/`](../../projects/f103-cmsis-hal/)（占位） |

## 与 `projects/` 的关系

```text
projects/<name>/          # 可独立 Preset 构建的固件工程（CMake + 源码）
doc/projects/<name>.md    # 工程说明、硬件要点、排错（本目录）
```

各工程彼此独立。代码侧入口：[`projects/README.md`](../../projects/README.md)

## 相关学习笔记

| 主题 | 文档 |
|------|------|
| 编译与链接流程 | [f103-module-build-flow.md](../learn/f103-module-build-flow.md) |
| 链接器 map | [linker-map-file.md](../learn/linker-map-file.md) |
| CMSIS 与手写边界 | [cmsis-overview.md](../learn/cmsis-overview.md) |
| vendor-pack 三层参照 | [stm32-cmsis-component-repos.md](../learn/stm32-cmsis-component-repos.md) §5 |

## 新增工程 checklist

1. 复制 [`projects/f103-manual-reg/`](../../projects/f103-manual-reg/) 目录结构（或从 CMSIS/HAL 模板新建）
2. 在 [`cmake/mcu-config.cmake`](../../cmake/mcu-config.cmake) 使用 `embed_mcu_add_executable()`
3. 在根 [`CMakeLists.txt`](../../CMakeLists.txt) 增加 `add_subdirectory(projects/<name>)`
4. 在本目录新增 `<name>.md`，并更新本 README 工程表
5. 验证：`./scripts/build.sh <name> build`
