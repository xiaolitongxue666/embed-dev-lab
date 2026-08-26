# f103-manual-reg 模块编译流程

整理自 embed-dev-lab 开发过程中的问答，说明 `startup/` 汇编与 `src/` C 文件如何通过 CMake 与链接脚本合并为单一固件。运行时启动语义见 [STM32 裸机启动与时钟](stm32-bare-metal-bootstrap.md) Q5/Q11/Q12。

`f103-cmsis-hal` **共用**同一 `./scripts/build.sh` / `embed_mcu_add_executable()` 框架；日常命令见 [编写 → 编译 → 下载](../workflow-write-build-flash.md)。

## 1. 端到端流程总览

日常构建入口：

```bash
./scripts/build.sh f103-manual-reg          # configure + build（默认 all）
./scripts/build.sh f103-manual-reg build      # 仅编译
```

```mermaid
flowchart LR
  buildSh["build.sh f103-manual-reg"]
  preset["CMakePresets debug"]
  toolchain["toolchain-arm-none-eabi.cmake"]
  ninja["Ninja compile+link"]
  elf["f103-manual-reg.elf"]
  hex["f103-manual-reg.hex"]
  buildSh --> preset --> toolchain --> ninja --> elf
  elf -->|"POST_BUILD objcopy"| hex
```

| 阶段 | 位置 | 行为 |
|------|------|------|
| configure | [`scripts/build.sh`](../../scripts/build.sh) → `do_configure` | 在 `projects/f103-manual-reg/` 执行 `cmake --preset debug`，生成 `build/` |
| toolchain | [`cmake/toolchain-arm-none-eabi.cmake`](../../cmake/toolchain-arm-none-eabi.cmake) | 交叉编译：`CMAKE_SYSTEM_NAME Generic`，`arm-none-eabi-gcc` 兼作 C/ASM 编译器 |
| build | `cmake --build --preset debug` | Ninja 编译各源文件为 `.o`，再链接为 `f103-manual-reg.elf` |
| POST_BUILD | [`cmake/mcu-config.cmake`](../../cmake/mcu-config.cmake) | `arm-none-eabi-objcopy -O ihex` 生成 `f103-manual-reg.hex` |

**裸机链接关键**：工具链设 `--specs=nosys.specs -nostartfiles`，**不使用** gcc 自带的 `crt0` 启动文件；复位入口与 C 运行时初始化由模块内 [`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) 提供。

产物目录：`projects/f103-manual-reg/build/`

| 文件 | 说明 |
|------|------|
| `f103-manual-reg.elf` | 链接输出；probe-rs / IDE F5 烧录 |
| `f103-manual-reg.hex` | POST_BUILD 自动生成；OpenOCD 烧录 |
| `f103-manual-reg.map` | 链接 map；概念说明见 [链接器 Map 文件](linker-map-file.md)，本模块实例见 [§3.2 map 精读](#f103-manual-regmap-精读链接顺序实证) |
| `compile_commands.json` | clangd 索引；`build.sh` 同步至仓库根 |

---

## 2. CMake 入口与 Presets

### 2.1 仓库根 vs 模块目录

| 入口 | 用途 |
|------|------|
| [`CMakeLists.txt`](../../CMakeLists.txt)（仓库根） | `add_subdirectory(projects/f103-manual-reg)`，聚合子模块 |
| [`projects/f103-manual-reg/CMakePresets.json`](../../projects/f103-manual-reg/CMakePresets.json) | **日常构建**走此 Preset，不依赖根目录 configure |

[`CMakePresets.json`](../../projects/f103-manual-reg/CMakePresets.json) 要点：

- **generator**：Ninja
- **binaryDir**：`${sourceDir}/build`
- **CMAKE_TOOLCHAIN_FILE**：`../../cmake/toolchain-arm-none-eabi.cmake`
- **CMAKE_EXPORT_COMPILE_COMMANDS**：`ON`（供 clangd）

### 2.2 [`projects/f103-manual-reg/CMakeLists.txt`](../../projects/f103-manual-reg/CMakeLists.txt) 逐段解析

```cmake
# 单独打开本模块目录时，自行声明 project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(f103-manual-reg C ASM)
endif()
```

| 行/块 | 含义 |
|-------|------|
| L8–11 | 以模块为工程根打开时声明 `project`；**`ASM` 语言**使 CMake 识别 `.s` 并用汇编器编译 |
| L13 | `include(../../cmake/mcu-config.cmake)` — 引入公共函数 `embed_mcu_add_executable` |
| L15–19 | `F103_SOURCES` — **C 与汇编列在同一列表**，无特殊「合并」语法 |
| L22–27 | 调用 `embed_mcu_add_executable`，传入源文件、头文件目录、链接脚本、MCU 标志 |

完整源文件列表：

```cmake
set(F103_SOURCES
    src/main.c                  # 应用入口：GPIO 闪烁 + printf
    src/usart.c                 # USART1 纯寄存器
    src/syscalls.c              # newlib _write/_sbrk → 串口
    src/system_stm32f1xx.c      # SystemInit / 72 MHz 时钟
    startup/startup_stm32f103xb.s # 向量表、.data/.bss、跳转 main
)
```

`gpioc_bitband.h`、`usart.h`、`system_stm32f1xx.h` 为头文件，由 `#include` 引入，**不**列入 `SOURCES`。

链接 `--specs=nosys.specs` 还会拉入 **libc.a** / **libnosys.a**（printf 用）；工程内 `syscalls.c` 的 `_write` 等于链接期替换 nosys 桩，见 [f103-manual-reg § printf](../projects/f103-manual-reg.md#printf-与-newlib-syscall)。

---

## 3. `embed_mcu_add_executable` 做了什么

定义于 [`cmake/mcu-config.cmake`](../../cmake/mcu-config.cmake)，各 `projects/<name>/` 共用。

### 3.1 编译与链接两阶段

```cmake
add_executable(${target_name}.elf ${MCU_SOURCES})
target_link_options(${target_name}.elf PRIVATE
    ${MCU_FLAGS_LIST}
    -T ${MCU_LINKER_SCRIPT}
    -Wl,-Map=${target_name}.map
    -Wl,--gc-sections
)
```

| 阶段 | 工具 | 输入 | 输出 |
|------|------|------|------|
| **编译** | `arm-none-eabi-gcc -c`（C）/ 汇编器（`.s`） | 每个 `.c` / `.s` | 独立目标文件（如 `main.c.obj`） |
| **链接** | `arm-none-eabi-gcc`（链接器前端 `ld`） | 全部 `.o` + 链接脚本 | 单一 `${target_name}.elf` |

Ninja 规则示例（`build/` 内；Windows 下扩展名为 `.obj`，Linux 下多为 `.o`）：

```text
CMakeFiles/f103-manual-reg.elf.dir/src/main.c.obj
CMakeFiles/f103-manual-reg.elf.dir/src/usart.c.obj
CMakeFiles/f103-manual-reg.elf.dir/src/syscalls.c.obj
CMakeFiles/f103-manual-reg.elf.dir/src/system_stm32f1xx.c.obj
CMakeFiles/f103-manual-reg.elf.dir/startup/startup_stm32f103xb.s.obj
  → 链接 → f103-manual-reg.elf（另含 libc.a / libgcc.a / libnosys.a）
```

### 3.2 目标文件链接顺序

需区分两个概念：**链接命令行上 `.obj` 的先后顺序**，与 **Flash/RAM 中最终段布局** —— 后者不由前者单独决定。

#### 命令行顺序：来自 `F103_SOURCES` 列表

CMake `add_executable(f103-manual-reg.elf ${MCU_SOURCES})` 按 **`SOURCES` 在 `CMakeLists.txt` 中的书写顺序** 生成链接命令。当前顺序为：

```text
main.c.obj → system_stm32f1xx.c.obj → startup_stm32f103xb.s.obj
```

可在 `build/build.ninja` 链接规则与 `build/f103-manual-reg.map` 的 `LOAD` 行核对：

```text
build f103-manual-reg.elf: ... main.c.obj system_stm32f1xx.c.obj startup_stm32f103xb.s.obj
```

调整 [`CMakeLists.txt`](../../projects/f103-manual-reg/CMakeLists.txt) 中 `F103_SOURCES` 条目顺序，重新 configure 后链接命令行顺序会随之改变。

#### 内存布局顺序：由链接脚本按「段名」决定

[`linker/STM32F103C8_FLASH.ld`](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) 的 `SECTIONS` 规定**输出段**先后，与 `.obj` 在命令行上的先后**无直接对应**：

| 输出段 | 链接脚本规则 | 实际来源 |
|--------|--------------|----------|
| `.isr_vector` @ Flash 起始 | `KEEP(*(.isr_vector))` + `ALIGN(4)` | 仅 startup 含此段 → **固定 0x08000000**；`KEEP` 防 gc-sections 丢弃 |
| `.text` | `*(.text) *(.text*)` | 各 `.obj` 中带 `.text` 的输入段 |
| `.data` / `.bss` | `*(.data*)` / `*(.bss*)` | 各 C 文件的已初始化/未初始化全局变量 |

因此 **startup 虽在命令行排最后，向量表仍在 Flash 最前** —— 因为链接脚本先把所有 `.isr_vector` 输入段收进独立输出段，再处理 `.text`。向量表与 NVIC 概念见 [中断向量表与 NVIC](interrupt-vector-table-and-nvic.md)。本仓库 `f103-manual-reg.map` 片段：

```text
.isr_vector     0x08000000       0x40
 .isr_vector    0x08000000       0x40  startup_stm32f103xb.s.obj   ← 向量表

.text           0x08000040      0x250
 .text.main     0x080000bc       0x44  main.c.obj
 .text.SystemInit
                0x080001e8       0x58  system_stm32f1xx.c.obj
 .text.Reset_Handler
                0x08000240       0x50  startup_stm32f103xb.s.obj   ← 代码段在后
```

同一输出段 `.text` **内部**各函数的前后关系，才与命令行 `.obj` 顺序相关：GNU `ld` 按输入文件顺序依次合并同名输入段，故当前顺序下 `main.c.obj` 的 `.text` 在前、`startup.s.obj` 的 `Reset_Handler` 在后。**这不影响复位与跳转**，因入口由 `ENTRY(Reset_Handler)` 与向量表第二项地址决定，而非 `.text` 段内谁先谁后。

#### 符号解析与顺序无关

`bl SystemInit`、`bl main` 等跨文件调用在链接阶段通过**全局符号表**解析，**不依赖** `.obj` 先后顺序；只要各符号有且仅有一处定义，链接即成功。

#### 链接顺序为何与直觉不同？

常见直觉：**上电先跑 startup，所以 `startup.s.obj` 应该先链接**。这与实际机制不符 —— 混淆了**运行时启动顺序**与**构建时链接命令行顺序**。

| 直觉 | 实际 |
|------|------|
| 上电先跑 startup → 应先链接 | 上电看**向量表**里的函数指针，不看链接命令行谁先谁后 |
| `.s.obj` 应第一个出现在 map 的 `.text` 里 | 向量表已在 Flash 最前（`.isr_vector` 段）；`.text` 内谁先谁后只影响代码在 Flash 中的**排列**，不影响能否启动 |
| 链接顺序 = 执行顺序 | **执行顺序**：复位 → 向量表 → `Reset_Handler` → `SystemInit` → `main` |

当前 `main → system → startup` 仅因 [`CMakeLists.txt`](../../projects/f103-manual-reg/CMakeLists.txt) 里 `F103_SOURCES` **按应用层习惯**（先写 C 业务、后写启动）排列；CMake 不会按「谁上电先跑」自动排序。许多 ST/CMSIS 模板**习惯**把 startup 写在前面，目的是 map 里 `.text` 更易读（`Reset_Handler` 紧挨向量表），**不是为了程序能跑**。

链接器做的是**拼符号地址表**（每个符号最终在 Flash/RAM 的哪），不是排「播放列表」。两件事保证程序正确，均与命令行顺序无关：

1. **段布局** — 链接脚本把 `.isr_vector` 固定在 Flash 起始；
2. **符号解析** — `Reset_Handler`、`SystemInit`、`main` 全部链接完成后统一填地址；向量表第二项指向 `Reset_Handler` 的 Thumb 入口（如 `0x08000241`），与 `Reset_Handler` 在 `.text` 段内靠后并不矛盾。

若希望构建顺序更符合直觉，可将 `startup/...s` 挪到 `F103_SOURCES` **首位**（功能不变，仅 map 可读性变化），见下节实践建议。

#### 实践建议

| 关注点 | 是否需要调整 `F103_SOURCES` 顺序 |
|--------|----------------------------------|
| 向量表在 Flash 起始 | **否** — 靠链接脚本 `.isr_vector` 段 |
| 复位进入 `Reset_Handler` | **否** — 靠 `ENTRY()` + 向量表 |
| `SystemInit` / `main` 能否链上 | **否** — 符号解析与顺序无关 |
| 同一段内函数地址排列 / map 可读性 | 可选 — 仅影响 `.text` 内布局 |

若希望 startup 的 `.text` 紧挨向量表，可将 `startup/...s` 提前到 `F103_SOURCES` 首位，例如：

```cmake
set(F103_SOURCES
    startup/startup_stm32f103xb.s
    src/system_stm32f1xx.c
    src/main.c
)
```

重新 configure + build 后，map 中 `.text` 将变为 `Reset_Handler` 在前、`main` 在后；**对本 demo 功能无必要**。

```mermaid
flowchart TB
  subgraph cmdOrder [命令行 .obj 顺序]
    direction LR
    o1["main.c.obj"] --> o2["system.c.obj"] --> o3["startup.s.obj"]
  end
  subgraph ldScript [链接脚本 SECTIONS]
    direction TB
    sec1[".isr_vector 仅收 startup"]
    sec2[".text 按输入文件顺序合并各 .text"]
    sec3[".data / .bss"]
    sec1 --> sec2 --> sec3
  end
  cmdOrder --> ldScript
```

#### `f103-manual-reg.map` 精读（链接顺序实证）

map 文件概念、四大核心内容与通用用途见 **[链接器 Map 文件](linker-map-file.md)**。以下为 f103-manual-reg 实例。

路径：`projects/f103-manual-reg/build/f103-manual-reg.map`（须先 `build` 生成；`build/` 不入 Git）。

**① 命令行 `LOAD` 顺序（L693–697 附近，以当前 CMakeLists 为准）**

与 `F103_SOURCES` 一致（**不含** libc，libc 在后续 `START GROUP`）：

```text
LOAD .../main.c.obj
LOAD .../usart.c.obj
LOAD .../syscalls.c.obj
LOAD .../system_stm32f1xx.c.obj
LOAD .../startup/startup_stm32f103xb.s.obj
```

其后 `START GROUP … libgcc.a libc.a libnosys.a` 为 `--specs=nosys.specs` 引入的运行库。

若工程含 `syscalls.c` 并实现 `_write` / `_sbrk` 等，则这些 **强符号** 在链接期替换 libnosys.a 中的同名占位桩（非 weak 覆盖）；详见 [裸机 newlib 与串口输出](../learn/newlib-nosys-stdio-retarget.md) 与 [f103-manual-reg § printf](../projects/f103-manual-reg.md#printf-与-newlib-syscall)。**f103-cmsis-hal 无 syscalls.c**，串口走 `HAL_UART_Transmit`。

**② `Discarded input sections`**

因 `-ffunction-sections` 把每个函数拆成独立输入段（如 `.text.main`），各 `.obj` 里**汇总的** `.text` 段为空（size `0x0`），map 开头显示为「丢弃的空段」——属正常现象。

**③ 输出段布局（含 printf 后变化）**

引入 `printf` 后，**libc.a** 占 Flash 主体；工程 `.obj` 中可见例如：

| 符号 / 函数 | 来源 |
|-------------|------|
| `main` / `GPIOC_Init` / `delay` | main.c.obj |
| `USART1_Init` / `USART1_Write` | usart.c.obj |
| `_write` / `_sbrk` | syscalls.c.obj |
| `SystemInit` | system_stm32f1xx.c.obj |
| `Reset_Handler` | startup.obj |
| `_write_r` / `vfprintf` 等 | libc.a |

`.isr_vector` 仍在 Flash `0x08000000`（链接脚本段名决定，与 `LOAD` 顺序无关）。当前 Debug 构建：纯字符串 `printf` 常被优化为 `puts`，`.text` 约 **8 KB**（`arm-none-eabi-size`）；带格式符的 `printf` 才接近 **30 KB**。仅 LED、无 libc 时约 **0.6 KB** 量级——**勿**用旧 map 中的固定地址做绝对对照，以当前 `f103-manual-reg.map` 为准。

**④ 向量表与 `Reset_Handler`**

向量表在 `0x08000000`，第二项为复位向量，汇编里写 `.word Reset_Handler`；链接后该字为 **`Reset_Handler` 的 Thumb 入口地址**（`0x08000241`，LSB=1 表示 Thumb）。`Reset_Handler` 函数体在 `.text` 段 `0x08000240` —— 虽在 Flash 布局上位于所有 C 函数**之后**，复位硬件只读向量表中的指针，**不**要求 `Reset_Handler` 紧挨向量表。

```text
0x08000000  g_pfnVectors[0] = _estack = 0x20005000
0x08000004  g_pfnVectors[1] = Reset_Handler+1  →  0x08000241
...
0x08000240  Reset_Handler 函数代码（.text 段靠后）
```

**⑤ RAM 段**

含 `printf` 时 libc 带来 `.data` / `.bss`（如 `stdout`、堆变量）；链接脚本在 `.bss` 后导出 `end`/`_end`（与 `_ebss` 同址）供 `_sbrk`。主栈顶仍为 `_estack = 0x20005000`。

**⑥ 调试段**

`.debug_info`、`.debug_line` 等同样按 **main → system → startup** 顺序排列，与 `LOAD` 一致；这些段**不烧进 Flash**，仅供 GDB/clangd 使用。

**⑦ 小结对照**

```text
CMake SOURCES 顺序:  main → usart → syscalls → system → startup
LOAD 行:              ✓ 同上（libc 在 GROUP 之后）
.isr_vector:          仅 startup（链接脚本段名决定）
工程 .text 符号顺序:  大致跟随 LOAD；libc 符号占 Flash 大部分
```

查看 map 时优先看 **`Linker script and memory map`** 一节；`Discarded input sections` 可忽略。

### 3.3 其他构建选项

| 选项 | 作用 |
|------|------|
| `-ffunction-sections -fdata-sections` | 每个函数/变量独立段 |
| `-Wl,--gc-sections` | 链接时丢弃未被引用的段 |
| Debug：`-O0 -g3` | 调试符号 |
| Release：`-Os` | 体积优化 |
| POST_BUILD `objcopy -O ihex` | 生成 `.hex`（OpenOCD 用） |

---

## 4. startup 与 src 如何「编到一起」

**结论**：不是汇编 `#include` C，也不是 CMake 做特殊合并 —— 是 **分别编译为目标文件 → 链接器按链接脚本布局拼成一个 ELF**。

```mermaid
flowchart TB
  subgraph compile [Compile 各自独立]
    mainC["main.c"] --> mainO["main.c.obj"]
    sysC["system_stm32f1xx.c"] --> sysO["system_stm32f1xx.c.obj"]
    startupS["startup_stm32f103xb.s"] --> startupO["startup_stm32f103xb.s.obj"]
  end
  subgraph link [Link 一次完成]
    ldScript["STM32F103C8_FLASH.ld"]
    mainO --> elf["f103-manual-reg.elf"]
    sysO --> elf
    startupO --> elf
    ldScript --> elf
  end
```

### 4.1 链接脚本与 startup 协作

[`linker/STM32F103C8_FLASH.ld`](../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) 用 GNU ld 的 `MEMORY` 定义 Flash/RAM 边界，用 `SECTIONS` 规定输出段布局与段符号；[`startup_stm32f103xb.s`](../../projects/f103-manual-reg/startup/startup_stm32f103xb.s) 在 `Reset_Handler` 中使用这些符号。VMA/LMA 逐段说明见 **[linker-vma-lma.md](linker-vma-lma.md)**；`SECTIONS`/`ALIGN`/`KEEP` 见 [从零手写 §2.1–§2.2](f103-manual-build-from-scratch.md#21-gnu-ld-脚本结构entry--memory--sections)。

链接脚本使用 Main Flash **物理**基址 `0x08000000`；复位时 CPU 经逻辑 `0x00000000` 别名读到同一向量表（BOOT 与 memory map 见 [内存映射与启动流程](stm32f103-memory-boot-map.md)）。

| 链接脚本 | startup 使用 |
|----------|--------------|
| `.isr_vector` → Flash 起始 | `g_pfnVectors` 置于 `.section .isr_vector` |
| `_estack = 0x20005000` | `ldr r0, =_estack` 设主栈 |
| `_sidata` / `_sdata` / `_edata` | 将 `.data` 从 Flash（LMA）拷贝到 RAM（VMA） |
| `_sbss` / `_ebss` | 清零 `.bss` |
| `ENTRY(Reset_Handler)` | 芯片复位后从此入口执行 |

向量表固定在 Flash 起始，与 startup 在链接命令行上是否排最后无关 —— 见 [§3.2 目标文件链接顺序](#32-目标文件链接顺序)。

Flash 布局（简化）：

```text
0x08000000  .isr_vector   ← g_pfnVectors（startup）
            .text          ← Reset_Handler、SystemInit、main、delay 等
            .rodata        ← 只读常量
            (.data 的 LMA) ← 初始化数据的 Flash 镜像
0x20000000  .data          ← 已初始化全局变量（RAM）
            .bss           ← 未初始化全局变量（RAM）
0x20005000  _estack        ← 栈顶（满递减：push 时 SP 减小，向 .bss 方向增长）
```

栈方向说明见 [memory-boot-map §6.1](stm32f103-memory-boot-map.md#61-主栈满递减与-_estack)。

### 4.2 跨文件符号解析

链接阶段解析 startup 中的 `bl` 目标：

| startup 引用 | 定义位置 |
|--------------|----------|
| `bl SystemInit` | [`src/system_stm32f1xx.c`](../../projects/f103-manual-reg/src/system_stm32f1xx.c) |
| `bl main` | [`src/main.c`](../../projects/f103-manual-reg/src/main.c) |

C 中的 `delay()`、`GPIOC_Init()` 等进入 `.text`，与 `Reset_Handler` 同处 Flash，由链接器统一排布地址；无需 startup 显式调用。

---

## 5. 产物与烧录

| 命令 | 使用文件 | 说明 |
|------|----------|------|
| `./scripts/build.sh f103-manual-reg flash` | `.elf` | probe-rs `--binary-format elf` |
| `./scripts/build.sh f103-manual-reg flash-openocd` | `.hex` | 须先 `build` |
| IDE **F103 Probe-rs Debug**（F5） | `.elf` | 见 [ide-debug.md](../ide-debug.md) |

`flash` **不会**自动编译；改源码后须先 `build`。详见 [f103-manual-reg 模块说明](../projects/f103-manual-reg.md) 与 [probe-rs.md](../probe-rs.md)。

---

## 6. 新增模块参考

0. **从零手写**：按文件依赖顺序新建，见 [从零手写构建指南](f103-manual-build-from-scratch.md)
1. 复制 [`projects/f103-manual-reg/`](../../projects/f103-manual-reg/) 目录结构（`CMakeLists.txt`、`CMakePresets.json`、`startup/`、`linker/`、`src/`）
2. 在 `CMakeLists.txt` 中调整 `F103_SOURCES` 与 `LINKER_SCRIPT`，调用 `embed_mcu_add_executable()`
3. `./scripts/build.sh <新模块名>`

---

## 延伸阅读

- [链接器 Map 文件](linker-map-file.md) — map 是什么、核心内容、六大用途、生成方式
- [STM32F103 内存映射与启动流程](stm32f103-memory-boot-map.md) — BOOT 重映射、Flash 物理地址 vs 复位别名、启动加载
- [STM32 裸机启动与时钟](stm32-bare-metal-bootstrap.md) — Reset_Handler、`SystemInit` 运行时行为（Q5/Q11/Q12/Q13）
- [f103-manual-reg 模块说明](../projects/f103-manual-reg.md) — 硬件要点、PC13、构建命令
- [从零手写构建指南](f103-manual-build-from-scratch.md) — 文件编写顺序、CMSIS 对照、验收清单
- [脚本参考](../scripts-reference.md) — `build.sh` action 与自动化链路
- [CMSIS 标准与手写裸机边界](cmsis-overview.md) — startup 规范兼容判定
