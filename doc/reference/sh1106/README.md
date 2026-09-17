# SH1106 1.3″ 4 针 I2C OLED

供 [`f103-manual-reg`](../../../projects/f103-manual-reg/) 查阅。源码：[`sh1106.c`](../../../projects/f103-manual-reg/src/driver/sh1106.c)。

I2C 写帧与寄存器步骤：[i2c1-master-polling.md](../stm32f103/md/topics/i2c1-master-polling.md)。

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

`0x78 == 0x3C << 1`。固件 / 日志 / Probe / `DR` 只用 **`0x78`**，不要把 `0x3C` 当 API 实参，也不要对 `0x78` 再左移。

## 从零：页和怎么上屏

屏是 **128 格宽 × 64 格高** 的点阵，只认亮/灭，不懂「数字」。`Clear` / `DrawClock` / `DrawTemp` / `DrawPixel` 只改 MCU 里 1024 字节草稿纸；**只有 `Refresh` 才往 I2C 发像素**。

**页**不是文档页，是把 64 行从上到下切成 **8 条横带**，每条高 8 行：

| page | 屏幕 y |
|------|--------|
| 0 | 0–7（最顶） |
| 1 | 8–15 |
| 2 | 16–23 |
| 3 | 24–31 |
| 4 | 32–39 |
| 5 | 40–47 |
| 6 | 48–55 |
| 7 | 56–63（最底） |

中央时钟高 32、从 `y=16` 起，占 **page 2–5**。上下四页全黑。

一页要发的图是 **128 个字节，从左到右一列一个**。一个字节 = 该列上的 8 个竖点：`bit0` 是这条带子最上一行，`bit7` 是最下一行。`00` 全灭，`FF` 全亮。

写一页（页号 0..7）：

```text
START  78  00  B0+页号  STOP     // 选哪一条带子
START  78  00  02       STOP     // 列从可见区左边开始（偏移 2）
START  78  00  10       STOP
START  78  40  128字节  STOP     // 图只在 40 后面（128 列由 DMA1 CH6 搬）
```

显示 `00:00:00` 的操作顺序：

1. `I2C1_Init` → `I2C1_Probe(0x78)`（总线上只有 `START 78 STOP`）
2. `SH1106_Init`：命令含电荷泵 `8D 14`（把 3.3 V 泵到面板高压；不是地址、不是分页），再开显示
3. `SH1106_Clear` → `SH1106_DrawClock(0,0,0)`（只改 RAM）
4. `SH1106_Refresh`：对 page 0..7 各做上面 4 次事务（命令轮询，128 列 DMA；共 32 次 START/STOP）

记住三句：页是 8 像素高的横带；一页 128 个列字节；真正的图只在 `40` 后面。命令轮询、128 列 DMA、ISR 在 [`i2c.c`](../../../projects/f103-manual-reg/src/periph/i2c.c) 的 `DMA1_Channel6_IRQHandler`。白话见 [谁走 DMA](../stm32f103/md/topics/i2c1-master-polling.md#白话谁走-dma中断在哪)；DMA1 须开 AHB 时钟，见 [dma1-ahb-clock.md](../stm32f103/md/topics/dma1-ahb-clock.md)。总线帧格式见 [I2C1 主机写一帧](../stm32f103/md/topics/i2c1-master-polling.md#主机写一帧)。

### 字库

**SH1106 片内没有字库。** 只有 GDDRAM，不认 ASCII，也不能「写一个 `'0'` 就出字」。

本仓库的字形在 MCU Flash：[`sh1106_font.c`](../../../projects/f103-manual-reg/src/driver/sh1106_font.c) 有 **`'0'`–`'9'`、`':'`、`'.'`、`'-'`、`'C'`** 的 8×16 点阵。`DrawClock`（×2）/ `DrawTemp`（1×）查表再 `DrawPixel`。没有汉字。

要更多字：自行取模，加 `const` 数组，或按点画。字库占的是 **F103 的 64 KB Flash**，不是 OLED 芯片。

## I2C 控制字节

每次命令或数据前必须先发：

- `0x00`：后续为命令
- `0x40`：后续为 GDDRAM 数据

省略则黑屏。

## 与 SSD1306

SSD1306 的显存同样按页切（8 页 × 128 列）。差别：它还可以 horizontal / vertical 寻址，设一次后连发 1024 字节自动翻页；**SH1106 通常只有 page 寻址**，每一页都要再发 `B0+page` 和列地址，所以本驱动每页 4 次事务。

不要整段套用 SSD1306 初始化当「同一块屏」。电荷泵是片内升压（面板大约要 7–9 V），命令 **`0x8D` 再 `0x14` 必须发**（不发则几乎全黑）。官方 SH1106 有时写 `0xAD`/`0x8B`，本模块按能点亮的 `0x8D`/`0x14`。刷新必须带列偏移 `02`/`10`，否则左右黑边 / 错位。

## 写显存（每页）

```text
START  78  00  B0+page  STOP     // page 0=B0 … 7=B7
START  78  00  02       STOP     // 列低 4 bit，偏移 2
START  78  00  10       STOP     // 列高 4 bit
START  78  40  d0..d127 STOP     // 该页 128 列（DMA1 CH6）；图源不要自己加 2 列
```

4 针无 RES，用忙等代替复位脚。

## 固件接口

源码：[`sh1106.h`](../../../projects/f103-manual-reg/src/driver/sh1106.h) / [`i2c.h`](../../../projects/f103-manual-reg/src/periph/i2c.h)。

```text
I2C1_Init()
I2C1_Probe(0x78)           → 串口 SH1106 ACK addr=0x78
SH1106_Init()              → 命令序列 + 清 RAM + 0xAF
循环每秒：
  SH1106_Clear()
  SH1106_DrawClock(h,m,s)  → 中央 HH:MM:SS（只改缓冲）
  SH1106_DrawTemp(centi)   → 右下 8×16 温度（y=48）
  SH1106_Refresh()         → 8 页 × 上表 4 次事务（数据段 DMA）
```

| 符号 | 作用 |
|------|------|
| `I2C1_Write(addr8, buf, len)` | 短包轮询；超时 / AF 返回 0 |
| `I2C1_WriteDma(addr8, ctrl, mem, len)` | `ctrl` 轮询，其后 DMA1 CH6 |
| `I2C1_Probe(addr8)` | 只发地址看 ACK |
| `SH1106_Init` | init 序列、清屏、开显示 |
| `SH1106_Clear` | 清缓冲，不写屏 |
| `SH1106_DrawPixel` / `SH1106_DrawClock` / `SH1106_DrawTemp` | 改缓冲 |
| `SH1106_Refresh` | 缓冲 → GDDRAM |

**当前上电行为**：屏中央 `HH:MM:SS`（8×16 点阵 ×2，y=16..47）；右下补偿温度（1× 8×16，y=48）。SysTick 1 ms，**不是 RTC**；`SH1106_Init` 后 `SysTick_SetMs(0)`，从 `00:00:00` 起。复位清零。自己画图见下文，不必走 `DrawClock`。

## 实际例子：画出电子时钟 `00:00:00`

对照 [`main.c`](../../../projects/f103-manual-reg/src/app/main.c)：`SH1106_Clear` → `SH1106_DrawClock` → `SH1106_DrawTemp` → `SH1106_Refresh`。软件只改 RAM；**上屏的 I2C 字节全部来自 Refresh**（外加 Probe / Init）。

几何（与源码一致）：时钟 8 个字形 × 16 像素宽 = 128，高 32；`x=0`，`y=16`，占 **page 2–5**（`y=16..47`）。温度 1× 8×16 右对齐，`y=48`，占 **page 6–7**。page 0/1 全是 `00`。

### Probe

```text
START  78  STOP
```

### Init（摘录）

每条命令一次事务 `START 78 00 cmd STOP`。完整序列在 `SH1106_Init`，至少包括：

```text
START  78  00  AE  STOP     // 关显示
START  78  00  8D  STOP
START  78  00  14  STOP     // 电荷泵
START  78  00  AF  STOP     // 开显示
```

中间还有时钟分频、MUX、对比度等；Init 末尾会先 Refresh 一帧全 0，再 `AF`。

### Refresh（上屏）

每一页 4 次 START/STOP，page = 0..7，共 **32** 次事务：

```text
START  78  00  B0+page  STOP
START  78  00  02       STOP
START  78  00  10       STOP
START  78  40  d0 … d127  STOP
```

page 0 时第一条命令是 `B0`，page 2 是 `B2`，page 7 是 `B7`。

### page 2 数据帧（时钟最上 8 行）

`y=16..23`。字形从左到右：`0 0 : 0 0 : 0 0`，各占 16 列。字节由 [`sh1106_font.c`](../../../projects/f103-manual-reg/src/driver/sh1106_font.c) 的 8×16 点阵按 `DrawPixel`（`bit = y%8`）再 2×2 放大得到，可用逻辑分析仪对照。

`'0'` 切片 16 字节：

```text
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00
```

`':'` 切片 16 字节：

```text
00 00 00 00 00 00 C0 C0 C0 C0 00 00 00 00 00 00
```

整页 128 字节（`78 40` 之后）：

```text
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00   // '0'
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00   // '0'
00 00 00 00 00 00 C0 C0 C0 C0 00 00 00 00 00 00   // ':'
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00   // '0'
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00   // '0'
00 00 00 00 00 00 C0 C0 C0 C0 00 00 00 00 00 00   // ':'
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00   // '0'
00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00   // '0'
```

总线上这一帧即：

```text
START  78  40  00 00 F0 F0 …（上表 128 字节）…  STOP
```

page 0/1/6/7：`78 40` 后 128 个 `00`。自己放图时，命令三行不变，只换 `d0..d127`。

这 128 字节只是 page 2（时钟头顶 8 行），不是整颗 `'0'`。完整 `'0'` 高 32 像素，肚子和底在 page 3–5。

一字节拆开时右边是低位（bit0 在上）。常用值：

| 值 | 二进制 bit7…bit0 | 该列从上到下 8 点（`.`灭 `#`亮） |
|----|------------------|----------------------------------|
| `00` | `00000000` | `........` |
| `0C` | `00001100` | `..##....`（只亮 y=18、19） |
| `F0` | `11110000` | `....####`（只亮 y=20..23） |
| `FC` | `11111100` | `..######`（亮 y=18..23） |
| `C0` | `11000000` | `......##`（只亮 y=22、23） |

第一个 `'0'` 的 16 列 × page 2 的 8 行（相邻列成对，因为 2× 放大）：

```text
列:  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
     00 00 F0 F0 FC FC 0C 0C 0C 0C FC FC F0 F0 00 00

y16  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
y17  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
y18  .  .  .  .  #  #  #  #  #  #  #  #  .  .  .  .   顶横梁
y19  .  .  .  .  #  #  #  #  #  #  #  #  .  .  .  .
y20  .  .  #  #  #  #  .  .  .  .  #  #  #  #  .  .   左右竖边
y21  .  .  #  #  #  #  .  .  .  .  #  #  #  #  .  .
y22  .  .  #  #  #  #  .  .  .  .  #  #  #  #  .  .
y23  .  .  #  #  #  #  .  .  .  .  #  #  #  #  .  .
```

`':'` 的上点是中间四个 `C0`：只在 y=22、23 亮 4 列。下点在 page 4。

## 显示图像

页、列、bit 与 [从零：页和怎么上屏](#从零页和怎么上屏) 相同。坐标系与 [`SH1106_DrawPixel`](../../../projects/f103-manual-reg/src/driver/sh1106.c) 一致：

- 可视 `x=0..127`，`y=0..63`
- `page = y / 8`，`bit = y % 8`，亮点：`buf[page][x] |= 1 << bit`
- 全屏 1bpp = **1024 字节** = 8 页 × 128 列
- 须 `Refresh` 才上屏；每页仍发 `02`/`10`，图源**不要**自己加 2 列

现有 API（无 `DrawBitmap`）：

1. `SH1106_Clear()`
2. 亮点调用 `SH1106_DrawPixel(x, y, 1)`；或按下表拆 `const` 数组再 `DrawPixel`
3. `SH1106_Refresh()`

页内字节 **bit0 = 该页最上一行**（与 `DrawPixel` 相同）：

```c
/* img[page][x]：8×128，bit0 = y=page*8 */
const unsigned char img[8][128] = { /* 取模结果 */ };

unsigned int page, x, bit;
SH1106_Clear();
for (page = 0; page < 8; page++) {
    for (x = 0; x < 128; x++) {
        for (bit = 0; bit < 8; bit++) {
            if ((img[page][x] & (1U << bit)) != 0U) {
                SH1106_DrawPixel(x, page * 8U + bit, 1U);
            }
        }
    }
}
SH1106_Refresh();
```

不要把 BMP/PNG 直接链进固件。取模后存 `const unsigned char[]` 于内部 Flash。全屏一帧 1 KB；动画仍受 64 KB 限制（约 8–15 帧）。

## 黑屏排查

1. 串口 `SH1106 ACK addr=0x78`（NACK 先查 3.3V / 共地 / SCL·SDA）
2. 发了 `0x8D,0x14`
3. 每页发了 `0x02,0x10`
4. 控制字节 `0x00` / `0x40` 未省略
