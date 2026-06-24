# 链接器 Map 文件

整理自嵌入式开发中的 map 文件概念与用法。本仓库 [`f103-manual-reg`](../../projects/f103-manual-reg/) 的实例解读见 [f103-manual-reg 编译流程 §3.2](f103-module-build-flow.md#32-目标文件链接顺序)（含 `f103-manual-reg.map` 链接顺序精读）。

---

## 一、map 文件是什么

**map 文件（映射文件）是链接器（Linker）在生成最终可执行程序（`.elf` / `.axf`）的同时，输出的一份纯文本格式的内存布局报告**。

它相当于程序编译链接后的「完整内存清单」：链接器按照链接脚本，把所有目标文件（`.o` / `.obj`）、库文件的代码和数据分配到 Flash、RAM 的具体地址后，会把整个分配过程和结果全部记录在 map 文件中。它本身**不会被烧录到芯片里**，仅用于开发者分析程序。

结合本仓库手写启动文件的场景：汇编里处理的 `.data`、`.bss` 段，它们在 Flash 中的存储位置（LMA）、在 RAM 中的运行地址（VMA）、各自占用的字节数，都会在 map 文件里体现，可直接验证启动逻辑与 [`STM32F103C8_FLASH.ld`](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) 是否匹配。

**本仓库路径**（须先 `./scripts/build.sh f103-manual-reg build`）：

```text
projects/f103-manual-reg/build/f103-manual-reg.map
```

`build/` 在 `.gitignore` 中，未编译时不存在。

---

## 二、map 文件的核心内容

以 ARM GCC 生成的 STM32 工程 map 文件为例，核心包含 4 部分：

### 1. 内存区域总览

展示链接脚本 `MEMORY` 定义的所有存储区域（Flash、RAM、CCM 等）的起始地址、总大小、属性，是评估芯片资源是否够用的直观依据。各输出段的地址与大小则由 `SECTIONS` 决定，并在 map 的 **`Linker script and memory map`** 一节逐项列出；`MEMORY` / `SECTIONS` 语义见 [从零手写 §2.1](f103-manual-build-from-scratch.md#21-gnu-ld-脚本结构entry--memory--sections)。

本仓库 `f103-manual-reg.map` 示例：

```text
Name             Origin             Length             Attributes
FLASH            0x08000000         0x00010000         xr
RAM              0x20000000         0x00005000         xrw
```

### 2. 段（Section）分布详情

逐个列出各段的：起始地址、占用大小、归属存储区、来自哪个目标文件/库。map 中同一输出段常同时标注 **VMA**（运行地址）与 **LMA**（烧录地址）：`.text` 二者相同；`.data` VMA 在 RAM、LMA 在 Flash；`.bss` 仅 VMA。语义详解见 [linker-vma-lma.md](linker-vma-lma.md)。

常见段：

| 段 | 含义 | 典型位置 |
|----|------|----------|
| `.isr_vector` | 中断向量表 | Flash 起始（本仓库由链接脚本单独段） |
| `.text` | 代码（函数指令） | Flash |
| `.rodata` | 只读常量 | Flash |
| `.data` | 已初始化全局变量 | RAM 运行；Flash 存初始值（LMA） |
| `.bss` | 未初始化全局变量 | RAM；启动时清零 |
| `.stack` / `.heap` | 栈与堆 | RAM（本 demo 栈由 `_estack` 符号定义，无显式 `.stack` 段） |

阅读时优先找 **`Linker script and memory map`** 一节；开头的 `Discarded input sections` 在启用 `-ffunction-sections` 时常见，多为空汇总段，可忽略。

### 3. 符号地址明细

列出全局符号（函数、全局变量）的：

- 绝对地址
- 占用字节数
- 定义所在的源文件 / 库

例如可精确看到 `main` 位于 `0x080000bc`、占 `0x44` 字节，`SystemInit` 位于 `0x080001e8` 等。

### 4. 链接依赖与引用关系

记录输入文件的 `LOAD` 顺序、库文件（如 `libgcc.a`、`libc.a`、`libnosys.a`）的链接组、弱符号别名（如 `Default_Handler`）等。

---

## 三、核心作用（嵌入式开发场景）

map 文件是排查问题、优化程序的核心工具，常用场景如下。

### 1. 排查内存溢出，评估资源占用

编译报 `region 'FLASH' overflowed` 或 RAM 不足时，打开 map 文件：

- 判断是 `.text` 代码超了，还是 `.bss` / `.data` 变量占太多
- 定位到具体哪个文件、哪个函数/变量占用最大，针对性裁剪

对 STM32 等资源有限的 MCU，这是评估程序能否装进芯片的核心依据。

### 2. 定位崩溃地址，辅助故障排查

HardFault、跑飞等异常时常能拿到崩溃瞬间的 **PC**（程序计数器）、**LR**。

将地址对照 map 文件，可快速定位到哪个函数甚至哪段代码；无在线调试器时尤其有用。量产固件保留对应版本的 map 文件，是远程排查的标准手段。

### 3. 验证内存布局与链接脚本

自定义链接脚本或特殊内存分配时（函数放 RAM 执行、向量表位置、CCM 等），须用 map 验证：

- 指定函数/变量是否落在目标地址
- 向量表是否在 Flash 起始（本仓库 `g_pfnVectors` @ `0x08000000`）
- 段的 LMA 与 VMA 是否符合预期（如 `_sidata` / `_sdata`）

### 4. 针对性优化代码/数据体积

- 哪个库、哪个函数占 Flash 最多，可否裁剪或替换
- 哪个全局数组占 RAM 过大，可否缩小或改为局部变量

避免盲目优化，精准命中占用最大的部分。

### 5. 排查符号冲突与链接异常

- 同名全局变量最终用了哪个文件的定义
- 强符号是否覆盖库中弱符号
- 某符号从哪个库链入、是否被 `--gc-sections` 优化掉

### 6. 确认栈堆配置，规避溢出风险

- 栈顶 `_estack`、栈与 `.bss`/`.data` 的边界（满递减：`_estack` 为 RAM 上界，push 时 SP 减小；见 [memory-boot-map §6.1](stm32f103-memory-boot-map.md#61-主栈满递减与-_estack)）
- 堆大小及与栈的重叠风险（本 demo 无堆使用）

栈溢出是隐蔽故障源；map 结合实际调用深度评估栈空间是否充足。

---

## 四、生成方式

### 本仓库（ARM GCC + CMake）

[`cmake/mcu-config.cmake`](../../cmake/mcu-config.cmake) 中 `embed_mcu_add_executable` 已配置：

```cmake
target_link_options(${target_name}.elf PRIVATE
    ...
    -Wl,-Map=${target_name}.map
    ...
)
```

执行 `./scripts/build.sh f103-manual-reg build` 后，与 `.elf` 同目录生成 `f103-manual-reg.map`。新增模块沿用同一函数即可自动产出 `<module>.map`。

### 其他工具链

| 工具链 | 方式 |
|--------|------|
| **ARM GCC**（CubeMX Makefile/CMake） | 链接参数 `-Wl,-Map=output.map`；CubeMX 工程通常默认开启 |
| **Keil MDK** | 编译后自动生成，输出目录下 `.map` |
| **IAR** | 编译后自动生成 `.map` |

---

## 延伸阅读

- [f103-manual-reg 编译流程](f103-module-build-flow.md) — CMake 构建、`f103-manual-reg.map` 链接顺序与段布局精读
- [STM32 裸机启动与时钟](stm32-bare-metal-bootstrap.md) — `.data`/`.bss` 运行时语义
- [STM32F103 内存映射与启动流程](stm32f103-memory-boot-map.md) — BOOT 重映射、Flash/SRAM 分工、复位加载
- [f103-manual-reg 链接脚本](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) — `MEMORY` / `SECTIONS` 定义
- [链接脚本 VMA 与 LMA](linker-vma-lma.md) — VMA/LMA 逐段对照与 map 验证
