# STM32F103C8T6 硬件外设与接线

> 目标芯片：STM32F103C8T6（LQFP48，64 KB Flash / 20 KB RAM）。本文记录**板级接线与采购**；当前仓库固件仍仅实现 **PC13 LED + USART1**，下文「计划外设」尚未在代码中初始化。

相关流程：[快速上手](../getting-started.md) · [编写 → 编译 → 下载](../workflow-write-build-flash.md) · [官方手册索引](../reference/stm32f103/README.md)

---

## 1. 现状与范围

| 类别 | 内容 | 状态 |
|------|------|------|
| 板载 LED | PC13（多数核心板低电平点亮） | **已实现**（两工程） |
| USART1 | PA9 TX / PA10 RX，CH341 USB-TTL | **已实现** |
| SWD | PA13 SWDIO / PA14 SWCLK | **接线保留**，禁止改普通 GPIO |
| SSD1306 0.96 寸 OLED（4 针 I2C） | I2C1 PB6/PB7 | **计划接线** |
| FT6236U 触摸盖板 | I2C1 + PB0 INT + PB1 RST | **计划接线** |
| CJMCU-633 LSM6DSO | SPI1 全双工（独占 SPI） | **计划接线** |
| BMP280（可选） | I2C1 多从机练习 | **计划接线** |

源码占用依据：

- LED：[`projects/f103-manual-reg/src/main.c`](../../projects/f103-manual-reg/src/main.c)（`LED_PIN 13`）
- USART1：[`projects/f103-manual-reg/src/usart.c`](../../projects/f103-manual-reg/src/usart.c)（默认映射，无 AFIO remap）
- HAL 对照：[`projects/f103-cmsis-hal/src/main.c`](../../projects/f103-cmsis-hal/src/main.c)

全部模块 **3.3 V** 供电、GND 共地；ST-Link 仅下载调试。外设接入后由核心板 **USB 供电**，ST-Link **3.3V 悬空**（避免双源顶牛）。仅 LED demo、板子无 USB 电时，仍可按 [快速上手](../getting-started.md) 由 ST-Link 供电。

---

## 2. 学习目标与硬约束（日后固件意图，未实现）

| 目标 | 约束 |
|------|------|
| SPI 全双工 | 用 **LSM6DSO** 读写寄存器（MOSI 发地址、MISO 同步回数据）；**禁止** Flash / W25Q / TF 等存储类外设当 SPI 教材 |
| I2C 总线 | 多从机挂载、7 位地址、触摸 INT 中断 |
| 显示 | 无内置字库、无 TF；像素由 MCU 控制；动画帧预取模为 `const uint8_t[]` 存**内部 Flash**；月薪猫类动画仅 **8–15 帧**精简循环（完整帧放不下 64 KB） |
| 总线分工 | **SPI1 专给 IMU**；屏幕走 I2C，**不占 SPI** |
| UI 库（若日后引入） | 关闭文件系统与 GIF：`LV_USE_FILESYSTEM=0`、`LV_USE_GIF=0`；本仓库**尚未接入 LVGL** |

---

## 3. 外设清单与采购

价位为 **2026-08-28** 淘宝/天猫检索快照，随促销波动；店铺商品 ID 常变，以**搜索关键词**为稳定入口。下单前核对规格（针脚数、驱动芯片、供电电压、是否带 INT/RST）。

| 型号 | 协议 / 作用 | 接到 MCU | 参考价（元） | 购买入口 |
|------|-------------|----------|--------------|----------|
| SSD1306 0.96 寸 OLED（4 针） | I2C1；显示 / 精简动画点阵 | VCC→3.3V，GND→GND；SCL→PB6，SDA→PB7 | 约 4–8（店面常见 6.8 起） | [淘宝搜索：0.96寸OLED 4针I2C SSD1306 128*64 3.3V](https://s.taobao.com/search?q=0.96%E5%AF%B8OLED%204%E9%92%88I2C%20SSD1306%20128*64%203.3V) |
| FT6236U 0.96 寸触摸盖板 | I2C1 + GPIO；坐标与 INT | SCL/SDA→PB6/PB7；INT→PB0；RST→PB1；VCC/GND | **0.96″ 零售稀缺**；≥2.3″ FT6236 面板常见约 30+ | [淘宝搜索：FT6236U 0.96寸电容触摸盖板 I2C 带INT RST 3.3V](https://s.taobao.com/search?q=FT6236U%200.96%E5%AF%B8%E7%94%B5%E5%AE%B9%E8%A7%A6%E6%91%B8%E7%9B%96%E6%9D%BF%20I2C%20%E5%B8%A6INT%20RST%203.3V) |
| CJMCU-633 LSM6DSO | SPI1 全双工；六轴 IMU（非存储） | SCK→PA5，MOSI→PA7，MISO→PA6，CS→PA4；VCC/GND | 检索约 **40–65**（高于口头 8–12，以检索为准） | [淘宝搜索：CJMCU-633 LSM6DSO 六轴姿态传感器 SPI I2C 3.3V](https://s.taobao.com/search?q=CJMCU-633%20LSM6DSO%20%E5%85%AD%E8%BD%B4%E5%A7%BF%E6%80%81%E4%BC%A0%E6%84%9F%E5%99%A8%20SPI%20I2C%203.3V) |
| BMP280（可选） | I2C1；温度气压多从机 | SCL→PB6，SDA→PB7；VCC/GND | 约 3–6 | [淘宝搜索：BMP280 气压温度传感器模块 I2C 3.3V](https://s.taobao.com/search?q=BMP280%20%E6%B0%94%E5%8E%8B%E6%B8%A9%E5%BA%A6%E4%BC%A0%E6%84%9F%E5%99%A8%E6%A8%A1%E5%9D%97%20I2C%203.3V) |
| 830 孔面包板 | 实验底板 | — | 约 3.4–12 | [淘宝搜索：830孔面包板 带电源轨](https://s.taobao.com/search?q=830%E5%AD%94%E9%9D%A2%E5%8C%85%E6%9D%BF%20%E5%B8%A6%E7%94%B5%E6%BA%90%E8%BD%A8) |
| 40P 杜邦线套装 | 接线 | — | 约 6–8 | [淘宝搜索：40P杜邦线 公对公+公对母+母对母](https://s.taobao.com/search?q=40P%E6%9D%9C%E9%82%A6%E7%BA%BF%20%E5%85%AC%E5%AF%B9%E5%85%AC%2B%E5%85%AC%E5%AF%B9%E6%AF%8D%2B%E6%AF%8D%E5%AF%B9%E6%AF%8D) |
| ST-Link V2 | SWD 下载调试 | SWDIO→PA13，SWCLK→PA14，GND→GND；**3.3V 悬空** | 已有 | 无需购买 |
| CH341 USB-TTL | 串口日志（已用） | RX←PA9，TX→PA10，GND 共地 | 已有 | 见 [串口规则](../scripts-reference.md) |

### 采购核对

- OLED 必须买 **4 针 I2C**（VCC/GND/SCL/SDA），**不要**买 7 针 SPI；驱动优先 **SSD1306**（店内常混卖 SSD1315）。
- **无** 0.96 寸 OLED+触摸一体零售成品：OLED 上叠放触摸盖板，胶带固定。若搜不到 0.96″ FT6236 盖板，可试 `FT6236 电容触摸盖板 I2C INT RST`；注意外形是否盖得住 0.96″ 有效区，坐标与分辨率需日后校准。
- LSM6DSO 走 **SPI** 须接满 SCK / MOSI / MISO / CS；勿买成纯 I2C 断排针、或 W25Q/TF 当「SPI 练习」。
- BMP280：I2C 模式时 CSB 接高（多数模块已处理）；SDO 决定地址 `0x76`/`0x77`。
- 模块丝印 **VCC/GND 顺序不一**，以板上丝印为准。
- 全部选 **3.3 V**；禁止模块 VCC 接 5 V。

---

## 4. 完整 IO 对应表

含**已占用**与**计划**引脚；全部外设可同时接线，无冲突。

| 模块 | MCU 引脚 | 功能 | 配置要点 | 备注 |
|------|----------|------|----------|------|
| 板载 LED | PC13 | GPIO 推挽输出 | Backup 域：先 PWREN + DBP | **已实现**；低电平点亮常见 |
| SSD1306 / FT6236 / BMP280 | PB6 | I2C1_SCL | I2C1 默认映射，开漏 + 上拉 | 总线共用 |
| SSD1306 / FT6236 / BMP280 | PB7 | I2C1_SDA | I2C1 默认映射，开漏 + 上拉 | 总线共用 |
| FT6236 | PB0 | INT | GPIO_EXTI，下降沿 | 触摸数据就绪 |
| FT6236 | PB1 | RST | GPIO 推挽输出 | 硬件复位 |
| LSM6DSO | PA5 | SPI1_SCK | SPI1 默认映射 | 全双工时钟 |
| LSM6DSO | PA7 | SPI1_MOSI | SPI1 默认映射 | 主机→传感器 |
| LSM6DSO | PA6 | SPI1_MISO | SPI1 默认映射 | 传感器→主机（必接） |
| LSM6DSO | PA4 | CS | GPIO 软件片选（低有效） | 不用硬件 NSS |
| USART1 | PA9 | USART1_TX | 默认映射 | **已实现**；CH341 RX←PA9 |
| USART1 | PA10 | USART1_RX | 默认映射 | **已实现**；CH341 TX→PA10 |
| ST-Link | PA13 | SWDIO | Serial Wire | **禁止改普通 GPIO** |
| ST-Link | PA14 | SWCLK | Serial Wire | **禁止改普通 GPIO** |

### I2C 7 位从机地址（多从机，互不冲突）

| 器件 | 常见地址 | 说明 |
|------|----------|------|
| SSD1306 | `0x3C` 或 `0x3D` | 以模块跳线 / `I2C` 扫描为准 |
| FT6236 | `0x38`（ADDR 可改 `0x39`） | FocalTech FT6x36 系列 |
| BMP280 | `0x76` 或 `0x77` | 由 SDO 电平决定 |

---

## 5. 引脚冲突核对

依据 **RM0008 Rev 21** Table 54–56 / `AFIO_MAPR`（本地 PDF：`doc/reference/stm32f103/pdf/`，可用 `./scripts/fetch-stm32f103-docs.sh` 下载）。

| 外设 | 默认映射（REMAP=0） | REMAP=1 | 本方案 |
|------|---------------------|---------|--------|
| USART1 | TX=PA9，RX=PA10 | TX=PB6，RX=PB7 | **默认**（与现有固件一致） |
| I2C1 | SCL=PB6，SDA=PB7 | SCL=PB8，SDA=PB9 | **默认**（OLED / 触摸 / BMP280） |
| SPI1 | NSS=PA4，SCK=PA5，MISO=PA6，MOSI=PA7 | NSS=PA15，SCK=PB3，… | **默认**；PA4 作 GPIO CS |

**同时可用的条件：**

1. 保持 USART1 / SPI1 / I2C1 **默认映射**；日后**勿**置位 `USART1_REMAP`（否则与 I2C1 抢 PB6/PB7）。
2. **不启用 USART2**（默认会占 PA0–PA4，与 SPI1 CS/SCK/MISO/MOSI 冲突）。
3. PA4 作软件 CS，不依赖硬件 NSS。
4. 不占用 PD0/PD1（多数核心板 HSE）。
5. PA13/PA14 保持 SWD。

占用汇总（无重叠）：

```text
PC13              LED（已实现）
PA4, PA5, PA6, PA7  SPI1 → LSM6DSO（计划）
PA9, PA10         USART1（已实现）
PA13, PA14        SWD
PB0, PB1          FT6236 INT / RST（计划）
PB6, PB7          I2C1 多从机（计划）
```

---

## 6. 连接图与供电

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
│    PB6  → I2C1_SCL ──┬── OLED SCL / FT6236 SCL [/ BMP280]  │
│    PB7  → I2C1_SDA ──┴── OLED SDA / FT6236 SDA [/ BMP280]  │
│    PB0  → FT6236 INT                                         │
│    PB1  → FT6236 RST                                         │
│    PA5  → LSM6DSO SCK                                        │
│    PA7  → LSM6DSO SDI(MOSI)                                  │
│    PA6  ← LSM6DSO SDO(MISO)                                  │
│    PA4  → LSM6DSO CS                                         │
│    PA9  → CH341 RX（USART1_TX）                              │
│    PA10 ← CH341 TX（USART1_RX）                              │
│    PC13 → 板载 LED                                           │
│                                                              │
│  OLED 4 针：VCC/GND→电源轨；SCL/SDA→PB6/PB7                  │
│  触摸盖板：叠在 OLED 上；SCL/SDA 同上；INT→PB0；RST→PB1     │
│  LSM6DSO：VCC/GND→电源轨；四线 SPI 如上                      │
│  BMP280（可选）：VCC/GND→电源轨；SCL/SDA→PB6/PB7             │
└─────────────────────────────────────────────────────────────┘

供电：电脑 USB → 核心板 USB 口 → 板上 3.3V → 外设
串口：CH341 GND 必须与 MCU 共地，否则乱码
SPI：专给 IMU；屏幕不占 SPI
```

---

## 7. 注意事项

1. **电压**：模块一律 3.3 V，勿接 5 V（LSM6DSO 绝对最大约 3.6 V）。
2. **共地**：I2C / SPI / 串口 / SWD 的 GND 必须连通。
3. **SWD**：PA13、PA14 保持调试功能。
4. **I2C**：SCL/SDA 需上拉（多数模块已焊 ~4.7–10 kΩ）；缺上拉则总线卡死。多从机靠不同 7 位地址区分。
5. **触摸**：0.96″ 无一体成品；盖板叠装后 INT 下降沿置标志，主循环再 I2C 读坐标。
6. **SPI / LSM6DSO**：Full-Duplex Master；软件 NSS（PA4 低有效）；默认 4-wire（须接 MISO）；芯片兼容 SPI Mode 0 / 3。CS 拉高时器件可走 I2C——本方案用 SPI，CS 由 MCU 驱动。
7. **禁止**用 W25Q / TF 当 SPI「学习外设」；本方案 SPI 仅 IMU。
8. **Flash 预算**：动画帧进内部 Flash；完整高帧数动画放不下，限 8–15 帧精简循环。
9. **与固件**：接好计划外设后，**当前 demo 不会驱动 SPI/I2C**；LED 与串口仍应按原接线工作。

---

## 8. 日后固件 / Cube 配置摘要（备查）

本仓库**当前不改代码**。若日后实现计划外设，配置方向如下：

| 项 | 建议 |
|----|------|
| RCC | HSE 8 MHz × PLL×9 → 72 MHz（与现工程一致） |
| SYS Debug | Serial Wire（保留 SWD） |
| USART1 | PA9/PA10 异步，**不 remap** |
| SPI1 | Full-Duplex Master；PA5/PA6/PA7；软件 NSS=PA4；Mode 0 |
| I2C1 | PB6/PB7，**不 remap**；400 kHz Fast Mode |
| GPIO | PB0 = EXTI 下降沿；PB1 = 推挽输出（RST） |
| 业务划分 | SPI 任务仅 LSM6DSO；I2C：OLED 显示 + FT6236 触摸 + 可选 BMP280 |
| LVGL（若引入） | `LV_USE_FILESYSTEM 0`；`LV_USE_GIF 0`；`LV_COLOR_DEPTH 1`；`LV_MEM_SIZE` 约 7 KB —— **未接入本仓库** |

---

## 9. 出处

| 资料 | 用途 |
|------|------|
| [DS5319](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) / [本地索引](../reference/stm32f103/md/datasheet-index.md) | 引脚、封装 |
| [RM0008](https://www.st.com/resource/en/reference_manual/cd00171190.pdf) Table 54–56、`AFIO_MAPR` | USART1 / I2C1 / SPI1 默认与 remap |
| [SSD1306](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) | I2C / 显示控制器 |
| [FT6x36](https://buydisplay.com/download/ic/FT6236-FT6336-FT6436L-FT6436_Datasheet.pdf) | 触摸 I2C、INT、RST |
| [LSM6DSO](https://www.st.com/resource/en/datasheet/lsm6dso.pdf) | SPI 4-wire、供电 |
| [BMP280](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf) | I2C 地址与接口 |
| 本仓库 `f103-manual-reg` / `f103-cmsis-hal` 源码 | 已实现 PC13、USART1 引脚 |
| 淘宝/天猫搜索（2026-08-28） | 采购价位与搜索入口 |
