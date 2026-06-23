# modules — 固件应用层

本目录存放可独立构建的 **裸机固件模块**（CMake Preset + 源码 + 链接脚本）。人类说明文档在 [`doc/modules/`](../doc/modules/)。

## 当前模块

| 模块 | 芯片 | 文档 |
|------|------|------|
| [`f103-blink/`](f103-blink/) | STM32F103C8T6 | [doc/modules/f103-blink.md](../doc/modules/f103-blink.md) |

## 构建

```bash
./scripts/build.sh <module> build    # 编译
./scripts/build.sh <module> flash    # 烧录（须先 build）
./scripts/build-flash.sh <module>   # 一键 build + flash
```

产物：`modules/<module>/build/<module>.elf`（probe-rs 烧录用）。

## 新增模块

1. 复制 `f103-blink/` 目录结构
2. 使用 [`cmake/mcu-config.cmake`](../cmake/mcu-config.cmake) 中 `embed_mcu_add_executable()`
3. 在根 [`CMakeLists.txt`](../CMakeLists.txt) 增加 `add_subdirectory(modules/<name>)`
4. 在 [`doc/modules/`](../doc/modules/) 新增说明并更新索引
