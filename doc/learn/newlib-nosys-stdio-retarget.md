# 裸机 newlib、nosys 与串口输出

整理自 embed-dev-lab 开发问答：STM32F103 **无操作系统** 裸机上，`printf` 与 `HAL_UART_Transmit` 的关系、`--specs=nosys.specs`、以及为何需要（或不必）自写 `syscalls.c`。

工具链配置见 [`cmake/toolchain-arm-none-eabi.cmake`](../../cmake/toolchain-arm-none-eabi.cmake)（`--specs=nosys.specs -nostartfiles`）。

---

## 1. 两层分工：HAL 管硬件，libc 管格式化

| 层 | 谁提供 | 做什么 |
|----|--------|--------|
| **libc（newlib）** | 工具链 `libc.a` | `printf` 格式化字符串；内部经 `_write_r` 调 `_write` |
| **HAL / 寄存器** | ST HAL 或手写 MMIO | 初始化 USART、发/收字节（`HAL_UART_Transmit` / 写 `USART1_DR`） |
| **`_write`（可选）** | 工程 `syscalls.c` | 把 libc 的 stdout **接到** 具体外设 |

**HAL 不提供 `printf`，也不会自动把 stdout 接到 UART。** 它只解决「怎么驱动 USART」；「C 库的输出往哪发」须工程接好，或 bypass libc 直接调 HAL。

```text
【路径 A：printf】
printf → vfprintf → _write_r → _write（syscalls.c）→ HAL / USART1_Write → PA9

【路径 B：不用 printf】
USART1_WriteStr → HAL_UART_Transmit → PA9
```

---

## 2. 为什么标准 C 库没有「能用的」`_write`

newlib 要跑在 Linux、RTOS、semihosting、各种 UART 引脚上，**不可能**在 libc 里写死：

- USART1 还是 USART2？
- PA9 还是重映射后的 PB6？
- 115200 还是 1500000？

因此 libc 只负责算好要发的字节；**字节从哪个引脚、什么波特率出去** 属于板级支持（BSP），不是 C 标准的一部分。

本仓库 `--specs=nosys.specs` 会链 **libnosys.a**，其中 `_write` 等为**占位桩**（多数情况等于不输出），目的是 **能链接通过**，不是 bug。

---

## 3. `--specs=nosys.specs` 与「必须写 syscalls 吗」

| 含义 | 说明 |
|------|------|
| **no sys** | 按「无操作系统」链接 |
| **libnosys.a** | 提供 `_write`、`_read`、`_sbrk` 等默认空实现 |
| **裸机 F103 demo** | 与无 OS 模型一致，本仓库默认使用该 specs |

**只有在使用 `printf` / stdout 时**，才 practically 需要自写 **`_write`**（文件名惯例叫 `syscalls.c`，也可叫 `retarget.c`）。

| 场景 | 是否需要 `_write` |
|------|-------------------|
| 使用 `printf` | **需要**（否则走 libnosys 空桩，无串口输出） |
| 只用 `HAL_UART_Transmit` / `USART1_WriteStr` | **不需要** |
| 不用任何 libc I/O | 不必写；libnosys 占位即可链接 |

未在工程里实现的 syscall，仍用 libnosys 默认桩；**你提供的同名强符号会替换** libnosys 里对应成员。

其他裸机选项（本仓库未用）：semihosting specs（经调试器，慢）、RTOS 自带 retarget、CubeMX 生成的 `syscalls.c` 等。

### 常见误解：用了 nosys 就必须写 syscalls.c？

**半对半错。** 更精确的说法：

| 说法 | 对错 |
|------|------|
| F103 裸机 demo 没有 OS | ✓ |
| 本仓库常用 `--specs=nosys.specs` | ✓（与无 OS 链接模型一致） |
| 用了 nosys **就必须**写完整 `syscalls.c` | ✗ |
| 要用 **`printf` 打到串口** | 须自写 **`_write`**（文件名可叫 `retarget.c`） |
| 只用 **`HAL_UART_Transmit`** | **不必**写 `_write`；libnosys 占位即可链接 |

可压缩为三句：

1. 裸机无 OS，stdout 没有现成设备。
2. `nosys.specs` 告诉工具链按无 OS 链接，libnosys 提供空桩。
3. 只有当你用 libc 的 `printf` 时，才需要 `_write` 把 stdout 接到 UART；本仓库 **cmsis-hal 选 bypass**，**manual-reg 选 printf**。

---

## 4. `_write` 是 weak 覆盖吗

**不完全是。** 更准确说是 **链接期用工程里的强符号替换 libnosys 桩**：

1. `syscalls.c.obj` 定义 `_write`（`nm` 为 **T** 强符号，不是 **W** weak）
2. libc 的 `_write_r` 已解析到该符号
3. 链接器**不再**从 libnosys.a 拉入同名 `_write`

不是 C 语言 `__attribute__((weak))` 那种「强符号覆盖弱符号」；而是 **同名符号已由 .obj 满足，archive 成员不被选中**。

验证（示例）：

```bash
arm-none-eabi-nm projects/f103-manual-reg/build/f103-manual-reg.elf | grep _write
# _write 地址应对应 syscalls.c.obj，而非 libnosys
```

---

## 5. `printf` 与 `HAL_UART_Transmit` 如何选

| | `printf` + `_write` | `HAL_UART_Transmit` / `WriteStr` |
|---|----------------------|----------------------------------|
| **Flash** | 纯字符串常优化为 `puts`，Debug 约 **8 KB** text；带格式符的 `printf` 接近 **30 KB** | 约 **6 KB** 量级（本仓库 HAL demo 实测） |
| **依赖** | `syscalls.c`、常需 `_sbrk`、链接脚本 `end` | 仅 HAL UART |
| **格式化** | 支持 `%d` `%x` 等 | 仅字符串；要格式可先 `snprintf` 再发 |
| **适用** | 学习 newlib 重定向、调试信息多 | HAL 最小 demo、体积敏感 |

**本仓库约定：**

| 工程 | 串口输出方式 | 说明 |
|------|--------------|------|
| [`f103-manual-reg`](../../projects/f103-manual-reg/) | `printf` → [`syscalls.c`](../../projects/f103-manual-reg/src/syscalls.c) → [`USART1_Write`](../../projects/f103-manual-reg/src/usart.c) | 演示 newlib 重定向 + 手写寄存器 |
| [`f103-cmsis-hal`](../../projects/f103-cmsis-hal/) | [`USART1_WriteStr`](../../projects/f103-cmsis-hal/src/usart.c) → `HAL_UART_Transmit` | **无** `syscalls.c`、不链 libc I/O |

若 HAL 工程日后需要 `printf`，可复制 manual-reg 的 `syscalls.c` 思路，在 `_write` 内调 `HAL_UART_Transmit`。

---

## 6. 换行（Windows 串口助手）

C 字符串 `\n`（LF）在 Windows 串口终端上往往只换行不回列首，输出会「阶梯状」右移。

两工程均在发送层补 `\r`：

- manual-reg：[`syscalls.c`](../../projects/f103-manual-reg/src/syscalls.c) 的 `_write`
- cmsis-hal：[`usart.c`](../../projects/f103-cmsis-hal/src/usart.c) 的 `USART1_WriteStr`

应用层字符串只需写 `\n`。

---

## 7. 延伸阅读

| 文档 | 内容 |
|------|------|
| [f103-manual-reg § printf](../projects/f103-manual-reg.md#printf-与-newlib-syscall) | 本仓库 printf 路径与堆 |
| [f103-cmsis-hal § USART1](../projects/f103-cmsis-hal.md#usart1-串口输出) | HAL 直发、不用 printf |
| [f103-module-build-flow § map](../learn/f103-module-build-flow.md#32-f103-manual-regmap-精读链接顺序实证) | libnosys / libc 链接顺序 |
| [CMSIS 与手写边界](cmsis-overview.md) | 为何不链 CMSIS 仍可用 newlib |
