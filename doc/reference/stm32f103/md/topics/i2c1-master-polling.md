# I2C1 主机写（轮询）

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 I2C 章（CR1/CR2/CCR/TRISE/SR1/SR2/DR） |
| 源码 | [`projects/f103-manual-reg/src/i2c.c`](../../../../../projects/f103-manual-reg/src/i2c.c) |
| 接线 | [stm32f103-peripherals.md](../../../../hardware/stm32f103-peripherals.md) |
| 屏幕 | [sh1106](../../../sh1106/README.md) |

## 本工程约定

- 硬件 I2C1，默认映射 **PB6=SCL、PB7=SDA**；复用开漏 50 MHz（`CRL` 半字节 `0xF`）
- **不** `USART1_REMAP` / `I2C1_REMAP`
- 从机参数是 **8 位写地址**，原样写入 `DR`。SH1106 为 **`0x78`**，禁止再 `<< 1`
- 轮询 + 超时；无 I2C IRQ / DMA（USART 已用 DMA1 CH4/CH5）
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

## 应用接口（`i2c.h`）

| 函数 | 作用 |
|------|------|
| `I2C1_Init` | 开时钟、PB6/PB7 AF_OD、400 kHz、`PE` |
| `I2C1_Write(addr8, buf, len)` | 主机写；`addr8` 原样进 `DR` |
| `I2C1_Probe(addr8)` | 只发地址，ACK=1 |

屏幕传 `SH1106_ADDR_WR`（`0x78`）。见 [sh1106](../../../sh1106/README.md)。

## GPIO

PB6/PB7：`CNF=11 MODE=11` → `0xF`。见 [gpio-cnf-mode.md](gpio-cnf-mode.md)。
