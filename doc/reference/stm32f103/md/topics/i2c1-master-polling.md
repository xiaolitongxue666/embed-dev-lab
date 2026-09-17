# I2C1 主机写（轮询）

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 I2C 章（CR1/CR2/CCR/TRISE/SR1/SR2/DR） |
| 源码 | [`projects/f103-manual-reg/src/periph/i2c.c`](../../../../../projects/f103-manual-reg/src/periph/i2c.c) |
| 接线 | [stm32f103-peripherals.md](../../../../hardware/stm32f103-peripherals.md) |
| 屏幕 | [sh1106](../../../sh1106/README.md)（含时钟 `00:00:00` 总线字节） |

## 主机写一帧

总线上一次写事务：

```text
START → DR=0x78（8 位写地址）→ ACK → 数据字节… → STOP
```

本模块（R6 焊、R5 空）：

- 固件 / Probe / `DR` 一律 **`0x78`**
- `0x78 == 0x3C << 1`；读地址 `0x79` 本驱动不用
- **禁止**对 `0x78` 再 `<< 1`（会变成 `0xF0`，必 NACK）
- StdPeriph `I2C_Send7bitAddress(I2C1, 0x78>>1, …)` 会先右移再由库左移，手写路径不要模仿

发给 SH1106 的两种 payload：

| 用途 | 总线上的字节（地址之后） | 完整帧 | 路径 |
|------|--------------------------|--------|------|
| 命令 | `00 cmd` | `START 78 00 cmd STOP` | `I2C1_Write` 轮询 |
| 一页 GDDRAM | `40` + 128 列 | `START 78 40 d0…d127 STOP` | 软件发 `78 40`，`d0…d127` 由 DMA1 CH6 搬 |

`00` / `40` 是控制字节（Co=0；D/C#=0 命令、=1 数据）。省略则黑屏。`40` 后 128 字节是**一页从左到右的列**（一字节一列、管内 8 个竖点），页/bit 见 [从零：页和怎么上屏](../../../sh1106/README.md#从零页和怎么上屏)。时钟上屏的逐字节例子见 [画出 00:00:00](../../../sh1106/README.md#实际例子画出电子时钟-000000)。

DMA **不**产生 START / STOP / 从地址 / `0x40`。USART 已占 CH4/CH5；I2C1_TX 用 CH6。TC 中断里关 `CR2.DMAEN`、等 `BTF` 再 `STOP`。

## 本工程约定

- 硬件 I2C1，默认映射 **PB6=SCL、PB7=SDA**；复用开漏 50 MHz（`CRL` 半字节 `0xF`）
- **不** `USART1_REMAP` / `I2C1_REMAP`
- 从机参数是 **8 位写地址**，原样写入 `DR`。SH1106 为 **`0x78`**，禁止再 `<< 1`
- 命令 / Probe：轮询 + 超时。页数据：DMA1 CH6 + TC 中断（无 I2C EV/ER IRQ）
- F1 BUSY 锁死：超时则 `PE` 关 + APB1 复位 I2C1 再恢复时序
- PCLK1 = 36 MHz：`CR2.FREQ=36`；Fast 400 kHz duty 2：`CCR=30`、`FS=1`；`TRISE=11`
- `OAR1` bit14 手册要求软件保持为 1

## 主机写顺序

1. 等 `SR2.BUSY=0`
2. `CR1.START`，等 `SR1.SB`
3. `DR = addr8`（屏幕 `0x78`）
4. 等 `SR1.ADDR`；若 `AF` 则清 `AF` 并 `STOP`
5. 读 `SR1` 再读 `SR2` 清 ADDR
6. 每字节：等 `TXE`，写 `DR`
7. 末字节等 `BTF`，`CR1.STOP`

StdPeriph 的 `I2C_Send7bitAddress(I2C1, 0x78>>1, …)` 会先右移再由库左移。手写路径不要模仿。

## 白话：谁走 DMA、中断在哪

`Refresh` 每页仍是 4 段。①②③ 是短命令，④ 才是 128 列像素：

```text
① START 78 00 B0+页  STOP     I2C1_Write 轮询
② START 78 00 02     STOP     同上
③ START 78 00 10     STOP     同上
④ START 78 40 [128列] STOP    I2C1_WriteDma：仅 128 列走 DMA1 CH6
```

短包不走 DMA：`I2C1_Probe`、命令 `00 cmd`、控制字节 `0x40` 本身。DMA **不**发 START / STOP / `0x78` / `0x40`。

没有 HAL `Callback`。完成处理是 [`i2c.c`](../../../../../projects/f103-manual-reg/src/periph/i2c.c) 的 `DMA1_Channel6_IRQHandler`（覆盖 startup 弱符号）。不开 I2C EV/ER。

`I2C1_Init`：开 AHB 上 DMA1 时钟 + NVIC CH6（IRQn 16）。每次写页再 `DMA1_Channel_Start` 并置 `CR2.DMAEN`。DMA 时钟见 [dma1-ahb-clock.md](dma1-ahb-clock.md)。

## 页数据 DMA（CH6）

1. 命令三笔仍走上面的轮询写（`B0+page` / `02` / `10`）
2. `START`，`DR=0x78`，ACK，轮询写 `0x40`，等 `BTF`
3. `CR2.DMAEN=1`，DMA1 CH6：内存→`I2C1_DR`，128 字节，只开 `TCIE`
4. `DMA1_Channel6_IRQHandler`：停 CH6、清 `DMAEN`、等 `BTF`、`STOP`

`sh1106_buf` 为 `.c` 全局，禁止用局部栈当 DMA 源。

## 应用接口（`i2c.h`）

| 函数 | 作用 |
|------|------|
| `I2C1_Init` | 开时钟、PB6/PB7 AF_OD、400 kHz、`PE`、NVIC CH6 |
| `I2C1_Write(addr8, buf, len)` | 短包轮询；`addr8` 原样进 `DR` |
| `I2C1_WriteDma(addr8, ctrl, mem, len)` | `ctrl` 轮询，其后 `mem` DMA |
| `I2C1_Probe(addr8)` | 只发地址，ACK=1 |

屏幕传 `SH1106_ADDR_WR`（`0x78`）。见 [sh1106](../../../sh1106/README.md)。

## GPIO

PB6/PB7：`CNF=11 MODE=11` → `0xF`。见 [gpio-cnf-mode.md](gpio-cnf-mode.md)。
