# STM32 裸机启动与时钟 — 学习笔记

整理自 embed-dev-lab 开发过程中的问答，供 F103 裸机入门查阅。寄存器细节以 [RM0008](../reference/stm32f103/md/rm0008-index.md) / [DS5319](../reference/stm32f103/md/datasheet-index.md) 为准。

## 资料在本仓库的分工

```text
doc/reference/stm32f103/   ST 官方 PDF + 精选 MD（权威寄存器摘录）
vendor-pack/               本地厂商包：驱动、板级例程、STM32CubeF1
modules/f103-blink/        可构建 demo：手写 startup / system / GPIO
```

| 位置 | 内容 | 是否提交 Git |
|------|------|--------------|
| `doc/reference/` | DS5319、RM0008 PDF 与 topic MD | PDF 忽略，MD 提交 |
| `vendor-pack/STLink/` | WinUSB 驱动 | USBDriver 可提交 |
| `vendor-pack/STM32F103C8T6核心板/` | 厂商 MDK 例程 | 忽略 |
| `vendor-pack/STM32CubeF1/` | ST 固件包（CMSIS/HAL） | 仅 README 提交 |

---

## Q1：`system_stm32f10x.c` 是怎么来的？

**要点**

- 本仓库版本**不是** STM32CubeMX 生成的。
- 对齐厂商裸机例程「核心板测试程序(PC13闪烁)」中的 `SetSysClockTo72`，并去掉 HAL/CMSIS 依赖，只保留 `SystemInit()` 与 RCC/FLASH 寄存器操作。
- 由 [`startup_stm32f103xb.s`](../../modules/f103-blink/startup/startup_stm32f103xb.s) 在 `main` 之前调用。

**与本仓库**

- 源码：[`modules/f103-blink/src/system_stm32f10x.c`](../../modules/f103-blink/src/system_stm32f10x.c)
- 寄存器核对：[`doc/reference/stm32f103/md/topics/rcc-clock-hse-pll.md`](../reference/stm32f103/md/topics/rcc-clock-hse-pll.md)

---

## Q2：不依赖 CubeMX/HAL，怎么只读手册写出这个文件？

**要点**

- **Datasheet（DS5319）** 不够写寄存器：它给时钟树、最高主频（F103 medium-density → 72 MHz）、HSE 电气参数。
- **Reference Manual（RM0008）§6 RCC** 才是写 `RCC_CR`、`RCC_CFGR` 每一位的依据；Flash 等待周期见 RM0008 Flash 章节。
- **两份手册如何分工、改本文件该以哪本为主**，见 [Datasheet 与 Reference Manual 怎么读？](datasheet-vs-reference-manual.md)。

**推荐步骤**

1. 确认板载 HSE 频率（本板 8 MHz）。
2. 查内存映射：`RCC` @ `0x40021000`，`FLASH` @ `0x40022000`。
3. `SystemInit` 前半：把 RCC 恢复为复位默认态。
4. 升频前设 Flash：`LATENCY=2`（72 MHz），开预取。
5. 开 HSE，轮询 `HSERDY`（加超时；失败则保持 HSI 8 MHz）。
6. 配 `RCC_CFGR`：APB1 = HCLK/2；PLL 源 HSE，×9。
7. 开 PLL，等 `PLLRDY`；`SW=PLL`，轮询 `SWS`。

**行业惯例**

| 来源 | 典型产物 |
|------|----------|
| ST CMSIS Device 包 | `system_stm32f10x.c` 模板（多 `#define` 选时钟方案） |
| CubeMX + HAL | `main.c` 里 `SystemClock_Config()`（HAL_RCC_*） |
| 裸机 / 厂商例程 | 精简版 `SystemInit`，直接写寄存器（本仓库） |

---

## Q3：RCC 寄存器是什么？

**RCC** = **R**eset and **C**lock **C**ontrol（复位与时钟控制）。

芯片的「时钟配电箱」：决定 CPU、AHB/APB 总线及各外设时钟频率与来源。

| 寄存器 | 基址 + 偏移 | 作用 |
|--------|-------------|------|
| `RCC_CR` | `0x40021000` | 开关 HSE/HSI/PLL，读就绪标志 |
| `RCC_CFGR` | `0x40021004` | 系统时钟源、PLL 倍频、总线分频 |
| `FLASH_ACR` | `0x40022000` | 与主频相关的 Flash 等待周期 |

---

## Q4：「HSE → PLL → 72 MHz」里 HSE 是什么？

| 符号 | 全称 | 含义 |
|------|------|------|
| **HSE** | High Speed **E**xternal | 板载外部晶振（本板 **8 MHz**） |
| **HSI** | High Speed **I**nternal | 片内 RC（8 MHz，上电即有，精度较差） |
| **PLL** | Phase-Locked Loop | 倍频器 |
| **SYSCLK** | System Clock | CPU 主时钟（目标 **72 MHz**） |

```text
HSE 8 MHz → PLL ×9 → SYSCLK 72 MHz
                    → APB1 = HCLK/2 = 36 MHz（APB1 上限 36 MHz）
```

HSE 启动失败时，本仓库 `set_sys_clock_to_72mhz()` 超时返回，继续使用复位默认 **HSI 8 MHz**，避免无晶振板卡死。

---

## Q5：`startup_stm32f103xb.s` 是哪里来的？

**要点**

- 标准 **CMSIS Device 包**启动流程，不是 CubeMX 逐行「生成」的逻辑。
- 文件名：`stm32f103` 系列，`xB` = **medium-density**（C8/CB 等 64K/128K Flash 档）。
- 职责：设栈 → 拷贝 `.data` → 清零 `.bss` → `SystemInit` → `main`。

**与本仓库**

- 源码：[`modules/f103-blink/startup/startup_stm32f103xb.s`](../../modules/f103-blink/startup/startup_stm32f103xb.s)
- ST 官方模板（fetch 后）：`vendor-pack/STM32CubeF1/.../Templates/gcc/startup_stm32f103xb.s`

---

## Q6：CMSIS 是什么？

**CMSIS** = **C**ortex **M**icrocontroller **S**oftware **I**nterface **S**tandard（ARM 制定的 Cortex-M 软件接口标准）。

```text
应用代码 (main.c, 驱动...)
    ↑
CMSIS-Core     — ARM：NVIC、SysTick、内核寄存器
    ↑
CMSIS-Device   — 芯片厂：stm32f103xx.h、startup、SystemInit
    ↑
STM32F103 硬件
```

**CMSIS 设备支持包** = 某系列 MCU 的 Device 层：头文件、启动汇编、`system_*.c` 等。

本仓库 **f103-blink 不链接 CMSIS 头文件**，但保留 `SystemInit` / 向量表等命名，以便与工具链和 ST 模板对照。

---

## Q7：ST CMSIS Device 包怎么获取？（官方途径）

| 途径 | 说明 |
|------|------|
| [STM32CubeF1](https://www.st.com/en/embedded-software/stm32cubef1.html) | 官网 ZIP（可能需 ST 账号）；解压后含完整 CMSIS |
| [GitHub STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1) | `git clone --recursive`（勿用 GitHub「Download ZIP」，缺 submodule） |
| [cmsis-device-f1](https://github.com/STMicroelectronics/cmsis-device-f1) | 仅 Device 层，体积较小 |
| STM32CubeMX / CubeIDE | 安装时自动下载 pack 到本地缓存 |

本仓库脚本：

```bash
./scripts/fetch-stm32cubef1.sh
./scripts/fetch-stm32cubef1.sh --verify-only
```

详见 [`vendor-pack/STM32CubeF1/README.md`](../../vendor-pack/STM32CubeF1/README.md)。

---

## Q8：和 STM32CubeMX 的关系？

- **CubeMX** 常规生成 HAL 工程：`main.c`、`SystemClock_Config()`、外设 `MX_*_Init()` 等。
- **`startup_*.s` / `system_*.c`** 通常来自 **CMSIS Device 包**，CubeMX 引用而非从零生成。
- 裸机项目可手写精简版 startup/system（本仓库做法），用 Cube 包作对照即可。

---

## 延伸阅读

- [Datasheet 与 Reference Manual 怎么读？](datasheet-vs-reference-manual.md)
- [RCC：HSE → PLL → 72 MHz](../reference/stm32f103/md/topics/rcc-clock-hse-pll.md)
- [f103-blink 模块说明](../modules-f103-blink.md)
- [脚本：fetch-stm32cubef1.sh](../scripts-reference.md#fetch-stm32cubef1sh--stm32cubef1-固件包)
