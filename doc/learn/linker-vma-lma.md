# 链接脚本 VMA 与 LMA

整理自 embed-dev-lab 开发过程中的问答，说明 GNU ld 链接脚本中 **VMA**（运行地址）与 **LMA**（加载/烧录地址）在 STM32F103C8 裸机工程中的含义、语法与 startup 协作。与本仓库 [`STM32F103C8_FLASH.ld`](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld)、[`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) 逐行对应。

相关文档：[内存映射与启动流程](stm32f103-memory-boot-map.md) · [链接器 Map 文件](linker-map-file.md) · [从零手写 §2.1](f103-manual-build-from-scratch.md#21-gnu-ld-脚本结构entry--memory--sections)

---

## 核心结论

| 术语 | 含义（裸机语境） |
|------|------------------|
| **VMA**（Virtual Memory Address） | **运行时地址**：CPU 取指、读写变量时访问的地址。无 MMU 时即物理地址，「Virtual」为历史沿用名 |
| **LMA**（Load Memory Address） | **加载/烧录地址**：固件写入 Flash 时，该段数据实际存放的物理地址 |

本质区别：**VMA = 运行时在哪；LMA = 烧录时存在哪**。

Flash（只读、非易失）与 RAM（读写、易失）是两块独立地址空间，才会出现 VMA ≠ LMA——嵌入式链接最核心的场景是 **`.data` 段**：初值存 Flash（LMA），运行时在 RAM（VMA），上电由 startup 从 LMA 拷贝到 VMA。

---

## 1. STM32 典型段的 VMA / LMA

| 段 | 作用 | VMA | LMA | 相等？ | 原因 |
|----|------|-----|-----|--------|------|
| `.isr_vector` | 中断向量表 | Flash `0x0800xxxx` | Flash | 是 | 复位硬件从 Flash 起始读 MSP/PC，只读、不搬运 |
| `.text` / `.rodata` | 代码与只读常量 | Flash | Flash | 是 | XIP 原地取指/读常量 |
| `.data` | 已初始化全局/静态变量 | **RAM** `0x2000xxxx` | **Flash**（紧跟 `.text` 镜像后） | **否** | 运行需 RAM 可写；初值须固化在 Flash |
| `.bss` | 未初始化全局/静态变量 | RAM | 无（不占 Flash） | — | 初值恒为 0，startup 清零即可 |

### 链接脚本符号与地址类型

| 符号 | 地址类型 | 含义 |
|------|----------|------|
| `_sidata` | **LMA** | `.data` 初值在 Flash 中的起始地址（`LOADADDR(.data)`） |
| `_sdata` / `_edata` | **VMA** | `.data` 在 RAM 中的起止地址 |
| `_sbss` / `_ebss` | **VMA** | `.bss` 在 RAM 中的起止地址 |
| `_etext` | **VMA**（= LMA） | `.text` 在 Flash 中的结束地址 |
| `.`（位置计数器） | **VMA** | 当前输出段内的链接地址指针 |

---

## 2. ld 语法：`> 区域` 与 `AT > 区域`

```text
MEMORY   → 定义 VMA 的分配「地址池」（Flash / RAM）
SECTIONS → 逐个输出段指定归属
  > FLASH / > RAM     → 指定 VMA 所在区域
  AT > FLASH          → 单独指定 LMA 所在区域（不写则 LMA 默认 = VMA）
  LOADADDR(.data)     → 取出 .data 的 LMA 起始地址
```

本仓库 `.data` 写法：

```ld
_sidata = LOADADDR(.data);
.data : { _sdata = .; ... _edata = .; } > RAM AT > FLASH
```

`AT > FLASH` 由链接器自动把 `.data` 镜像排在 Flash 中 `.text` 之后，比手写 `AT(_etext)` 更不易因对齐/间隙算错。

---

## 3. 逐段对照 `STM32F103C8_FLASH.ld`

### 3.1 MEMORY — VMA 的容器

```24:29:projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}
```

所有段的 VMA 从上述两池分配；LMA 可跟随 VMA，也可经 `AT` 跨池指定。

### 3.2 `.isr_vector` — VMA = LMA，均在 Flash

- `> FLASH`，无 `AT` → LMA = VMA，固定 `0x08000000`
- NVIC 硬件复位时从此读向量表，不可搬运

### 3.3 `.text` — VMA = LMA，均在 Flash

- 函数机器码与 `.rodata` 原地执行/读取（XIP）
- `_etext`：`.text` 结束；因 VMA=LMA，亦等于 Flash 中代码区结束位置

### 3.4 `.data` — VMA 在 RAM，LMA 在 Flash（唯一分离段）

- `> RAM`：`_sdata` / `_edata` 记录 **VMA**
- `AT > FLASH` + `_sidata = LOADADDR(.data)`：记录 **LMA**
- startup 拷贝循环：源 `_sidata`（Flash）→ 目标 `_sdata`～`_edata`（RAM）

### 3.5 `.bss` — 仅 VMA，无 LMA

- `> RAM`，无 `AT`，不占 Flash 烧录空间
- startup 将 `_sbss`～`_ebss` 清零

---

## 4. startup 中的 VMA / LMA 操作

### 4.1 `.data` 拷贝：LMA → VMA

```61:76:projects/f103-manual-reg/startup/startup_stm32f103xb.s
    /* .data：LMA（Flash/_sidata）→ VMA（RAM/_sdata.._edata）搬运，见 doc/learn/linker-vma-lma.md §4.1 */
    ldr r0, =_sdata                      /* 拷贝目标：.data 在 RAM 的 VMA 起始 */
    ldr r1, =_edata                      /* 拷贝目标：.data 在 RAM 的 VMA 结束（不含） */
    ldr r2, =_sidata                     /* 拷贝源：.data 初值在 Flash 的 LMA 起始 */
    movs r3, #0
    b LoopCopyDataInit
    ...
```

### 4.2 `.bss` 清零：初始化 VMA

```78:90:projects/f103-manual-reg/startup/startup_stm32f103xb.s
    /* .bss：仅 VMA，无 LMA；将 _sbss.._ebss 清零，见 doc/learn/linker-vma-lma.md §4.2 */
    ldr r2, =_sbss                       /* .bss 在 RAM 的 VMA 起始 */
    ldr r4, =_ebss                       /* .bss 在 RAM 的 VMA 结束（不含） */
    ...
```

---

## 5. 为何必须区分 VMA 与 LMA

1. **RAM 易失**：全局变量初值不能只放 RAM，须固化到 Flash（LMA）。
2. **Flash 只读**：运行时变量需读写，须落在 RAM（VMA）。
3. **分工**：初值存 Flash，运行用 RAM，**startup 负责搬运**——C 代码执行前 `Reset_Handler` 必须完成 `.data` 拷贝与 `.bss` 清零。

---

## 6. 用 map 文件验证

编译后查看 `projects/f103-manual-reg/build/f103-manual-reg.map` 的 **`Linker script and memory map`** 一节（须先 `./scripts/build.sh f103-manual-reg build`）：

| 段 | 预期 |
|----|------|
| `.isr_vector`、`.text` | VMA 与 LMA 数值相同，均在 `0x0800xxxx` |
| `.data` | VMA 在 `0x2000xxxx`，LMA 在 `0x0800xxxx` |
| `.bss` | 仅有 VMA，不占用 Flash 大小 |

详见 [linker-map-file.md](linker-map-file.md)。

---

## 7. 本工程内存模型一句话

- **代码 + 只读常量**：VMA = LMA，都在 Flash，XIP 执行。
- **已初始化全局变量**：VMA 在 RAM，LMA 在 Flash，上电 startup 拷贝。
- **未初始化全局变量**：仅 VMA 在 RAM，上电 startup 清零。

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| BOOT、Flash/SRAM 布局图 | [stm32f103-memory-boot-map.md §6](stm32f103-memory-boot-map.md#6-f103c8-main-flash-链接布局f103-manual-reg) |
| CMake 链接与 map 精读 | [f103-module-build-flow.md §3.2](f103-module-build-flow.md#32-目标文件链接顺序) |
| 向量表与复位加载 | [interrupt-vector-table-and-nvic.md](interrupt-vector-table-and-nvic.md) |
| 启动流程 Q&A | [stm32-bare-metal-bootstrap.md](stm32-bare-metal-bootstrap.md) |
