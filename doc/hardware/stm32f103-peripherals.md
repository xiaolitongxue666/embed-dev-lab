# STM32F103C8T6 硬件外设与接线

> 目标芯片：STM32F103C8T6（LQFP48，64 KB Flash / 20 KB RAM）。本文记录**板级接线与采购**；`f103-manual-reg` 已实现 **PC13 / PB12 LED + PB13 KEY + USART1 + ADC1 PA0 旋钮 + SPI1 BMP280 温度 + LSM6DS3 + I2C1 SH1106**。  
> **目标接法（一次定死）**：BMP280 与 LSM6DS3 **共用 SPI1 三线**（PA5/PA7/PA6），独立片选 **PA3=BMP280、PA8=LSM6DS3**；JY003 PWM 用 **PA1=TIM2_CH2**。

完整引脚总表（丝印 / FT / 复用 / 本仓库占用）：[stm32f103c8t6-pinout.md](stm32f103c8t6-pinout.md) · 官方 Table 5 摘录：[lqfp48-pinout.md](../reference/stm32f103/md/topics/lqfp48-pinout.md)

相关流程：[快速上手](../getting-started.md) · [供电、共地与 SWD](power-and-common-ground.md) · [编写 → 编译 → 下载](../workflow-write-build-flash.md) · [官方手册索引](../reference/stm32f103/README.md) · [LSM6DS3 参考](../reference/lsm6ds3/README.md)

---

## 1. 现状与范围

| 类别 | 内容 | 状态 |
|------|------|------|
| 板载 LED | PC13（多数核心板低电平点亮） | **已实现**（两工程） |
| 外接 LED | PB12→220 Ω→LED+，LED-→GND（拉电流） | **已实现**（`f103-manual-reg`） |
| 按键 | PB13←按键→GND（片内上拉，低有效） | **已实现**（`f103-manual-reg`） |
| 旋钮电位器 | PA0 ADC12_IN0；模块 3.3V，SIG→PA0 | **已实现**（`f103-manual-reg`） |
| USART1 | PA9 TX / PA10 RX，CH341 USB-TTL | **已实现** |
| SWD | PA13 SWDIO / PA14 SWCLK | **接线保留**，禁止改普通 GPIO |
| SH1106 1.3 寸 OLED（4 针 I2C） | I2C1 PB6/PB7，写地址 `0x78` | **已实现**（`f103-manual-reg`） |
| FT6236U 触摸盖板 | I2C1 + PB0 INT + PB1 RST | **暂不使用**（条目保留，接线可选） |
| LSM6DS3 / LSM6DS3TR 模块 | SPI1 全双工；CS=PA8 | **固件已实现**；硬件尚未接 |
| BMP280 / BME280 | SPI1 共用三线；CS=PA3，Mode 0；补偿温度上屏 | **已实现**（`f103-manual-reg`） |
| JY003 风扇模块 | TIM2_CH2 PWM @ PA1；电机电源独立 | **阶段 4** |

源码占用依据：

- LED / KEY：[`projects/f103-manual-reg/src/main.c`](../../projects/f103-manual-reg/src/main.c)（`BOARD_LED_PIN` PC13、`EXT_LED_PIN` PB12、`KEY_PIN` PB13）
- USART1：[`projects/f103-manual-reg/src/usart.c`](../../projects/f103-manual-reg/src/usart.c)（默认映射，无 AFIO remap）
- ADC1 / 旋钮：[`adc.c`](../../projects/f103-manual-reg/src/adc.c)（PA0 = ADC12_IN0）
- SPI1 / LSM6DS3：[`spi.c`](../../projects/f103-manual-reg/src/spi.c)、[`lsm6ds3.c`](../../projects/f103-manual-reg/src/lsm6ds3.c)（PA8 CS + Mode 3）
- I2C1 / SH1106：[`i2c.c`](../../projects/f103-manual-reg/src/i2c.c)、[`sh1106.c`](../../projects/f103-manual-reg/src/sh1106.c)（8 位写地址 `0x78`）
- BMP280：[`bmp280.c`](../../projects/f103-manual-reg/src/bmp280.c)（PA3 CS + Mode 0，校准补偿 T/P）；JY003：阶段 4 `tim2.c`
- HAL 对照：[`projects/f103-cmsis-hal/src/main.c`](../../projects/f103-cmsis-hal/src/main.c)（本轮未同步 SPI / I2C）

全部当前模块 **3.3 V** 供电。蓝板由 **MicroUSB 独立供电**；ST-Link 只做 SWD，并把 **3.3V / GND** 拉到面包板（方案 A：5V 闲置）——**禁止**把 ST-Link 电源接到蓝板。共地以面包板 GND 轨为汇集点。详解：[供电、共地与 SWD](power-and-common-ground.md)。

---

## 2. 学习目标与硬约束

| 目标 | 约束 |
|------|------|
| SPI 全双工 | **LSM6DS3** 与 **BMP280** 共用 SPI1 三线、独立 CS；**禁止** Flash / W25Q / TF 等存储类外设当 SPI 教材 |
| I2C 总线 | 多从机挂载、7 位地址；触摸 INT 为**预留能力**（当前阶段不启用）；BMP280 **不**再挂 I2C |
| 显示 | **1.3″ SH1106**，无内置字库依赖、无 TF；像素由 MCU 控制；动画帧预取模为 `const uint8_t[]` 存**内部 Flash**；月薪猫类动画仅 **8–15 帧**精简循环（完整帧放不下 64 KB） |
| 总线分工 | **SPI1**：传感器总线（BMP280 + LSM6DS3）；屏幕走 I2C，**不占 SPI** |
| UI 库（若日后引入） | 关闭文件系统与 GIF：`LV_USE_FILESYSTEM=0`、`LV_USE_GIF=0`；本仓库**尚未接入 LVGL** |

---

## 3. 外设清单与采购

价位为 **2026-08-28** 快照（OLED 以用户指定商品页为准）；店铺 SKU / 促销会变。下单前核对规格（针脚数、驱动芯片、供电电压）。

| 型号 | 协议 / 作用 | 接到 MCU | 参考价（元） | 购买入口 |
|------|-------------|----------|--------------|----------|
| SH1106 1.3 寸 OLED（4 针 IIC） | I2C1；显示 / 精简动画点阵 | VCC→3.3V，GND→GND；SCL→PB6，SDA→PB7 | 约 **11**（商品页 upstream 价位；随 SKU 波动） | [淘宝商品 id=797013563341](https://item.taobao.com/item.htm?id=797013563341) · [搜索：1.3寸OLED 4针 SH1106 IIC](https://s.taobao.com/search?q=1.3%E5%AF%B8OLED%204%E9%92%88%20SH1106%20IIC) |
| FT6236U 触摸盖板 | I2C1 + GPIO；坐标与 INT | SCL/SDA→PB6/PB7；INT→PB0；RST→PB1；VCC/GND | **暂不采购**；零售 0.96″ 稀缺，≥2.3″ 面板约 30+ | [淘宝搜索：FT6236U 电容触摸盖板 I2C 带INT RST 3.3V](https://s.taobao.com/search?q=FT6236U%20%E7%94%B5%E5%AE%B9%E8%A7%A6%E6%91%B8%E7%9B%96%E6%9D%BF%20I2C%20%E5%B8%A6INT%20RST%203.3V) |
| LSM6DS3 / LSM6DS3TR 模块 | SPI1 全双工；六轴 IMU（非存储） | **丝印**：SCL←PA5，SDA←PA7，SAO→PA6，CS←**PA8**（勿与 BMP280 共 PA3）；**3V3**/GND→面包板 3.3V 轨（ST-Link，勿接 VIN） | 已有模块 | 丝印见模块背面；手册 [reference/lsm6ds3](../reference/lsm6ds3/README.md) |
| BMP280 / BME280 | SPI1 4 线；温度气压 | SCK←PA5，SDI←PA7，SDO→PA6（**勿接地**），CSB←PA3；VCC/GND→3.3V 轨 | 约 3–6 | [淘宝搜索：BMP280 气压温度传感器模块 SPI 3.3V](https://s.taobao.com/search?q=BMP280%20%E6%B0%94%E5%8E%8B%E6%B8%A9%E5%BA%A6%E4%BC%A0%E6%84%9F%E5%99%A8%E6%A8%A1%E5%9D%97%20SPI%203.3V) |
| JY003 风扇模块 | TIM2 PWM 调速 | PWM/IN/SIG→**PA1**；电机 VCC 接模块自带电源（12V/5V）；GND→面包板 GND 轨 | 已有模块 | 以模块丝印为准；勿用 MCU 3.3V 给电机供电 |
| 830 孔面包板 | 实验底板 | — | 约 3.4–12 | [淘宝搜索：830孔面包板 带电源轨](https://s.taobao.com/search?q=830%E5%AD%94%E9%9D%A2%E5%8C%85%E6%9D%BF%20%E5%B8%A6%E7%94%B5%E6%BA%90%E8%BD%A8) |
| 40P 杜邦线套装 | 接线 | — | 约 6–8 | [淘宝搜索：40P杜邦线 公对公+公对母+母对母](https://s.taobao.com/search?q=40P%E6%9D%9C%E9%82%A6%E7%BA%BF%20%E5%85%AC%E5%AF%B9%E5%85%AC%2B%E5%85%AC%E5%AF%B9%E6%AF%8D%2B%E6%AF%8D%E5%AF%B9%E6%AF%8D) |
| ST-Link V2（迷你 5Pin） | SWD + 面包板电源轨 | SWDIO→PA13，SWCLK→PA14；GND/3.3V→面包板轨（方案 A：5V 闲置；勿接蓝板电源） | 已有 | [供电与共地](power-and-common-ground.md) |
| CH341 USB-TTL | 串口日志（已用） | RX←PA9，TX→PA10，GND→面包板 GND 轨 | 已有 | 见 [串口规则](../scripts-reference.md) |

### 采购核对

- OLED 买 **1.3 寸、4 针 IIC、驱动 SH1106**（分辨率多为 128×64）；**不要**买成 7 针 SPI，也**不要**默认当 SSD1306 驱动（GDDRAM 布局不同，错驱动会花屏/偏移）。
- 指定链接：[item.taobao.com/item.htm?id=797013563341](https://item.taobao.com/item.htm?id=797013563341)；下单选 **4 针 SH1106 IIC** SKU。
- **触控暂不启用**：FT6236 条目保留，当前阶段不接线、不采购；日后若启用，注意盖板尺寸需匹配 1.3″ 有效区。
- LSM6DS3 走 **SPI** 须接满 SCK / MOSI / MISO / CS；供电接面包板 **3V3** 勿接 VIN；勿买 W25Q/TF 当「SPI 练习」。CS 接 **PA8**，与 BMP280 的 PA3 分开。
- BMP280 **4 线 SPI**：CSB 由 MCU GPIO 拉低才是 SPI（悬空/拉高会进 I2C）；SDO=MISO，**禁止接地**（I2C 才用 SDO 选地址）。ID 寄存器 `0xD0`：BMP280=`0x58`，BME280=`0x60`。
- 模块丝印 **VCC/GND 顺序不一**，以板上丝印为准。
- 当前采购模块一律选 **3.3 V**（接面包板 3.3V 轨）；禁止把 3.3V 模块 VCC 接 5 V。面包板 5V 轨仅给明确标 5V 的外设（见 [供电与共地](power-and-common-ground.md)）。

### LSM6DS3 模块焊接（不必焊满全部焊盘）

常见拆板约 **11 个通孔焊盘**（两侧 6+5）+ 附带整排排针；**本仓库 SPI demo 不要求焊满**。

| 类别 | 丝印（以模块背面为准） | 是否必须焊/接 |
|------|------------------------|---------------|
| 供电 | `3V3`、`GND` | **必须**（`3V3` 接面包板 3.3V 轨 / ST-Link 3.3V，**勿接 VIN**） |
| SPI 四线 | `SCL`、`SDA`、`SAO`、`CS` | **必须**（→ PA5 / PA7 / PA6 / **PA8**） |
| 本 demo 不用 | `INT1`、`INT2`、`OCS`、`SCX`、`SDX` 等 | **可不焊、不接**（固件未用中断 / Sensor Hub） |
| 其他供电丝印 | `VIN`（若有） | **不接** |

实务建议：

1. **最少**：只焊上述 **6 个**焊盘（或只焊这 6 根针脚），杜邦线/面包板接到 MCU。
2. **图省事**：把附带排针整条焊上，未用脚悬空即可，不影响 SPI 轮询。
3. 焊完后核对丝印 → MCU 表见下文 [§4 LSM6DS3](#lsm6ds3)；源码表见 [`spi.c`](../../projects/f103-manual-reg/src/spi.c) 头注释。

---

## 4. 外设接线对照（分表）

按实现顺序书写；全部外设可同时接线，无冲突。每表列：模块脚 → MCU / 电源。占用总览见 §5。

### 板载 LED

**已实现**（两工程）。灌电流、低电平点亮常见；±3 mA。Backup 域须先 PWREN + DBP。[拉/灌](../learn/gpio-led-source-sink.md) · [八态](../learn/gpio-eight-modes.md)

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| LED（板载） | **PC13** | GPIO 推挽输出 |

### 外接 LED

**已实现**（`f103-manual-reg`）。拉电流；与 PC13 同步翻转、写相同电平（一亮一灭）。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| LED+（经 220 Ω） | **PB12** | GPIO 推挽输出 |
| LED- | 面包板 **GND** 轨 | — |

### 按键

**已实现**（`f103-manual-reg`）。片内上拉，低有效；按下 IDR=0。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| 一端 | **PB13** | GPIO 上拉输入 |
| 另一端 | 面包板 **GND** 轨 | — |

### 旋钮电位器

**已实现**（`f103-manual-reg`）。ADC12_IN0；SIG 接对面排针（非 B12 侧）。**勿**再作 TIM2_CH1 风扇 PWM。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| VCC | 面包板 **3.3V** 轨 | 勿接 5V |
| GND | 面包板 **GND** 轨 | — |
| SIG | **PA0** | 模拟输入 |

### USART1 / CH341

**已实现**。默认映射，无 AFIO remap。TTL 见 [uart-ttl-rs232-rs485.md](../learn/uart-ttl-rs232-rs485.md)。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| RX | **PA9**（USART1_TX） | CH341 RX←PA9 |
| TX | **PA10**（USART1_RX） | CH341 TX→PA10 |
| GND | 面包板 **GND** 轨 | 必须直接进地轨 |

### SWD

接线保留，**禁止**改普通 GPIO。[SWD ≠ USART](../learn/swd-vs-usart.md)

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| SWDIO | **PA13** | ST-Link → 蓝板 |
| SWCLK | **PA14** | ST-Link → 蓝板 |
| GND | 面包板 **GND** 轨 | 蓝板 SWD GND 从地轨短线接入 |

### SH1106

**已实现**（`f103-manual-reg`）。I2C1 默认映射，开漏 + 上拉。8 位写地址 **`0x78`**（本模块 R6 焊、R5 空；`0x78 == 0x3C<<1`），不再左移。BMP280 不挂此总线。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| VCC | 面包板 **3.3V** 轨 | — |
| GND | 面包板 **GND** 轨 | — |
| SCL | **PB6**（I2C1_SCL） | — |
| SDA | **PB7**（I2C1_SDA） | — |

### FT6236

**暂不使用**。日后若叠装：INT 下降沿置标志，主循环再 I2C 读坐标。7 位地址常见 `0x38`（ADDR 可改 `0x39`）。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| VCC | 面包板 **3.3V** 轨 | 当前阶段可不接 |
| GND | 面包板 **GND** 轨 | 当前阶段可不接 |
| SCL | **PB6**（与 SH1106 并联） | 当前阶段可不接 |
| SDA | **PB7**（与 SH1106 并联） | 当前阶段可不接 |
| INT | **PB0** | 当前阶段可不接 |
| RST | **PB1** | 当前阶段可不接 |

### BMP / BME280

**已实现**（`f103-manual-reg`）。与 LSM6DS3 **共用** SPI1 三线（PA5/PA7/PA6），片选独立。SPI **Mode 0**。读 24 字节校准后补偿温度/气压（无湿度）。ID 寄存器 `0xD0`：BMP280=`0x58`，BME280=`0x60`。短线直连，不加 4.7kΩ。屏右下显示温度。

| 模块脚 | MCU / 电源 | 说明 |
|--------|------------|------|
| VCC | 面包板 **3.3V** 轨 | ≤3.6V，禁止 5V |
| GND | 面包板 **GND** 轨 | 必须共地 |
| SCK | **PA5**（SPI1_SCK） | 与 LSM6 并联 |
| SDI | **PA7**（SPI1_MOSI） | MCU → 传感器 |
| SDO | **PA6**（SPI1_MISO） | 传感器 → MCU；**禁止接地** |
| CSB | **PA3**（GPIO，低有效） | 空闲必须高；悬空会进 I2C |

### LSM6DS3

**固件已实现**；硬件尚未接。SPI **Mode 3**。WHO_AM_I 期望 `0x69`。SCK/MOSI/MISO 与 BMP280 并联。CS=PA8。

| 模块丝印 | MCU / 电源 | 说明 |
|----------|------------|------|
| 3V3 | 面包板 **3.3V** 轨 | 勿接 VIN |
| GND | 面包板 **GND** 轨 | 必须共地 |
| SCL | **PA5**（SPI1_SCK） | 与 BMP280 并联 |
| SDA | **PA7**（SPI1_MOSI） | MCU → 传感器 |
| SAO | **PA6**（SPI1_MISO） | 传感器 → MCU |
| CS | **PA8**（GPIO，低有效） | 勿与 BMP280 共 PA3 |
| INT1 / INT2 / OCS / SCX / SDX | 不接 | 本 demo 不用 |

### JY003

**阶段 4**。TIM2 默认 remap，CH2=PA1。电机电源独立，只共地。PWM 脚以模块丝印为准。

| 模块脚（以丝印为准） | MCU / 电源 | 说明 |
|----------------------|------------|------|
| PWM / IN / SIG | **PA1**（TIM2_CH2） | 3.3V 逻辑 PWM；勿接 PA0（旋钮） |
| 电机 VCC / VIN | **模块自带 12V 或 5V** | 禁止面包板 3.3V / GPIO 供电 |
| GND | 面包板 **GND** 轨 | 只共地，不共 3.3V |

---

## 5. 引脚冲突核对

依据 **RM0008 Rev 21** Table 54–56 / `AFIO_MAPR`（本地 PDF：`doc/reference/stm32f103/pdf/`，可用 `./scripts/fetch-stm32f103-docs.sh` 下载）。

| 外设 | 默认映射（REMAP=0） | REMAP=1 | 本方案 |
|------|---------------------|---------|--------|
| USART1 | TX=PA9，RX=PA10 | TX=PB6，RX=PB7 | **默认**（与现有固件一致） |
| I2C1 | SCL=PB6，SDA=PB7 | SCL=PB8，SDA=PB9 | **默认**（OLED / 可选触摸） |
| SPI1 | NSS=PA4，SCK=PA5，MISO=PA6，MOSI=PA7 | NSS=PA15，SCK=PB3，… | **默认**；PA3 / PA8 作 GPIO CS，不用硬件 NSS |
| TIM2 | CH1=PA0，CH2=PA1，CH3=PA2，CH4=PA3 | 部分/全 remap 到 PA15/PB3/PB10/PB11 | **默认**；风扇用 CH2=PA1，不 remap |

**同时可用的条件：**

1. 保持 USART1 / SPI1 / I2C1 **默认映射**；日后**勿**置位 `USART1_REMAP`（否则与 I2C1 抢 PB6/PB7）。
2. **不启用 USART2**（默认会占 PA0–PA4，与旋钮 / SPI1 冲突）。
3. PA3、PA8 作软件 CS，不依赖硬件 NSS；同时只拉低一个 CS。
4. TIM2 **不 remap**（风扇 PA1=CH2；PA0 留给 ADC）。
5. 不占用 PD0/PD1（多数核心板 HSE）。
6. PA13/PA14 保持 SWD。
7. **不用 SPI2**（默认 SCK=PB13，已被按键占用）。

占用汇总（目标接法，无重叠）：

```text
PC13              板载 LED（已实现）
PB12              外接 LED（已实现，f103-manual-reg，拉电流）
PB13              按键（已实现，f103-manual-reg，上拉输入）
PA0               ADC1 CH0 旋钮 SIG（已实现）
PA1               TIM2_CH2 → JY003 PWM（阶段 4）
PA3               BMP280 CSB（已实现）
PA4               空闲（面包板孔不可用，勿再作 CS）
PA5, PA6, PA7     SPI1 SCK/MISO/MOSI（BMP280 + LSM6 并联）
PA8               LSM6DS3 CS（阶段 3）
PA9, PA10         USART1（已实现）
PA13, PA14        SWD
PB0, PB1          FT6236 INT / RST（预留，暂不使用）
PB6, PB7          I2C1：SH1106（已实现，写地址 0x78）
```

### 引脚可行性（阶段 1 核对）

结论：**目标接法可行**，不改已接线外设。依据如下。

- **SPI1 三线并联**：DS5319 Table 5 默认映射 PA5=SCK、PA6=MISO、PA7=MOSI。两从机各自 CS；空闲 CS 高时从机 SDO 应呈高阻（BMP280 / LSM6 手册均如此）。短线直连，SPI **不加** 4.7 kΩ 上拉。
- **PA3 = BMP280 CS**：固件把 PA3 配成推挽 GPIO。BST-BMP280-DS001：CSB 拉低选 SPI；CSB 接高/悬空走 I2C。空闲必须输出高。SDO 在 SPI 下是 MISO，**接地则读不出 ID**。PA4 因面包板孔不可用，不再当 CS。
- **PA8 = LSM6 CS**：DS5319 Table 5 默认 USART1_CK / TIM1_CH1 / MCO。本工程 USART1 异步、未开 TIM1、未出 MCO，PA8 空闲。作普通推挽 GPIO 即可，不必 remap。FT 脚输出仍是 3.3 V。
- **SPI 模式**：LSM6 DocID026899 Table 6 / Figure 3 为 **Mode 3**（空闲时钟高）。BMP280 支持 Mode **00** 与 **11**（CSB 下降沿时 SCK 电平自动选）。计划 BMP280 用 Mode 0，故阶段 2 须在两次访问之间关 `SPE`、改 `CR1.CPOL/CPHA`。BMP280 也能用 Mode 11（与 LSM6 相同），那是备选，本方案不采用。
- **时钟**：SPI1 DIV16 ≈ 4.5 MHz，低于两芯片 10 MHz 上限。
- **PA1 = 风扇 PWM**：RM0008 TIM2 remap 表，`TIM2_REMAP=00` 时 CH2=PA1。参考 demo 的 PA0=TIM2_CH1 已被旋钮占用。PA1 未占用、非 FT，PWM 为 3.3 V 逻辑，**不能**给电机供电。
- **JY003**：公开资料几乎只有商品名，引脚以模块丝印（PWM / IN / SIG）为准。同类 PWM 风扇/驱动常见 1–20 kHz；阶段 4 用 20 kHz。电机电源独立，GND 并入面包板轨。ST-Link 3.3V ≈150 mA，带不动风扇。
- **排除**：SPI2（PB13=按键）；SPI1 remap 到 PB3/PB4（复位属 JTAG）；USART2（抢 PA0–PA4）。

---

## 6. 连接图与供电

规范全文：[供电、共地与 SWD](power-and-common-ground.md)。

```
电脑 USB ──┬──> 蓝板 MicroUSB（MCU 独立供电）
           └──> ST-Link USB

                      ┌────────────────────┐
                      │    ST-Link V2      │
                      │ SWDIO ─────────────┼───> 蓝板 PA13
                      │ SWCLK ─────────────┼───> 蓝板 PA14
                      │ GND ───────────────┼───> 面包板 GND 轨（汇集点）
                      │ 3.3V ──────────────┼───> 面包板 3.3V 轨
                      │ 5V                 │    方案 A 闲置（标 5V 模块才接 5V 轨）
                      └────────────────────┘
                      【禁止】ST-Link 3.3V/5V → 蓝板任何电源脚
                      【禁止】ST-Link GND → 蓝板 → 面包板（地线串联）

┌─────────────────────────────────────────────────────────────┐
│  830 面包板                                                  │
│  +3.3V 轨 ←── ST-Link 3.3V                                   │
│  +5V 轨   ←── 方案 A：ST-Link 5V 闲置                        │
│  GND 轨   ←── ST-Link GND、CH341 GND、蓝板 SWD GND、外设 GND │
│                                                              │
│  STM32F103C8T6 蓝板（MicroUSB 供电；不接 ST-Link 电源）     │
│    SWD GND ← 面包板 GND 轨（短线）                           │
│    PB6  → I2C1_SCL ──┬── SH1106 SCL [/ FT6236]             │
│    PB7  → I2C1_SDA ──┴── SH1106 SDA [/ FT6236]             │
│    PB0  → FT6236 INT   （暂不接线）                          │
│    PB1  → FT6236 RST   （暂不接线）                          │
│    PA5  → SPI1_SCK  ──┬── BMP280 SCK / LSM6DS3 SCL           │
│    PA7  → SPI1_MOSI ──┤── BMP280 SDI / LSM6DS3 SDA           │
│    PA6  ← SPI1_MISO ──┴── BMP280 SDO / LSM6DS3 SAO（SDO 勿接地）│
│    PA3  → BMP280 CSB                                         │
│    PA8  → LSM6DS3 CS（阶段 3）                               │
│    PA1  → JY003 PWM（目标，阶段 4；TIM2_CH2）                │
│    PA9  → CH341 RX（USART1_TX）                              │
│    PA10 ← CH341 TX（USART1_RX）                              │
│    PC13 → 板载 LED                                           │
│    PB12 → 220 Ω → 外接 LED+（LED- → GND 轨）                 │
│    PB13 ← 按键 → GND 轨（片内上拉）                          │
│                                                              │
│  1.3″ SH1106 4 针：VCC/GND→3.3V 轨；SCL/SDA→PB6/PB7        │
│  FT6236：当前阶段不接；日后 INT→PB0，RST→PB1                │
│  BMP280：3.3V/GND→3.3V 轨；四线 SPI 如上；CSB 空闲高         │
│  LSM6DS3：3V3/GND→3.3V 轨（勿接 VIN）；CS→PA8                │
│  JY003：PWM→PA1；电机电源独立；GND→面包板 GND 轨             │
└─────────────────────────────────────────────────────────────┘

供电：电脑 USB → 蓝板 MicroUSB（MCU）；电脑 USB → ST-Link → 面包板 3.3V（外设；5V 方案 A 闲置）
共地：面包板 GND 轨汇集 ST-Link / CH341 / 蓝板 SWD / 外设（星型，禁止串联）
串口：CH341 GND 必须直接进面包板 GND 轨，否则乱码
SPI：BMP280 + LSM6DS3 共用三线、独立 CS；屏幕不占 SPI
风扇：电机电源独立，只并 GND；禁止 MCU 3.3V 给风扇供电
```

---

## 7. 注意事项

1. **电压**：当前模块一律 3.3 V，接面包板 3.3V 轨（ST-Link），勿接 5 V / VIN（LSM6DS3 `Vdd` 1.71–3.6 V）。蓝板 MicroUSB 独立供电；ST-Link 电源勿接蓝板。
2. **共地**：面包板 GND 轨为汇集点；ST-Link / CH341 / 蓝板 SWD / 外设 GND 均接该轨，禁止串联（[供电与共地](power-and-common-ground.md)）。
3. **SWD**：PA13、PA14 保持调试功能（[SWD ≠ USART](../learn/swd-vs-usart.md)）；只接信号 + GND，不接 ST-Link 电源到蓝板。
4. **I2C**：SCL/SDA 需上拉（多数模块已焊 ~4.7–10 kΩ）；缺上拉则总线卡死。多从机靠不同 7 位地址区分。
5. **SH1106**：常见 128×64 可视区，片内 GDDRAM 多为 **132×64**，驱动须按 SH1106 处理列偏移；勿直接套用 SSD1306 初始化序列。上图步骤与 1024 B 页格式见 [SH1106 · 显示图像](../reference/sh1106/README.md#显示图像)。
6. **触控**：条目保留，**当前阶段不启用**；日后叠装时 INT 下降沿置标志，主循环再 I2C 读坐标。
7. **SPI / LSM6DS3**：Full-Duplex Master；4-wire（须接 MISO/SAO）；手册 **Mode 3**。CS=**PA8**。
8. **SPI / BMP280**：同一总线；CSB=**PA3**；Mode 0（也可 Mode 11，本方案不用）；SDO **禁止接地**；CSB 空闲高。
9. **禁止**用 W25Q / TF 当 SPI「学习外设」。
10. **JY003**：PWM 只接 PA1；电机电源独立；只共地。
11. **Flash 预算**：动画帧进内部 Flash；完整高帧数动画放不下，限 8–15 帧精简循环。
12. **与固件**：`f103-manual-reg` 已驱动 SPI1（PA3=BMP280 CS、PA8=LSM6 CS）与 I2C1/SH1106；风扇 PWM 为阶段 4。

---

## 8. 固件 / Cube 配置摘要（备查）

| 项 | 建议 |
|----|------|
| RCC | HSE 8 MHz × PLL×9 → 72 MHz（与现工程一致） |
| SYS Debug | Serial Wire（保留 SWD）；见 [swd-vs-usart.md](../learn/swd-vs-usart.md) |
| USART1 | PA9/PA10 异步，**不 remap** |
| SPI1 | Full-Duplex Master；PA5/PA6/PA7；软件 CS：PA3=BMP280（Mode 0）、PA8=LSM6（Mode 3） |
| I2C1 | PB6/PB7，**不 remap**；400 kHz Fast Mode（`f103-manual-reg` 已实现） |
| TIM2 | 默认映射；CH2=PA1 PWM（阶段 4）；不占用 PA0 |
| GPIO | PB0 / PB1 预留给 FT6236（**当前可不初始化**） |
| 业务划分 | SPI1：BMP280 + LSM6DS3；I2C：SH1106；FT6236 触摸延后；风扇独立电源 |
| LVGL（若引入） | `LV_USE_FILESYSTEM 0`；`LV_USE_GIF 0`；`LV_COLOR_DEPTH 1`；`LV_MEM_SIZE` 约 7 KB —— **未接入本仓库** |

---

## 9. 出处

| 资料 | 用途 |
|------|------|
| [DS5319](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) / [本地索引](../reference/stm32f103/md/datasheet-index.md) | 引脚、封装 |
| [RM0008](https://www.st.com/resource/en/reference_manual/cd00171190.pdf) Table 54–56、`AFIO_MAPR` | USART1 / I2C1 / SPI1 默认与 remap |
| SH1106 控制器手册（厂商 PDF） | 1.3″ OLED I2C、128×64 / 132×64 GDDRAM |
| [FT6x36](https://buydisplay.com/download/ic/FT6236-FT6336-FT6436L-FT6436_Datasheet.pdf) | 触摸 I2C、INT、RST（预留） |
| [LSM6DS3 DocID026899](../reference/lsm6ds3/README.md) | SPI 4-wire、Mode 3、供电、WHO_AM_I=`0x69` |
| [BMP280 BST-BMP280-DS001](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf) | 4 线 SPI Mode 00/11、CSB 选接口、id=`0x58`；SDO 勿接地 |
| RM0008 Table 45 | TIM2 默认 CH2=PA1 |
| 本仓库 `f103-manual-reg` 源码 | 已实现 PC13、USART1、ADC1/PA0、SPI1（PA3 BMP280 / PA8 LSM6） |
| [淘宝商品 797013563341](https://item.taobao.com/item.htm?id=797013563341)（2026-08-28） | 1.3″ SH1106 4 针采购入口 |
