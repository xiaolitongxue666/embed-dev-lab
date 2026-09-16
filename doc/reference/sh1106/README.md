# SH1106 1.3″ 4 针 I2C OLED

供 [`f103-manual-reg`](../../../projects/f103-manual-reg/) 查阅。源码：[`sh1106.c`](../../../projects/f103-manual-reg/src/sh1106.c)。

## 本模块

| 项 | 值 |
|----|-----|
| 接口 | 4 针 I2C；板载上拉，外部不再加 4.7 kΩ |
| 供电 | 面包板 **3.3V** 轨（模块也可 5V，本仓库不接 5V） |
| 8 位写地址 | **`0x78`**（R6 焊、R5 空）。读地址 `0x79` 本驱动不用 |
| 可视 | 128×64 |
| GDDRAM | 132×64；可见从列 2 起 |
| 列偏移 | 每页 `0x02` + `0x10`（`SH1106_COL_OFFSET=2`） |
| MCU | PB6=SCL，PB7=SDA（I2C1 默认映射） |

`0x78 == 0x3C << 1`。固件 / 日志 / Probe 只用 **`0x78`**，不要把 `0x3C` 当 API 实参，也不要对 `0x78` 再左移。

## I2C 控制字节

每次命令或数据前必须先发：

- `0x00`：后续为命令
- `0x40`：后续为 GDDRAM 数据

省略则黑屏。

## 与 SSD1306

不要整段套用 SSD1306 初始化当「同一块屏」。本驱动按参考序列发送，其中 **`0x8D,0x14` 电荷泵必须发**（黑则几乎看不见）。刷新必须带列偏移，否则左右黑边 / 错位。

## 写显存（每页）

```text
0xB0 + page     // page 0..7
0x02            // 列低 4 bit，偏移 2
0x10            // 列高 4 bit
0x40 + 128 字节 // 本工程按页一次事务，协议与单字节写相同
```

4 针无 RES，用忙等代替复位脚。

当前 demo：屏中央 `HH:MM:SS`（8×16 点阵放大 2 倍，垂直居中）。时基为 SysTick 1 ms，上电从 `00:00:00` 计时。

## 黑屏排查

1. 串口 `SH1106 ACK addr=0x78`（NACK 先查 3.3V / 共地 / SCL·SDA）
2. 发了 `0x8D,0x14`
3. 每页发了 `0x02,0x10`
4. 控制字节 `0x00` / `0x40` 未省略

## 固件接口

源码：[`sh1106.h`](../../../projects/f103-manual-reg/src/sh1106.h) / [`i2c.h`](../../../projects/f103-manual-reg/src/i2c.h)。

```text
I2C1_Init()
I2C1_Probe(0x78)           → 串口 SH1106 ACK addr=0x78
SH1106_Init()              → 命令序列 + 清 RAM + 0xAF
循环每秒：
  SH1106_Clear()
  SH1106_DrawClock(h,m,s)  → 只改 1024 B 缓冲
  SH1106_Refresh()         → 8 页 × (0xB0+page, 0x02, 0x10, 0x40+128)
```

| 符号 | 作用 |
|------|------|
| `I2C1_Write(addr8, buf, len)` | START + 8 位地址 + 数据 + STOP；超时 / AF 返回 0 |
| `I2C1_Probe(addr8)` | 只发地址看 ACK |
| `SH1106_Init` | init 序列、清屏、开显示 |
| `SH1106_Clear` | 清缓冲，不写屏 |
| `SH1106_DrawPixel` / `SH1106_DrawClock` | 改缓冲 |
| `SH1106_Refresh` | 缓冲 → GDDRAM |

时钟：SysTick 1 ms，**不是 RTC**；`SH1106_Init` 后 `SysTick_SetMs(0)`，从 `00:00:00` 起。复位清零。

## 总线

[i2c1-master-polling.md](../stm32f103/md/topics/i2c1-master-polling.md)
