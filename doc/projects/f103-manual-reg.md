# f103-manual-reg 模块

**STM32F103C8T6** 核心板 **PC13 LED 闪烁** demo，纯寄存器实现，对齐厂商例程 `vendor-pack/.../核心板测试程序(PC13闪烁)`。

## 模块定位

| 项 | 说明 |
|----|------|
| 目标芯片 | STM32F103C8T6（Cortex-M3，Medium-density F103xB） |
| 实现方式 | 全手写 `startup` / `system_stm32f1xx.c` / GPIO 寄存器，**不链接** CMSIS submodule 与 HAL |
| 兼容性 | 精简启动与复位流程（`SystemInit`、向量表）；与官方 CMSIS 模板**对照学习**，非同一套源码 |
| 参照关系 | 与 [`vendor-pack`](../../vendor-pack/) 三层对照见 [ST F1 软件仓库归纳 §5](../learn/stm32-cmsis-component-repos.md#5-与-embed-dev-lab-的三层参照)；从零手写流程见 [从零手写构建指南](../learn/f103-manual-build-from-scratch.md) |

## 目录结构

```text
projects/f103-manual-reg/
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── main.c              # GPIO 初始化与闪烁主循环
│   ├── system_stm32f1xx.c  # SystemInit，HSE→72 MHz
│   ├── system_stm32f1xx.h  # SystemInit 声明
│   └── gpioc_bitband.h     # PCout(n) 位带宏
├── startup/
│   └── startup_stm32f103xb.s   # 向量表、.data/.bss、Reset_Handler
└── linker/
    └── STM32F103C8_FLASH.ld      # 64K Flash / 20K RAM
```

## 源码地图

| 文件 | 职责 | 延伸阅读 |
|------|------|----------|
| [`startup/startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) | 向量表、`Reset_Handler`、`.data`/`.bss` 初始化、`bl SystemInit`、`bl main` | [编译流程](../learn/f103-module-build-flow.md)、[中断向量表与 NVIC](../learn/interrupt-vector-table-and-nvic.md) |
| [`src/system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) | `SystemInit()`：RCC 复位默认化，HSE×PLL→72 MHz；HSE 超时保持 HSI | [裸机入门 Q11/Q12](../learn/stm32-bare-metal-bootstrap.md#q11systeminit-是怎么调用的)、[RCC/HSE topic](../reference/stm32f103/md/topics/rcc-clock-hse-pll.md) |
| [`src/main.c`](../../projects/f103-manual-reg/src/main.c) | PWR+DBP、GPIOC 配置、PC13 闪烁主循环 | [MMIO 与 PC13](../learn/stm32f103-mmio-basics.md)、[Backup 域与 PC13](../reference/stm32f103/md/topics/backup-domain-pc13.md) |
| [`src/gpioc_bitband.h`](../../projects/f103-manual-reg/src/gpioc_bitband.h) | `PCout(n)` 位带宏，简化 ODR 读写 | [MMIO 基础 §5](../learn/stm32f103-mmio-basics.md#5-f103-manual-reg-pc13-点灯完整-mmio-流程) |
| [`linker/STM32F103C8_FLASH.ld`](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) | Flash 64K / RAM 20K，`.isr_vector` 固定 Flash 物理 `0x08000000` | [从零手写构建](../learn/f103-manual-build-from-scratch.md)、[内存映射与启动](../learn/stm32f103-memory-boot-map.md)、[链接器 map](../learn/linker-map-file.md)、[memory-map topic](../reference/stm32f103/md/topics/memory-map-medium-density.md) |

## 构建与烧录

C 源文件（`src/`）与启动汇编（`startup/`）在 CMake 中列入同一 `SOURCES`，分别编译为 `.o` 后由链接脚本合并为单一 `.elf`；详见 [f103-manual-reg 编译流程](../learn/f103-module-build-flow.md)。

```bash
./scripts/build.sh f103-manual-reg          # configure + build
./scripts/build.sh f103-manual-reg build
./scripts/build.sh f103-manual-reg flash
./scripts/build-flash.sh               # 一键 build + flash（编译失败暂停）
```

产物：

- `projects/f103-manual-reg/build/f103-manual-reg.elf` — 链接输出；**probe-rs / IDE F5** 直接烧录
- `projects/f103-manual-reg/build/f103-manual-reg.hex` — build 时自动生成（见下方）
- `projects/f103-manual-reg/build/f103-manual-reg.map` — 链接 map（调试用）

probe-rs chip：**`STM32F103C8Tx`**

## 构建产物与 ELF→HEX

本项目使用 **`arm-none-eabi-gcc` 裸机工具链**（非 `arm-linux-gnueabihf` Linux 应用链）。

| 步骤 | 位置 | 说明 |
|------|------|------|
| 链接 `.elf` | Ninja（`cmake --build`） | `embed_mcu_add_executable` 在 [`cmake/mcu-config.cmake`](../../cmake/mcu-config.cmake) |
| `.elf` → `.hex` | 同上目标的 **POST_BUILD** | `arm-none-eabi-objcopy -O ihex f103-manual-reg.elf f103-manual-reg.hex` |
| 生成 `.bin` | **无** | 当前未配置；需要可在 `mcu-config.cmake` POST_BUILD 增加 `-O binary` |

烧录时格式选择：

| 命令 / 方式 | 使用文件 |
|-------------|----------|
| `./scripts/build.sh f103-manual-reg flash` | `.elf`（probe-rs） |
| `./scripts/build.sh f103-manual-reg flash-openocd` | `.hex`（OpenOCD） |
| IDE **F103 Probe-rs Debug**（F5） | `.elf` |

```text
build.sh build → Ninja 链接 → f103-manual-reg.elf
              → POST_BUILD objcopy → f103-manual-reg.hex

build.sh flash          → probe-rs 读 .elf
build.sh flash-openocd  → OpenOCD 读 .hex
```

## 启动与时钟

上电后 [`startup/startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) 的 `Reset_Handler` 设栈、初始化 `.data`/`.bss`，再 `bl SystemInit`（**不在 `main.c` 中调用**），最后进入 `main`。

`SystemInit()`（[`src/system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c)）尝试 **HSE 8 MHz × PLL9 → 72 MHz**。HSE 超时则保持复位默认 **HSI 8 MHz**，避免无晶振板卡死。

## CMSIS 对照路径

本模块**不链接**官方 CMSIS 头文件与 HAL；下列路径供**对照**官方模板：

| 层级 | 主路径（submodule） | 可选（CubeF1 fetch 后） |
|------|---------------------|-------------------------|
| Device 启动 / system | `vendor-pack/cmsis-device-f1/Source/Templates/gcc/startup_stm32f103xb.s` | `vendor-pack/STM32CubeF1/.../Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/` |
| Device 头文件 | `vendor-pack/cmsis-device-f1/Include/stm32f103xb.h` | 同上 `Include/` |
| Core | `vendor-pack/cmsis-core/Include/core_cm3.h` | Cube 包内 `Drivers/CMSIS/Include/` |

获取 submodule：`./scripts/fetch-cmsis.sh`（Core `cm3`/`v5.6.0_cm3` + Device `v4.3.5`）。概念说明见 [CMSIS 标准与手写裸机边界](../learn/cmsis-overview.md)。

## PC13 与 Backup 域

PC13 属于 **Backup 域** GPIO，配置前必须：

1. `RCC_APB1ENR.PWREN` — 开启 PWR 时钟  
2. `PWR_CR.DBP` — 解除 Backup 域写保护  

否则 `GPIOC_CRH` 写入无效，LED 不亮。见 `src/main.c` 中 `GPIOC_Init()`。

详细寄存器说明与 PDF 页码：[Backup 域与 PC13](../reference/stm32f103/md/topics/backup-domain-pc13.md)

## LED 极性

多数核心板 **低电平点亮**：

- `PCout(13) = 1` → 灭  
- `PCout(13) = 0` → 亮  

闪烁逻辑：高 → 延时 → 低 → 延时（与厂商 `main.c` 一致）。

## 硬件参考

| 项目 | 说明 |
|------|------|
| LED 引脚 | PC13 |
| SWD | SWDIO=PA13, SWCLK=PA14 |
| 厂商例程 | `vendor-pack/STM32F103C8T6核心板/.../核心板测试程序(PC13闪烁)/` |

## 排错速查

| 现象 | 优先检查 |
|------|----------|
| 烧录成功但 LED 不闪 | `PWR`+`DBP` 是否已配置；是否先 `build` 再 `flash`；`probe-rs reset` |
| 程序卡死、无任何 IO | HSE 超时逻辑（`system_stm32f1xx.c`）；无晶振时应保持 HSI |
| `probe-rs list` 为空 | Windows WinUSB：[`stlink-winusb-windows.sh`](../../scripts/install/stlink-winusb-windows.sh) |
| 改代码后烧录仍是旧行为 | `flash` **不自动编译**；须 `build` 后再 `flash` |

完整问题表：[PROJECT_MEMORY.md](../../PROJECT_MEMORY.md)「问题 ↔ 解法」。

## 调试

- IDE：**F103 Probe-rs Debug**（`.vscode/launch.json`）
- CLI：见 [probe-rs.md](../probe-rs.md)

## 新增工程参考

- **从零手写**：按依赖顺序写链接脚本 / startup / system / main，见 [从零手写构建指南](../learn/f103-manual-build-from-scratch.md)
- **复制目录**：见 [应用层文档索引](README.md#新增工程-checklist)
