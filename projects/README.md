# projects — 固件小工程

本目录存放可独立构建的 **裸机固件小工程**（CMake Preset + 源码 + 链接脚本）。各工程彼此独立。人类说明在 [`doc/projects/`](../doc/projects/)。

## 当前工程

| 工程 | 芯片 | 说明 | 文档 |
|------|------|------|------|
| [`f103-manual-reg/`](f103-manual-reg/) | STM32F103C8T6 | 全手写寄存器；printf + syscalls 串口 | [doc/projects/f103-manual-reg.md](../doc/projects/f103-manual-reg.md) |
| [`f103-cmsis-hal/`](f103-cmsis-hal/) | STM32F103C8T6 | CMSIS+HAL；串口用 HAL_UART_Transmit（无 printf） | [doc/projects/f103-cmsis-hal.md](../doc/projects/f103-cmsis-hal.md) |

## 构建

```bash
./scripts/build.sh <project> build    # 编译
./scripts/build.sh <project> flash    # 烧录（须先 build）
./scripts/build-flash.sh <project>     # 一键 build + flash
```

产物：`projects/<project>/build/<project>.elf`（probe-rs 烧录用）。

## 新增工程

1. 复制 [`f103-manual-reg/`](f103-manual-reg/) 目录结构（或从 CMSIS/HAL 模板新建）
2. 使用 [`cmake/mcu-config.cmake`](../cmake/mcu-config.cmake) 中 `embed_mcu_add_executable()`
3. 在根 [`CMakeLists.txt`](../CMakeLists.txt) 增加 `add_subdirectory(projects/<name>)`
4. 在 [`doc/projects/`](../doc/projects/) 新增说明并更新索引
