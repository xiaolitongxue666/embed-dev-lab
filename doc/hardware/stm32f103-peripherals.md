# STM32F103C8T6 硬件外设与接线

> 目标芯片：STM32F103C8T6（LQFP48）。本文记录**板级接线与采购**；当前仓库固件仍仅实现 **PC13 LED + USART1**，下文「计划外设」尚未在代码中初始化。

相关流程：[快速上手](../getting-started.md) · [编写 → 编译 → 下载](../workflow-write-build-flash.md) · [官方手册索引](../reference/stm32f103/README.md)

---

## 1. 现状与范围

| 类别 | 内容 | 状态 |
|------|------|------|
| 板载 LED | PC13（多数核心板低电平点亮） | **已实现**（两工程） |
| USART1 | PA9 TX / PA10 RX，CH341 USB-TTL | **已实现** |
| SWD | PA13 SWDIO / PA14 SWCLK | **接线保留**，禁止改普通 GPIO |
| OLED SSD1306 | SPI1 4-Wire + DC/RES/CS | **计划接线** |
| 10K 电位器 | ADC1_IN0（PA0） | **计划接线** |
| LM75A | I2C1（PB6/PB7） | **计划接线** |

源码占用依据：

- LED：[`projects/f103-manual-reg/src/main.c`](../../projects/f103-manual-reg/src/main.c)（`LED_PIN 13`）
- USART1：[`projects/f103-manual-reg/src/usart.c`](../../projects/f103-manual-reg/src/usart.c)（默认映射，无 AFIO remap）
- HAL 对照：[`projects/f103-cmsis-hal/src/main.c`](../../projects/f103-cmsis-hal/src/main.c)

全部模块 **3.3 V** 供电、GND 共地；ST-Link 仅下载调试。外设接入后由核心板 **USB 供电**，ST-Link **3.3V 悬空**（避免双源顶牛）。仅 LED demo、板子无 USB 电时，仍可按 [快速上手](../getting-started.md) 由 ST-Link 供电。

---

## 2. 外设清单与采购

价位为 **2026-08-27** 淘宝/天猫检索快照，随促销波动；店铺商品 ID 常变，以**搜索关键词**为稳定入口。下单前核对规格（针脚数、驱动芯片、供电电压）。

| 型号 | 协议 / 作用 | 接到 MCU | 参考价（元） | 购买入口 |
|------|-------------|----------|--------------|----------|
| 0.96 寸 OLED 7 针 SSD1306 | SPI1 4-Wire；显示 ADC / 温度 | VCC→3.3V，GND→GND；D0→PA5，D1→PA7，RES→PA3，DC→PA2，CS→PA4 | 约 6.8–11.5 | [淘宝搜索：0.96寸OLED 7针 SPI SSD1306 3.3V](https://s.taobao.com/search?q=0.96%E5%AF%B8OLED%207%E9%92%88%20SPI%20SSD1306%203.3V) |
| 10K 旋钮电位器模块 | ADC1；0–4095 采样 | VCC→3.3V，GND→GND；AO→PA0 | 约 2–4 | [淘宝搜索：电位器模块 10K 旋钮 模拟输出](https://s.taobao.com/search?q=%E7%94%B5%E4%BD%8D%E5%99%A8%E6%A8%A1%E5%9D%97%2010K%20%E6%97%8B%E9%92%AE%20%E6%A8%A1%E6%8B%9F%E8%BE%93%E5%87%BA) |
| LM75A 温度传感器模块 | I2C1；环境温度 | VCC→3.3V，GND→GND；SCL→PB6，SDA→PB7 | 约 3–6 | [淘宝搜索：LM75A模块 I2C数字温度传感器 3.3V](https://s.taobao.com/search?q=LM75A%E6%A8%A1%E5%9D%97%20I2C%E6%95%B0%E5%AD%97%E6%B8%A9%E5%BA%A6%E4%BC%A0%E6%84%9F%E5%99%A8%203.3V) |
| 830 孔面包板 | 实验底板 | — | 约 3.4–12 | [淘宝搜索：830孔面包板 带电源轨](https://s.taobao.com/search?q=830%E5%AD%94%E9%9D%A2%E5%8C%85%E6%9D%BF%20%E5%B8%A6%E7%94%B5%E6%BA%90%E8%BD%A8) |
| 40P 杜邦线套装 | 接线 | — | 约 6–8 | [淘宝搜索：40P杜邦线 公对公+公对母+母对母](https://s.taobao.com/search?q=40P%E6%9D%9C%E9%82%A6%E7%BA%BF%20%E5%85%AC%E5%AF%B9%E5%85%AC%2B%E5%85%AC%E5%AF%B9%E6%AF%8D%2B%E6%AF%8D%E5%AF%B9%E6%AF%8D) |
| ST-Link V2 | SWD 下载调试 | SWDIO→PA13，SWCLK→PA14，GND→GND；**3.3V 悬空** | 已有 | 无需购买 |
| CH341 USB-TTL | 串口日志（已用） | RX←PA9，TX→PA10，GND 共地 | 已有 | 见 [串口规则](../scripts-reference.md) |

### 采购核对

- OLED 必须买 **7 针 SPI**（含 DC/RES/CS），不要买成仅 4 针 I2C；驱动优先 **SSD1306**（部分店混卖 SSD1315，驱动可能不同）。
- 温度传感器买 **LM75A（I2C）**，不要买成 DS18B20（单总线）。
- 模块丝印 **VCC/GND 顺序不一**，接线以板上丝印为准，勿假定固定顺序。
- 全部选 **3.3 V** 可用模块；禁止把模块 VCC 接到 5 V。

---

## 3. 完整 IO 对应表

含**已占用**与**计划**引脚；全部外设可同时接线，无冲突。

| 模块 | MCU 引脚 | 功能 | 配置要点 | 备注 |
|------|----------|------|----------|------|
| 板载 LED | PC13 | GPIO 推挽输出 | Backup 域：先 PWREN + DBP | **已实现**；低电平点亮常见 |
| OLED | PA5 | SPI1_SCK | SPI1 硬件复用（默认映射） | D0 / SCLK |
| OLED | PA7 | SPI1_MOSI | SPI1 硬件复用 | D1 / SDIN；只写屏，无 MISO |
| OLED | PA2 | OLED_DC | GPIO 推挽输出 | 命令/数据选择，**不可省略** |
| OLED | PA3 | OLED_RES | GPIO 推挽输出 | 硬件复位 |
| OLED | PA4 | OLED_CS | GPIO 推挽输出 | 软件片选（不用硬件 NSS） |
| （未用） | PA6 | SPI1_MISO | — | **悬空** |
| 电位器 | PA0 | ADC1_IN0 | 模拟输入 | 0–4095 |
| LM75A | PB6 | I2C1_SCL | I2C1 默认映射 | 开漏 + 上拉 |
| LM75A | PB7 | I2C1_SDA | I2C1 默认映射 | 开漏 + 上拉 |
| USART1 | PA9 | USART1_TX | 默认映射 | **已实现**；CH341 RX←PA9 |
| USART1 | PA10 | USART1_RX | 默认映射 | **已实现**；CH341 TX→PA10 |
| ST-Link | PA13 | SWDIO | Serial Wire | **禁止改普通 GPIO** |
| ST-Link | PA14 | SWCLK | Serial Wire | **禁止改普通 GPIO** |

---

## 4. 引脚冲突核对

依据 **RM0008 Rev 21** Table 54–56 / `AFIO_MAPR`（本地 PDF：`doc/reference/stm32f103/pdf/`，可用 `./scripts/fetch-stm32f103-docs.sh` 下载）。

| 外设 | 默认映射（REMAP=0） | REMAP=1 | 本方案 |
|------|---------------------|---------|--------|
| USART1 | TX=PA9，RX=PA10 | TX=PB6，RX=PB7 | **默认**（与现有固件一致） |
| I2C1 | SCL=PB6，SDA=PB7 | SCL=PB8，SDA=PB9 | **默认**（LM75A） |
| SPI1 | NSS=PA4，SCK=PA5，MISO=PA6，MOSI=PA7 | NSS=PA15，SCK=PB3，… | **默认**；PA4 作 GPIO CS |

**同时可用的条件：**

1. 保持 USART1 / SPI1 / I2C1 **默认映射**；日后**勿**置位 `USART1_REMAP`（否则与 I2C1 抢 PB6/PB7）。
2. **不启用 USART2**（默认 CTS/RTS/TX/RX/CK 会占 PA0/PA1/PA2/PA3/PA4，与电位器、OLED GPIO 冲突）。
3. PA4 作软件 CS，不依赖硬件 NSS。
4. 不占用 PD0/PD1（多数核心板 HSE 晶振）。
5. PA13/PA14 保持 SWD。

占用汇总（无重叠）：

```text
PC13          LED（已实现）
PA0           ADC1_IN0（计划）
PA2, PA3, PA4 OLED DC / RES / CS（计划）
PA5, PA7      SPI1 SCK / MOSI（计划）
PA6           悬空
PA9, PA10     USART1（已实现）
PA13, PA14    SWD
PB6, PB7      I2C1 LM75A（计划）
```

---

## 5. 连接图与供电

```
                      ┌────────────────────┐
                      │    ST-Link V2      │
                      │ SWDIO ─────────────┼───> PA13
                      │ SWCLK ─────────────┼───> PA14
                      │ GND   ─────────────┼───> 面包板 GND
                      │ 【3.3V 悬空，禁止接外设】│
                      └────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  830 面包板                                                  │
│  +3.3V 轨 ←── 核心板 3.3V（USB 供电）                        │
│  GND 轨   ←── 核心板 GND（与 ST-Link / CH341 共地）          │
│                                                              │
│  STM32F103C8T6 核心板                                        │
│    PA5  → OLED D0(SCLK)                                      │
│    PA7  → OLED D1(MOSI)                                      │
│    PA2  → OLED DC                                            │
│    PA3  → OLED RES                                           │
│    PA4  → OLED CS                                            │
│    PA0  → 电位器 AO                                          │
│    PB6  → LM75A SCL                                          │
│    PB7  → LM75A SDA                                          │
│    PA9  → CH341 RX（USART1_TX）                              │
│    PA10 ← CH341 TX（USART1_RX）                              │
│    PC13 → 板载 LED                                           │
│                                                              │
│  OLED 7 针：VCC/GND→电源轨；D0/D1/DC/RES/CS 如上            │
│  电位器：VCC/GND→电源轨；AO→PA0                              │
│  LM75A：VCC/GND→电源轨；SCL/SDA→PB6/PB7                     │
└─────────────────────────────────────────────────────────────┘

供电：电脑 USB → 核心板 USB 口 → 板上 3.3V → 外设
PA6（SPI1_MISO）本方案不用，悬空
串口：CH341 GND 必须与 MCU 共地，否则乱码
```

---

## 6. 注意事项

1. **电压**：模块一律 3.3 V，勿接 5 V。
2. **共地**：I2C / ADC / 串口 / SWD 的 GND 必须连通。
3. **SWD**：PA13、PA14 保持调试功能。
4. **OLED**：4-Wire SPI 时 **DC 不可省**；CS/RES 若硬件固定到 GND/VCC，须改驱动（本方案接 GPIO）。
5. **SPI**：主机写屏即可，PA6 MISO 悬空。
6. **I2C**：LM75A 需要 SCL/SDA 上拉（多数模块已焊 ~4.7–10 kΩ）；缺上拉则总线卡死。
7. **LM75A**：供电 2.7–5.5 V；7-bit 地址 `1001 A2 A1 A0`，A2–A0 接地时多为 **0x48**（以模块跳线为准）。
8. **ADC**：电位器分压输出接 PA0；勿超过 VDDA（3.3 V）。
9. **与固件**：接好计划外设后，**当前 demo 不会驱动 SPI/I2C/ADC**；LED 与串口仍应按原接线工作。

---

## 7. 日后固件 / Cube 配置摘要（备查）

本仓库**当前不改代码**。若日后实现计划外设，配置方向如下：

| 项 | 建议 |
|----|------|
| RCC | HSE 外部高速晶振（与现工程一致：8 MHz × PLL×9 → 72 MHz） |
| SYS Debug | Serial Wire（保留 SWD） |
| USART1 | PA9/PA10 异步，**不 remap** |
| SPI1 | Master；PA5 SCK、PA7 MOSI；MISO 不启用；CS 用 PA4 GPIO |
| I2C1 | PB6 SCL / PB7 SDA，**不 remap** |
| ADC1 | IN0 = PA0 |
| GPIO | PA2/PA3/PA4 推挽输出（OLED DC/RES/CS） |

---

## 8. 出处

| 资料 | 用途 |
|------|------|
| [DS5319](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) / [本地索引](../reference/stm32f103/md/datasheet-index.md) | 引脚、ADC 通道、封装 |
| [RM0008](https://www.st.com/resource/en/reference_manual/cd00171190.pdf) Table 54–56、`AFIO_MAPR` | USART1 / I2C1 / SPI1 默认与 remap |
| [SSD1306](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) § MCU Serial Interface (4-wire SPI) | D0=SCLK，D1=SDIN，D/C#，CS#，RES# |
| [TI LM75A](https://www.ti.com/lit/ds/symlink/lm75a.pdf) / [NXP LM75A](https://www.nxp.com/docs/en/data-sheet/LM75A.pdf) | I2C、供电、地址 |
| 本仓库 `f103-manual-reg` / `f103-cmsis-hal` 源码 | 已实现 PC13、USART1 引脚 |
| 淘宝/天猫搜索（2026-08-27） | 采购价位与搜索入口 |
