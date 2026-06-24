# 中断向量表与 NVIC

整理自 embed-dev-lab 开发过程中的问答，说明 Cortex-M3 上 **中断向量表** 与 **NVIC** 的概念、作用及与本仓库 [`f103-manual-reg`](../../projects/f103-manual-reg/) startup 的对应关系。启动流程与 `Reset_Handler` 调用链见 [STM32 裸机启动与时钟](stm32-bare-metal-bootstrap.md)；链接阶段向量表如何落到 Flash 起始见 [f103-manual-reg 编译流程](f103-module-build-flow.md)。

---

## 一句话总结

**中断向量表**是放在 Flash 里的「异常/中断服务函数地址对照表」；**NVIC** 是 Cortex-M 内核里负责查表、判优先级、控制跳转与嵌套的硬件调度中枢。CMSIS-Core 为 NVIC 提供标准寄存器定义与操作函数；STM32 外设中断只是 NVIC 管理的输入源之一。

---

## 什么是中断向量表

中断向量表（Cortex-M 文档中常称 **Exception and Interrupt Vector Table**）是一张 **函数指针数组**：每个元素是一个 32 位地址，指向某类异常或中断发生时要执行的代码入口。

在本仓库中，表名为 `g_pfnVectors`，由 [`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) 定义，链接脚本将其放入 `.isr_vector` 段，固定在 Flash 起始 `0x08000000`：

```31:47:projects/f103-manual-reg/startup/startup_stm32f103xb.s
g_pfnVectors:
    .word _estack              /* 0  初始主栈顶 */
    .word Reset_Handler        /* 1  复位 */
    .word NMI_Handler          /* 2  不可屏蔽中断 */
    .word HardFault_Handler    /* 3  硬 fault */
    .word MemManage_Handler    /* 4  存储器管理 fault */
    .word BusFault_Handler     /* 5  总线 fault */
    .word UsageFault_Handler   /* 6  用法 fault */
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler          /* 11 SVCall */
    .word DebugMon_Handler     /* 12 调试监视 */
    .word 0
    .word PendSV_Handler       /* 14 PendSV */
    .word SysTick_Handler      /* 15 SysTick */
```

**注意**：索引 0 的项 **不是函数**，而是初始主栈顶 `_estack`（`0x20005000`，RAM 上界）；从索引 1 起才是入口地址。Cortex-M3 为满递减栈：复位后 MSP 从此处开始，**push 时 SP 向低地址减小**（不是增大）。详见 [内存映射 §6.1](stm32f103-memory-boot-map.md#61-主栈满递减与-_estack)。

CMakeLists 注释中的「向量表」即指这张表：

```15:18:projects/f103-manual-reg/CMakeLists.txt
set(F103_SOURCES
    src/main.c                  # 应用入口与 GPIO 闪烁
    src/system_stm32f1xx.c      # SystemInit / 72 MHz 时钟
    startup/startup_stm32f103xb.s # 向量表、.data/.bss、跳转 main
```

链接脚本要求向量表必须位于 Flash 最前（`ALIGN` / `KEEP` 语义见 [从零手写 §2.2](f103-manual-build-from-scratch.md#22-align-与-keepisr_vector-段常用)）：

### 链接脚本中的 `.isr_vector`

```39:45:projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld
    /* ---------- 输出段 .isr_vector：中断向量表，须固定 Flash 物理起始 0x08000000 ---------- */
    .isr_vector :                        /* 独立输出段；排在 .text 之前，故链接后位于 Flash 最前 */
    {
        . = ALIGN(4);                    /* . 为位置计数器：向上对齐到 4 字节（向量表每项 4 字节） */
        KEEP(*(.isr_vector))             /* 从所有 .o 收集 .isr_vector；KEEP 防止 --gc-sections 丢弃向量表 */
        . = ALIGN(4);                    /* 段尾 4 字节对齐 */
    } > FLASH                            /* VMA 在 Flash；复位 CPU 从 0x08000000 读 MSP 与 Reset_Handler */
```

**`ALIGN(4)`**：`.` 为位置计数器；`= ALIGN(4)` 将当前链接地址向上对齐到 4 字节边界（向量表每项 4 字节，Cortex-M 要求字对齐）。

**`KEEP(*(.isr_vector))`**：从所有 `.o` 收集 startup 里 `.section .isr_vector` 定义的输入段；`KEEP` 在启用 `--gc-sections` 时仍强制保留——向量表不在普通调用链上，否则可能被链接器误删。语法详解见 [从零手写 §2.2](f103-manual-build-from-scratch.md#22-align-与-keepisr_vector-段常用)。

---

## 为什么需要？有什么作用

Cortex-M3 上电或复位后 **不会** 直接从 `main()` 开始。硬件固定执行（Flash 启动时 CPU 读逻辑 **`0x00000000` / `0x00000004`**，别名到物理 **`0x08000000` / `0x08000004`**，见 [内存映射与启动流程](stm32f103-memory-boot-map.md)）：

| 步骤 | 硬件行为 | 向量表对应项 |
|------|----------|--------------|
| 1 | 从向量表基址 **+0** 读第 1 个字 → 写入 **MSP**（主栈指针） | 索引 0：`_estack` |
| 2 | 从向量表基址 **+4** 读第 2 个字 → 写入 **PC**（程序计数器），开始执行 | 索引 1：`Reset_Handler` |

因此向量表的 **第一作用**：告诉 CPU 复位后栈在哪、代码从哪跑。没有它，CPU 既不知道栈指针，也不知道该跳转到哪里。

**第二作用**：异常或中断发生时，硬件 **自动查表跳转**。例如 HardFault 跳到 `HardFault_Handler`，SysTick 到期跳到 `SysTick_Handler`，外设中断（USART、TIM 等）跳到对应的 `XXX_IRQHandler`。软件无需在中断发生时手动 `goto`；NVIC 根据中断号算出表内偏移，把对应 `.word` 里的地址加载进 PC。

**第三作用**：提供 **默认兜底**。startup 里未单独实现的 handler 均为弱符号，最终指向 `Default_Handler` 死循环，避免跳转到随机地址：

```104:121:projects/f103-manual-reg/startup/startup_stm32f103xb.s
.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
...
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
```

在 C 中定义 `void SysTick_Handler(void) { ... }` 会 **覆盖** 弱符号，中断即进入你的函数。

### 与启动流程的关系

同一份 startup 文件还实现了 `Reset_Handler`，完整链路为：

```text
复位
  → 读向量表：设 MSP
  → PC = Reset_Handler
      → 拷贝 .data（Flash → RAM）
      → 清零 .bss
      → SystemInit()
      → main()
```

向量表负责「复位后第一步去哪」；`Reset_Handler` 负责「C 运行环境就绪前的汇编初始化」。详见 [裸机笔记 Q11](stm32-bare-metal-bootstrap.md#q11systeminit-是怎么调用的)。

### 本仓库向量表的范围

当前 f103-manual-reg **仅列出 Cortex-M3 内核的 16 个系统异常**（索引 0–15）。完整 ST 官方 startup 还会在索引 16 之后继续放置 **片上外设中断**（如 `TIM2_IRQHandler`、`USART1_IRQHandler`），数量由芯片型号决定。f103-manual-reg 只做 LED 闪烁、暂不使用外设中断，精简表已够用；一旦启用 USART 或定时器中断，需补全对应 IRQ 项并实现 handler。见 [CMSIS 概述 — startup 实例对照](cmsis-overview.md#42-本仓库实例对照)。

---

## NVIC 是什么

**NVIC** 全称 **Nested Vectored Interrupt Controller（嵌套向量中断控制器）**，是 **Cortex-M 内核内置的硬件模块**，不是 STM32 等芯片厂商在 `0x40000000` 外设总线上实现的那种 GPIO/RCC 类外设。NVIC 的 control/status 寄存器位于 ARM **PPB（Private Peripheral Bus）** 区 **`0xE000xxxx`**，与 ST 外设地址段不同；SoC 分层见 [memory-boot-map §2.1](stm32f103-memory-boot-map.md#21-soc-分层cpu-内核-vs-st-外设-vs-ppb)、[MMIO 基础 §7](stm32f103-mmio-basics.md#7-ppb-与-st-外设与-mmio-的关系)。

简单理解：

- 中断向量表是存在 Flash 里的「中断服务函数地址对照表」
- NVIC 是负责查表、判优先级、控制跳转、处理嵌套的硬件执行者

startup 里手写的 `g_pfnVectors`，本质上就是给 NVIC 使用的地址索引表。

---

## NVIC 的核心功能

NVIC 统一管理所有内核异常和外部外设中断，是中断能快速、有序、可嵌套响应的硬件基础。

### 1. 统一管理所有中断源

Cortex-M 体系里，中断源分为两类，全部由 NVIC 管控：

| 类型 | 说明 | 向量表位置 |
|------|------|------------|
| **内核异常**（前 16 个） | 复位、NMI、HardFault、SVC、PendSV、SysTick 等；编号固定，各 Cortex-M 芯片一致 | 索引 0–15 |
| **外部中断（IRQ）** | ST 接入的片上外设中断，如 GPIO、USART、SPI、DMA、定时器；数量与编号由厂商定义 | 索引 16 起 |

这对应 startup 向量表的结构：前 16 项为内核异常，完整版后面跟 STM32 外设中断。

### 2. 向量式快速响应（Vectored）

这是 NVIC 最核心的特性，也是「向量中断」名称的来源：

- 中断触发时，NVIC 根据中断号自动计算偏移，从向量表中取出对应服务函数的入口地址
- 硬件自动完成 CPU 现场压栈、跳转执行，无需软件逐个查询中断源
- 向量表须按固定顺序排列、放在固定基地址（默认 Flash 起始）——NVIC 硬件按偏移查表

### 3. 中断嵌套与优先级（Nested）

- 支持可编程优先级；高优先级中断可打断正在执行的低优先级中断，实现嵌套
- Cortex-M3 提供 8 位优先级配置位；STM32 通常只使用高 4 位，可划分为 **抢占优先级** 与 **子优先级**
- 优先级仲裁由 NVIC 硬件完成，嵌套逻辑不会乱

### 4. 基础控制能力

每个中断有独立控制位，由 NVIC 寄存器管理：

- **使能/除能**：单独开关某中断；关闭后即使外设触发信号，CPU 也不响应
- **挂起/解挂**：暂时无法响应时标记为挂起，条件满足后再执行；也支持软件手动触发
- **活跃状态**：记录当前正在执行的中断，用于嵌套判断与返回

### 5. 向量表重定位（VTOR）

NVIC 配合 **系统控制块 SCB** 中的 **向量表偏移寄存器（VTOR）** 可修改向量表基地址，不必固定在 Flash 起始。典型场景：

- Bootloader 跳转到应用程序时，切换到应用的向量表
- 将向量表搬到 RAM，实现运行时动态修改中断入口

---

## 向量表与 NVIC 的协作关系

二者是 **「地址索引表」** 与 **「硬件执行器」** 的配套关系：

```mermaid
flowchart LR
  SRC[外设或内核产生中断]
  NVIC[NVIC 检查使能与优先级]
  VT[向量表 g_pfnVectors]
  ISR[中断服务函数]
  RET[出栈恢复现场]

  SRC --> NVIC
  NVIC -->|允许响应| VT
  VT -->|读取入口地址| ISR
  ISR --> RET
```

完整链路：

1. 外设或内核产生中断信号，送到 NVIC
2. NVIC 检查该中断是否使能、优先级是否足够
3. 若允许响应，NVIC 根据 **中断号** 计算偏移，从「向量表基地址 + 偏移」读取服务函数地址
4. NVIC 硬件自动压栈保存现场，跳转到该函数执行
5. 服务函数返回后，硬件出栈恢复现场，回到被打断处继续执行

芯片复位后，CPU 从向量表基址（默认逻辑 `0x00000000`，Flash 启动时别名到物理 `0x08000000`）读表：第一项为初始栈顶，第二项为 `Reset_Handler` 地址——这也是向量表必须放在 Main Flash 最开头的根本原因。详见 [内存映射与启动流程](stm32f103-memory-boot-map.md)。

---

## 与 CMSIS 的关系

NVIC 属于 Cortex-M 内核外设，其寄存器定义与标准操作函数归属 **CMSIS-Core**。分层说明见 [CMSIS 标准与手写裸机边界](cmsis-overview.md)。

1. **寄存器定义在 CMSIS-Core 头文件中**  
   例如 `core_cm3.h` 定义 NVIC 寄存器结构体与位域掩码，逻辑与 GPIO、RCC 等外设寄存器定义一致。

2. **CMSIS 封装标准操作函数**（各 Cortex-M 芯片通用）：
   - `NVIC_EnableIRQ(IRQn_Type IRQn)` — 使能指定中断
   - `NVIC_DisableIRQ(IRQn_Type IRQn)` — 关闭指定中断
   - `NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority)` — 设置优先级
   - `NVIC_SystemReset(void)` — 触发系统复位
   - `SCB->VTOR` — 向量表偏移（SCB 与 NVIC 配合）

3. **与纯寄存器开发的关系**  
   本仓库 f103-manual-reg **不链接** CMSIS 头文件，但 startup / `SystemInit` 遵循向量表顺序与 Reset 流程等 CMSIS 规范。需要中断时可不调用 CMSIS 函数，直接操作 NVIC 寄存器（如 `NVIC->ISER`、`NVIC->IP`），原理与手写 RCC、GPIO 相同；CMSIS 只是把寄存器操作封装成通用接口。

---

## 与 STM32 HAL 的关系

- HAL 的中断管理底层调用 CMSIS 的 NVIC 标准函数
- CubeMX 里配置的中断优先级、是否使能，最终转化为对 NVIC 寄存器的配置
- 外设中断信号只是「输入端」；能否响应、优先级多少，由 NVIC 决定

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| Reset → SystemInit → main 调用链 | [stm32-bare-metal-bootstrap.md](stm32-bare-metal-bootstrap.md) |
| 向量表在链接/map 中的位置 | [f103-module-build-flow.md](f103-module-build-flow.md) |
| CMSIS 分层与手写边界 | [cmsis-overview.md](cmsis-overview.md) |
| Flash / RAM 内存映射 | [memory-map-medium-density.md](../reference/stm32f103/md/topics/memory-map-medium-density.md) |
| 启动重映射、BOOT、完整加载流程 | [stm32f103-memory-boot-map.md](stm32f103-memory-boot-map.md) |
| MMIO、PPB vs ST 外设 | [stm32f103-mmio-basics.md](stm32f103-mmio-basics.md) |
| RM0008 NVIC 章节 | [rm0008-index.md](../reference/stm32f103/md/rm0008-index.md)（§9 Nested vectored interrupt controller） |
| 本模块 startup 源码 | [`projects/f103-manual-reg/startup/startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) |
