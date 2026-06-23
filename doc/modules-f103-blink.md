# f103-blink 模块

**STM32F103C8T6** 核心板 **PC13 LED 闪烁** demo，纯寄存器实现，对齐厂商例程 `vendor-pack/.../核心板测试程序(PC13闪烁)`。

## 目录结构

```text
modules/f103-blink/
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── main.c              # GPIO 初始化与闪烁主循环
│   ├── system_stm32f10x.c  # SystemInit，HSE→72 MHz
│   └── gpio_like51.h       # PCout(n) 位带宏
├── startup/
│   └── startup_stm32f103xb.s   # 向量表、.data/.bss、Reset_Handler
└── linker/
    └── stm32f103c8.ld      # 64K Flash / 20K RAM
```

## 构建与烧录

C 源文件（`src/`）与启动汇编（`startup/`）在 CMake 中列入同一 `SOURCES`，分别编译为 `.o` 后由链接脚本合并为单一 `.elf`；详见 [f103-blink 编译流程](learn/f103-module-build-flow.md)。

```bash
./scripts/build.sh f103-blink          # configure + build
./scripts/build.sh f103-blink build
./scripts/build.sh f103-blink flash
./scripts/build-flash.sh               # 一键 build + flash（编译失败暂停）
```

产物：

- `modules/f103-blink/build/f103-blink.elf` — 链接输出；**probe-rs / IDE F5** 直接烧录
- `modules/f103-blink/build/f103-blink.hex` — build 时自动生成（见下方）
- `modules/f103-blink/build/f103-blink.map` — 链接 map（调试用）

probe-rs chip：**`STM32F103C8Tx`**

## 构建产物与 ELF→HEX

本项目使用 **`arm-none-eabi-gcc` 裸机工具链**（非 `arm-linux-gnueabihf` Linux 应用链）。

| 步骤 | 位置 | 说明 |
|------|------|------|
| 链接 `.elf` | Ninja（`cmake --build`） | `embed_mcu_add_executable` 在 [`cmake/mcu-config.cmake`](../cmake/mcu-config.cmake) |
| `.elf` → `.hex` | 同上目标的 **POST_BUILD** | `arm-none-eabi-objcopy -O ihex f103-blink.elf f103-blink.hex` |
| 生成 `.bin` | **无** | 当前未配置；需要可在 `mcu-config.cmake` POST_BUILD 增加 `-O binary` |

烧录时格式选择：

| 命令 / 方式 | 使用文件 |
|-------------|----------|
| `./scripts/build.sh f103-blink flash` | `.elf`（probe-rs） |
| `./scripts/build.sh f103-blink flash-openocd` | `.hex`（OpenOCD） |
| IDE **F103 Probe-rs Debug**（F5） | `.elf` |

```text
build.sh build → Ninja 链接 → f103-blink.elf
              → POST_BUILD objcopy → f103-blink.hex

build.sh flash          → probe-rs 读 .elf
build.sh flash-openocd  → OpenOCD 读 .hex
```

## 启动与时钟

上电后 [`startup/startup_stm32f103xb.s`](../../modules/f103-blink/startup/startup_stm32f103xb.s) 的 `Reset_Handler` 设栈、初始化 `.data`/`.bss`，再 `bl SystemInit`（**不在 `main.c` 中调用**），最后进入 `main`。调用链与汇编/x86 差异见 [裸机入门 Q11/Q12](learn/stm32-bare-metal-bootstrap.md#q11systeminit-是怎么调用的)；向量表与 NVIC 见 [中断向量表与 NVIC](learn/interrupt-vector-table-and-nvic.md)。

`SystemInit()`（[`src/system_stm32f10x.c`](../../modules/f103-blink/src/system_stm32f10x.c)）尝试 **HSE 8 MHz × PLL9 → 72 MHz**。HSE 超时则保持复位默认 **HSI 8 MHz**，避免无晶振板卡死。

## CMSIS 兼容性

本模块**不链接**官方 CMSIS 头文件与 HAL，但 startup / `SystemInit` 遵循 CMSIS 规范（向量表顺序、Reset 流程、标准接口名），属于 **CMSIS 生态兼容**的手写实现。概念说明与 ST 官方模板对照路径见 [CMSIS 标准与手写裸机边界](learn/cmsis-overview.md)；官方模板 fetch 后位于 `vendor-pack/STM32CubeF1/.../Templates/`。

## PC13 与 Backup 域

PC13 属于 **Backup 域** GPIO，配置前必须：

1. `RCC_APB1ENR.PWREN` — 开启 PWR 时钟  
2. `PWR_CR.DBP` — 解除 Backup 域写保护  

否则 `GPIOC_CRH` 写入无效，LED 不亮。见 `src/main.c` 中 `gpio_configuration()`。

详细寄存器说明与 PDF 页码：[Backup 域与 PC13](reference/stm32f103/md/topics/backup-domain-pc13.md)

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

## 调试

- IDE：**F103 Probe-rs Debug**（`.vscode/launch.json`）
- CLI：见 [probe-rs.md](probe-rs.md)

## 新增模块参考

1. 复制 `modules/f103-blink/` 结构  
2. 使用 `cmake/mcu-config.cmake` 中 `embed_mcu_add_executable()`  
3. `./scripts/build.sh <新模块名>`
