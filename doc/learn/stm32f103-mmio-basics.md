# STM32F103 Memory-Mapped I/O 基础

整理自 embed-dev-lab 开发过程中的问答，说明 F103 裸机上 **外设寄存器如何通过地址访问**、与 Flash/SRAM 段的区别，并以本仓库 [`f103-manual-reg`](../../projects/f103-manual-reg/) **PC13 点灯** 为完整实例。SoC 分层与 4 GB 地址空间见 [内存映射与启动流程](stm32f103-memory-boot-map.md) §2 / §2.1。

---

## 一句话总结

**Memory-mapped I/O = CPU 用读写「地址」的方式操作外设**；F103 裸机无 MMU，代码里的 `0x40021000` 等与 RM0008 手册 **同一套总线实地址**。外设寄存器在 **外设硬件（flip-flop）** 里，不在 Flash `.data`；`#define` 基址编译进 Flash 指令，写寄存器走总线解码。

---

## 1. Memory-mapped I/O 是什么？

Cortex-M 没有 x86 那种单独的 `in/out` 端口指令。外设与 SRAM/Flash **共用 32 位地址空间**：对某地址做 **load/store**，硬件决定访问 RAM、Flash 还是外设寄存器。

| 说法 | 含义 |
|------|------|
| 「memory-mapped」 | 外设也占「内存地址」 |
| **不是** | 外设被「映射进 SRAM」或存在 `.data` 段 |

```text
str r0, [0x20000100]  →  写到 SRAM 变量
str r0, [0x4001100C]  →  写到 GPIOC_ODR 寄存器（MMIO）
```

---

## 2. 手册地址 = 裸机实地址

| 步骤 | 说明 |
|------|------|
| ST 定址 | RM0008 §2.3 / Table 1 给出外设 **基址 + 偏移** |
| 代码 | `#define RCC_BASE 0x40021000U` 等照抄手册 |
| 编译 | 地址作为立即数或常量池，进入 Flash `.text`/`.rodata` |
| 运行 | CPU 对该地址 load/store → **地址解码器** 选中外设模块内对应寄存器 |

F103 无 MMU：**调试器、map 文件、手册、代码中的地址一致**（平坦物理地址）。与 x86 用户态虚拟地址对比见 [memory-boot-map §8](stm32f103-memory-boot-map.md#8-与-x86-虚拟地址对比)。

### 2.1 地址如何对应：总线解码（不是映进 RAM）

对应关系**不是**软件把外设「映射进 RAM」，而是 **硬件布线 + 手册规定**：RM0008 给每个外设固定 **基址 + 偏移**；芯片内部 **地址解码器** 根据 CPU 发出的地址选中 Flash、SRAM、外设模块或 PPB。load/store 直接改外设里的 flip-flop，**不会**先把寄存器拷进 SRAM。

以写 `GPIOC_ODR`（`0x4001100C`）为例：

```text
CPU 发出地址 0x4001100C
        │
        ▼
   地址解码器（芯片内部逻辑）
        │
        ├── 0x2000xxxx ──► 选中 SRAM
        ├── 0x0800xxxx ──► 选中 Main Flash
        ├── 0x40011000–0x400113FF ──► 选中 GPIOC 模块
        │         └── 偏移 +0x0C ──► 该模块内的 ODR 寄存器
        └── 其他区间 ──► RCC、USART…
```

§1 中 `str r0, [0x4001100C]` 走的就是上图中 **GPIOC / ODR** 分支，而非 `0x2000xxxx`（SRAM）分支。

| 环节 | 来源 | 本仓库例子 |
|------|------|------------|
| 手册 | RM0008 Table 1：`GPIOC` @ `0x40011000`，`ODR` @ `+0x0C` | [backup-domain-pc13 topic](../reference/stm32f103/md/topics/backup-domain-pc13.md) |
| 代码 | `#define` + `volatile` 指针 | `GPIOC_ODR` → `0x4001100C` |
| 编译 | 地址进 Flash `.text` 立即数 | `str` / `PCout(13)` |
| 硬件 | 解码 → GPIOC flip-flop → PC13 | [§5](#5-f103-manual-reg-pc13-点灯完整-mmio-流程) |

---

## 3. 寄存器与 flip-flop

**Flip-flop（触发器）** 是能存 **1 bit** 的数字电路单元；**外设寄存器** 是一组 flip-flop（通常 32 位宽）。

| 存什么 | 在哪里 | 例子 |
|--------|--------|------|
| 外设寄存器**当前值** | 外设模块硬件 | `GPIOC_ODR` bit13 |
| 程序**指令** | Main Flash | `main()`、`GPIOC_Init` |
| 地址**常量** | Flash 指令/立即数 | `0x4001100C` 编在 `str` 里 |
| 全局变量初值 | Flash LMA → SRAM VMA | `int x = 42` |
| 栈/局部变量 | SRAM | `delay()` 的 `count` |

写 `GPIOC_ODR` 不会改 Flash 或 `.data` 里的内容，只改 **GPIOC 外设里** 的 flip-flop，进而驱动 **PC13 引脚**。

---

## 4. `volatile` 指针惯用法

本仓库 [`main.c`](../../projects/f103-manual-reg/src/main.c)：

```c
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))
RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;
```

| 部分 | 作用 |
|------|------|
| `(RCC_BASE + 0x18U)` | 手册地址 → `0x40021018` |
| `(volatile unsigned int *)` | 告诉编译器：每次读写都访问硬件，不可优化成普通变量 |
| `*()` | 解引用 = 对该地址做 MMIO 读/写 |

---

## 5. f103-manual-reg PC13 点灯：完整 MMIO 流程

### 5.1 地址对应表（RM0008 → 代码）

| 操作 | 寄存器 | 总线地址 | 代码 |
|------|--------|----------|------|
| 开 PWR 时钟 | `RCC_APB1ENR` bit28 | `0x4002101C` | `RCC_APB1ENR \|= PWREN` |
| 解除 Backup 写保护 | `PWR_CR` bit8 DBP | `0x40007000` | `PWR_CR \|= DBP` |
| 开 GPIOC 时钟 | `RCC_APB2ENR` bit4 | `0x40021018` | `RCC_APB2ENR \|= IOPCEN` |
| 配置 PC13 模式 | `GPIOC_CRH` bit[23:20] | `0x40011004` | `GPIOC_CRH &= …; \|= …` |
| 拉高/拉低 PC13 | `GPIOC_ODR` bit13 | `0x4001100C`（`0x40011000`+`0x0C`，见 [§2.1](#21-地址如何对应总线解码不是映进-ram)） | `PCout(13) = 0/1` |

寄存器位域与 Backup 域顺序见 [backup-domain-pc13 topic](../reference/stm32f103/md/topics/backup-domain-pc13.md)。

### 5.2 初始化（[`GPIOC_Init`](../../projects/f103-manual-reg/src/main.c)）

```c
RCC_APB1ENR |= RCC_APB1ENR_PWREN;      /* MMIO 写 0x4002101C */
PWR_CR |= PWR_CR_DBP;                 /* MMIO 写 0x40007000 */
RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;    /* MMIO 写 0x40021018 */
GPIOC_CRH &= ~GPIOC_CRH_PC13_MASK;    /* 读-改-写 0x40011004 */
GPIOC_CRH |= GPIOC_CRH_PC13_OUT_PP;
```

### 5.3 闪烁（[`main`](../../projects/f103-manual-reg/src/main.c) + 位带）

[`gpioc_bitband.h`](../../projects/f103-manual-reg/src/gpioc_bitband.h) 中 `PCout(13)` 通过 **位带别名** 写 `GPIOC_ODR` bit13，本质仍是 MMIO，目标寄存器 **`0x4001100C`**。下面 `Store` 步骤经 [§2.1](#21-地址如何对应总线解码不是映进-ram) 解码树选中 GPIOC 的 ODR。

```mermaid
flowchart LR
    FlashCode["Flash 中 main 指令"] --> Store["CPU store 到 0x4001100C 或位带别名"]
    Store --> Bus["AHB/APB 总线解码"]
    Bus --> ODR["GPIOC_ODR flip-flop bit13"]
    ODR --> Pin["PC13 引脚电平"]
    Pin --> LED["板载 LED"]
```

多数 C8 核心板 **低电平点亮**：`PCout(13)=0` 亮，`=1` 灭。

---

## 6. Flash 各段 vs 外设地址

| 位置 | 存什么 | 外设地址在这吗？ |
|------|--------|------------------|
| Flash `.text` | 机器码 | 地址常量多嵌在指令里 |
| Flash `.rodata` | 只读常量 | 有时 literal pool |
| Flash `.data` LMA | 全局变量**初值镜像** | **否** |
| SRAM `.data` / `.bss` | 运行时变量 | **否**（除非定义 `uint32_t *p = (void*)0x40021000`） |
| `0x400xxxxx` | **外设寄存器硬件** | 这是访问**目标**，不是程序段 |

---

## 7. PPB 与 ST 外设（与 MMIO 的关系）

| 地址区 | 内容 | MMIO？ |
|--------|------|--------|
| `0x40000000` | ST 外设（RCC、GPIO…） | 是；本仓库主要写这类 |
| `0xE0000000` | ARM PPB（NVIC、SysTick、SCB/VTOR） | 是；NVIC 配置通常用 CMSIS 或手写 `0xE000xxxx` |

两者都是 MMIO，但 **设计方与手册章节不同**。见 [memory-boot-map §2.1](stm32f103-memory-boot-map.md#21-soc-分层cpu-内核-vs-st-外设-vs-ppb)。

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| PC13 / PWR / DBP 寄存器摘录 | [backup-domain-pc13.md](../reference/stm32f103/md/topics/backup-domain-pc13.md) |
| NVIC 与 PPB | [interrupt-vector-table-and-nvic.md](interrupt-vector-table-and-nvic.md) |
| 内存映射、Boot、Flash 布局 | [stm32f103-memory-boot-map.md](stm32f103-memory-boot-map.md) |
| DS5319 / RM0008 分工 | [datasheet-vs-reference-manual.md](datasheet-vs-reference-manual.md) |
| 裸机 Q14 速查 | [stm32-bare-metal-bootstrap.md Q14](stm32-bare-metal-bootstrap.md#q14外设寄存器地址从哪来mmio-与-data-有何不同) |
| 模块源码 | [`projects/f103-manual-reg/`](../../projects/f103-manual-reg/) |
