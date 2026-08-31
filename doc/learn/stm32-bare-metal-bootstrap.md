# STM32 裸机启动与时钟 — 学习笔记

整理自 embed-dev-lab 开发过程中的问答，供 F103 裸机入门查阅。寄存器细节以 [RM0008](../reference/stm32f103/md/rm0008-index.md) / [DS5319](../reference/stm32f103/md/datasheet-index.md) 为准。

## 资料在本仓库的分工

```text
doc/reference/stm32f103/   ST 官方 PDF + 精选 MD（权威寄存器摘录）
vendor-pack/               本地厂商包：驱动、板级例程、STM32CubeF1
projects/f103-manual-reg/        可构建 demo：手写 startup / system / GPIO
```

| 位置 | 内容 | 是否提交 Git |
|------|------|--------------|
| `doc/reference/` | DS5319、RM0008 PDF 与 topic MD | PDF 忽略，MD 提交 |
| `vendor-pack/STLink/` | WinUSB 驱动 | USBDriver 可提交 |
| `vendor-pack/STM32F103C8T6核心板/` | 厂商 MDK 例程 | 忽略 |
| `vendor-pack/STM32CubeF1/` | ST 固件包（CMSIS/HAL） | 仅 README 提交 |

---

## Q1：`system_stm32f1xx.c` 是怎么来的？

**要点**

- 本仓库版本**不是** STM32CubeMX 生成的。
- 对齐厂商裸机例程「核心板测试程序(PC13闪烁)」中的 `SetSysClockTo72`，并去掉 HAL/CMSIS 依赖，只保留 `SystemInit()` 与 RCC/FLASH 寄存器操作。
- 由 [`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) 在 `main` 之前调用。

**与本仓库**

- 源码：[`projects/f103-manual-reg/src/system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c)
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
| ST CMSIS Device 包 | `system_stm32f1xx.c` 模板（多 `#define` 选时钟方案） |
| CubeMX + HAL | `main.c` 里 `SystemClock_Config()`（HAL_RCC_*） |
| 裸机 / 厂商例程 | 精简版 `SystemInit`，直接写寄存器（本仓库） |

---

## Q3：RCC 寄存器是什么？

**RCC** = **R**eset and **C**lock **C**ontrol（复位与时钟控制）。

RCC 是 STM32 片上的一路 **ST 外设**（基址 `0x40021000`，见 RM0008 memory map），配套一整套 **RCC 寄存器**，用来管理：

| 职责 | 说明 | 本仓库涉及 |
|------|------|------------|
| 系统时钟源与倍频 | HSE / HSI / PLL、SYSCLK 切换 | [`system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) |
| 总线分频 | AHB、APB1、APB2 相对 HCLK 的分频 | 同上 |
| **外设时钟门控** | 各模块时钟使能（未开时钟则 MMIO 无效） | [`main.c`](../../projects/f103-manual-reg/src/main.c) 中 `RCC_APB1ENR` / `RCC_APB2ENR` |
| 复位相关控制 | 部分外设/域的复位与释放 | 间接（如先开 PWR 时钟再配 Backup 域） |

时钟是单片机的 **「心跳」**——CPU 指令节拍、外设采样与通信都依赖它。RCC 寄存器就是控制心跳的 **开关、分频器、切换器**；也可记作芯片的 **「时钟配电箱」**：决定电从哪来（HSE/HSI/PLL）、主频多少、分到哪条总线、哪路外设有没有电。

寄存器摘录与 72 MHz 配置流程见 [RCC：HSE → PLL → 72 MHz](../reference/stm32f103/md/topics/rcc-clock-hse-pll.md)；MMIO 写 RCC 的地址语义见 [MMIO 基础](stm32f103-mmio-basics.md)。

| 寄存器 | 基址 + 偏移 | 作用 |
|--------|-------------|------|
| `RCC_CR` | `0x40021000` | 开关 HSE/HSI/PLL，读就绪标志 |
| `RCC_CFGR` | `0x40021004` | 系统时钟源、PLL 倍频、总线分频 |
| `RCC_APB1ENR` / `RCC_APB2ENR` | `+0x1C` / `+0x18` | 外设时钟使能（门控） |
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

HSE 启动失败时，本仓库 `SetSysClockTo72()` 超时返回，继续使用复位默认 **HSI 8 MHz**，避免无晶振板卡死。

轮询 `HSERDY` 的原因与代码位置见 [Q9](#q9为什么需要轮询-hserdy)。

---

## Q5：`startup_stm32f103xb.s` 是哪里来的？

**要点**

- 标准 **CMSIS Device 包**启动流程，不是 CubeMX 逐行「生成」的逻辑。
- 文件名：`stm32f103` 系列，`xB` = **medium-density**（C8/CB 等 64K/128K Flash 档）。
- 职责：设栈 → 拷贝 `.data` → 清零 `.bss` → `SystemInit` → `main`。
- 语言为 GNU GAS **ARM/Thumb 汇编**（非 x86）；与 PC 汇编差异见 [Q12](#q12startup_stm32f103xbs-是汇编写的吗和-x86-汇编有什么区别)。

**与本仓库**

- 源码：[`projects/f103-manual-reg/startup/startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s)
- ST 官方模板（fetch 后）：`vendor-pack/STM32CubeF1/.../Templates/gcc/startup_stm32f103xb.s`

---

## Q6：CMSIS 是什么？

**CMSIS** = **C**ortex **M**icrocontroller **S**oftware **I**nterface **S**tandard（ARM 制定的 Cortex-M 软件接口标准），分 **Core**（内核）、**Device**（芯片寄存器与启动）与可选扩展三层。

本仓库 **f103-manual-reg 不链接 CMSIS 头文件**，但保留 `SystemInit` / 向量表等命名，属于 CMSIS **规范兼容**实现。分层详解、手写边界判定、与 HAL 关系见 **[CMSIS 标准与手写裸机边界](cmsis-overview.md)**。

---

## Q7：ST CMSIS Device 包怎么获取？（官方途径）

本仓库脚本：

```bash
./scripts/fetch-stm32cubef1.sh
./scripts/fetch-stm32cubef1.sh --verify-only
```

官方途径（STM32CubeF1、cmsis-device-f1、CubeMX 缓存等）与仓库拆分说明见 **[cmsis-overview.md §2](cmsis-overview.md#2-stm32-生态中的落地形式)**；路径表见 [`vendor-pack/STM32CubeF1/README.md`](../../vendor-pack/STM32CubeF1/README.md)。

---

## Q8：和 STM32CubeMX 的关系？

- CubeMX **生成**的主要是 HAL 层（`SystemClock_Config()`、`MX_*_Init()` 等）；**`startup_*.s` / `system_*.c`** 通常来自 CMSIS Device 包，CubeMX 引用而非从零生成。
- 裸机可手写精简版 startup/system（本仓库做法），用 Cube 包作对照。

CubeMX 与 CMSIS 的三类依赖（固定 / 随时钟变化 / 可选扩展）详见 **[cmsis-overview.md §3](cmsis-overview.md#3-cubemx-与-cmsis)**。

---

## Q9：为什么需要轮询 `HSERDY`？

[`system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) 中：

```c
#define HSE_STARTUP_TIMEOUT 0x0500U   /* 与 CMSIS 一致，1280 次循环 */

RCC_CR |= RCC_CR_HSEON;
while (!(RCC_CR & RCC_CR_HSERDY) && startup < HSE_STARTUP_TIMEOUT) {
    startup++;
}
if (!(RCC_CR & RCC_CR_HSERDY)) {
    return;   /* 超时：不升频，继续 HSI 8 MHz */
}
```

**要点**

1. **上电后外部晶振需要一点时间才能稳定起振**  
   写 `HSEON=1` 只是「请求启动」，晶振起振、幅度稳定通常要毫秒级（见 DS5319 HSE 电气特性）。此期间不能把 HSE 当 PLL 输入或系统时钟。

2. **软件轮询 `HSERDY`，不是读「HSE 本身」**  
   `RCC_CR` bit17 **`HSERDY`** 由硬件置 1，表示 HSE 已稳定可用。裸机 `SystemInit` 惯例是 busy-wait 读该位（无专用「HSE 就绪中断」流程）。

3. **`HSE_STARTUP_TIMEOUT` 防止死等**  
   无晶振、晶振损坏、负载电容不对时，`HSERDY` 永远为 0；无超时则卡在 `while` 里。超时后 `return`，跳过 PLL/72 MHz，程序仍进 `main`，用 **HSI 8 MHz**。

**归纳（流程）**

```text
HSEON → 轮询 HSERDY（计数 < HSE_STARTUP_TIMEOUT）
         ├─ HSERDY=1 → 继续 Flash 等待 / PLL / 切 72 MHz → main
         └─ 超时仍无 HSERDY → return，保持 HSI 8 MHz → main（不卡死）
```

**手册**：起振时间量级见 DS5319 §5.3.6；`HSERDY` 位定义见 RM0008 §6 RCC。

---

## Q10：f103-manual-reg 代码里有体现吗？

**有。** 逻辑在 `system_stm32f1xx.c`；`main.c` 不重复实现 HSE 等待，但会间接受时钟影响。

| 位置 | 体现 |
|------|------|
| [`system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) | `HSE_STARTUP_TIMEOUT`、轮询 `HSERDY`、超时 `return`；文件头 `@note` 说明 HSE 失败保持 HSI |
| [`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) | `Reset_Handler` 在 `main` 前 `bl SystemInit` |
| [`main.c`](../../projects/f103-manual-reg/src/main.c) | 注释指向 `system_stm32f1xx.c`；`delay()` 按 CPU 主频忙等，**不区分** 72 MHz / 8 MHz |

HSE 正常时：`delay(0xFFFFF)` 按约 72 MHz 节奏闪烁。HSE 失败时：同一延时约慢 9 倍，LED 仍闪，符合「超时退回 HSI、main 照常跑」的设计。

**对比**：同文件内 PLL 切换也轮询 `PLLRDY` / `SWS`，但**未**加超时（ST 裸机例程常见写法）；仅 HSE 等待做超时，因「无晶振板」最常见。

---

## Q11：`SystemInit` 是怎么调用的？

**要点**

- **`main.c` 不调用** `SystemInit`；进入 `main()` 前，启动汇编已完成时钟初始化。
- 上电/复位后，Cortex-M3 从向量表基址读**中断向量表**（Flash 启动：逻辑 `0x00000000/+4`，别名到物理 `0x08000000/+4`）；第二项为 `Reset_Handler` 入口。详见 **[内存映射与启动流程](stm32f103-memory-boot-map.md)** 与 **[中断向量表与 NVIC](interrupt-vector-table-and-nvic.md)**。
- `Reset_Handler` 完成 C 运行环境最小初始化后，用 `bl SystemInit` 跳转；链接阶段解析到 [`system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) 中的同名函数。

**调用链（对照本仓库源码）**

```text
硬件复位
  → g_pfnVectors @ 0x08000000          startup_stm32f103xb.s
       [0] _estack → MSP
       [1] Reset_Handler → PC
  → Reset_Handler
       mov sp, _estack
       拷贝 .data（_sidata → _sdata.._edata）
       清零 .bss（_sbss.._ebss）
  → bl SystemInit                      system_stm32f1xx.c
       RCC 恢复接近复位态
       SetSysClockTo72()：HSE×PLL×9 → 72 MHz（失败则 HSI 8 MHz）
  → bl main                            main.c
       GPIOC_Init / USART1_Init / SPI1_Init / LSM6DS3 …
  → 若 main 返回：b . 死循环
```

| 位置 | 作用 |
|------|------|
| [`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) | 向量表第二项指向 `Reset_Handler`；末尾 `bl SystemInit` / `bl main` |
| [`system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) | `SystemInit()`：RCC 复位默认化 + `SetSysClockTo72()` |
| [`main.c`](../../projects/f103-manual-reg/src/main.c) | 仅业务初始化；假定时钟已由 startup 配好 |
| [`f103-manual-reg.md`](../projects/f103-manual-reg.md) § 启动与时钟 | 模块文档中的同链表格与 mermaid |

`bl`（branch with link）等价于 x86 的 `call`：把返回地址写入 `lr`（r14），再跳转到目标。链接加 `-nostartfiles`，无 gcc crt0；`.data`/`.bss` 初始化全由 `Reset_Handler` 完成。

模块总览：[f103-manual-reg 模块文档](../projects/f103-manual-reg.md)。

---

## Q12：`startup_stm32f103xb.s` 是汇编写的吗？和 x86 汇编有什么区别？

**要点**

- 是**汇编源文件**（`.s`），由 **GNU 汇编器（GAS）** 按 **ARM/Thumb 统一语法**（`.syntax unified`）汇编，目标 CPU 为 **Cortex-M3**，不是 x86。
- 职责：放置中断向量表；实现 `Reset_Handler`（栈、`.data`/`.bss`、调 `SystemInit`、调 `main`）。详见 [Q5](#q5startup_stm32f103xbs-是哪里来的)。

**与常见 x86 汇编（PC）对比**

| 维度 | 本仓库 ARM 启动代码 | 常见 x86 汇编 |
|------|---------------------|---------------|
| CPU 架构 | ARM Cortex-M3，RISC，Thumb 指令集 | x86/x64，CISC，变长指令 |
| 运行环境 | 裸机，无 OS，上电从 Flash 向量表启动 | 多在 Linux/Windows 用户态或内核态 |
| 寄存器 | `r0`–`r15`；`sp`=r13，`lr`=r14，`pc`=r15 | `eax/ebx/...` 或 `rax/rbx/...` |
| 访存 | **Load/Store**：须 `ldr`/`str`，不能随意 `mov mem, mem` | 常可直接 `mov [addr], reg` |
| 函数调用 | `bl SystemInit` | `call SystemInit` |
| 语法风格 | GNU ARM：`.thumb`、`.word`、`ldr r0, =_estack` | Intel（`mov eax, ebx`）或 AT&T（`movl %ebx, %eax`） |
| 启动职责 | 向量表、RAM 段初始化、跳 C 入口 | 通常由 Bootloader/BIOS/UEFI 与 crt0 完成 |

**语法示例（本文件 vs x86 Intel 类比）**

ARM Thumb（[`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s)）：

```asm
ldr r0, =_estack    /* 符号地址载入 r0 */
mov sp, r0          /* 设主栈指针 MSP */
bl SystemInit       /* 调用，返回地址进 lr */
bl main
```

x86 Intel 风格（类比，非本工程）：

```asm
mov esp, offset _estack
call SystemInit
call main
```

**为何嵌入式常手写启动汇编**

复位后硬件只认向量表（第一项栈顶、第二项 Reset 入口）；`.data`/`.bss` 须在 C 全局变量可用前初始化——这些步骤 C 尚不能完成。PC 程序很少手写启动文件，因链接器自带的 `crt0.o` 与 OS 加载器已代为处理。

ST 官方 CMSIS 模板按工具链分 `gcc` / `iar` / `arm`（Keil）三版，向量表顺序与 Reset 流程一致，仅汇编语法不同。见 [cmsis-overview.md §2.1](cmsis-overview.md#21-官方仓库拆分)。

---

## Q13：为什么链接脚本用 `0x08000000`，复位却从 `0x00000000` 读向量表？

**要点**

- **链接脚本 / 烧录 / map** 使用 Main Flash **物理**基址 **`0x08000000`**（RM0008 memory map）。
- **CPU 复位**按 ARM 规定从向量表基址 **+0 / +4** 读 MSP 与 PC；默认基址为逻辑 **`0x00000000`**。
- Flash 启动（BOOT0=0）时，ST 将 **`0x00000000` 别名到 `0x08000000`**：读逻辑 `0x00000000` 与读物理 `0x08000000` **内容相同**，是地址转发，**不复制数据**。
- 因此：向量表在 ELF 里位于 `0x08000000`；复位硬件从 `0x00000000` 入口读到同一张表。

**与本仓库**

- [`STM32F103C8_FLASH.ld`](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) — `ORIGIN = 0x08000000`
- [`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) — `g_pfnVectors` 链接到 Flash 物理起始

完整 BOOT 表、System memory、Flash 段布局与启动流程见 **[STM32F103 内存映射与启动流程](stm32f103-memory-boot-map.md)**。

---

## Q14：外设寄存器地址从哪来？MMIO 与 `.data` 有何不同？

**要点**

- 外设 **基址 + 偏移** 来自 **RM0008 memory map**（Table 1）；本仓库用 `#define RCC_BASE 0x40021000U` 等照抄，编译后地址常量在 **Flash 指令**里，**不在 `.data` 段**。
- **Memory-mapped I/O**：`RCC_APB2ENR |= …` 是对总线地址 **load/store**，硬件解码后写入 **外设寄存器（flip-flop）**，不是写 SRAM。
- **Memory map** 是地址解码表，**不是**把外设「映进 RAM」；仅 `.data` 全局变量初值在启动时 Flash→SRAM 拷贝。
- NVIC 等在内核 **PPB `0xE000xxxx`**，与 ST 外设 **`0x40000000`** 不同段，见 [memory-boot-map §2.1](stm32f103-memory-boot-map.md#21-soc-分层cpu-内核-vs-st-外设-vs-ppb)。

**与本仓库**

- [`main.c`](../../projects/f103-manual-reg/src/main.c) — RCC/PWR/GPIOC MMIO；PC13 点灯 walkthrough
- [`gpioc_bitband.h`](../../projects/f103-manual-reg/src/gpioc_bitband.h) — `PCout` 位带写 `GPIOC_ODR` @ `0x4001100C`

详见 **[STM32F103 MMIO 基础](stm32f103-mmio-basics.md)**。

---

## 延伸阅读

- [中断向量表与 NVIC](interrupt-vector-table-and-nvic.md) — 向量表作用、NVIC 职责、与 CMSIS/HAL 关系
- [STM32F103 内存映射与启动流程](stm32f103-memory-boot-map.md) — BOOT 重映射、Flash/SRAM、复位加载、与 x86 对比
- [STM32F103 MMIO 基础](stm32f103-mmio-basics.md) — 外设地址、flip-flop、PC13 点灯、PPB vs ST 外设
- [f103-manual-reg 编译流程](f103-module-build-flow.md) — CMake、.c/.s 链接、链接脚本与 startup 协作
- [CMSIS 标准与手写裸机边界](cmsis-overview.md)
- [Datasheet 与 Reference Manual 怎么读？](datasheet-vs-reference-manual.md)
- [RCC：HSE → PLL → 72 MHz](../reference/stm32f103/md/topics/rcc-clock-hse-pll.md)
- [f103-manual-reg 模块说明](../projects/f103-manual-reg.md)
- [脚本：fetch-stm32cubef1.sh](../scripts-reference.md#fetch-stm32cubef1sh--stm32cubef1-固件包)
