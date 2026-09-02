# 供电、共地与 SWD 接线

> 本仓库硬件供电规范。STM32F103C8T6 蓝板用 **MicroUSB 独立供电**；ST-Link 迷你 5Pin **只做下载调试**，并把 3.3V（必要时 5V）与 GND 拉到面包板作外设电源轨。**共地以面包板 GND 轨为唯一汇集点（星型），不可省略。**

相关：[外设与接线](stm32f103-peripherals.md) · [快速上手](../getting-started.md) · [SWD ≠ USART](../learn/swd-vs-usart.md) · [probe-rs](../probe-rs.md)

---

## 需求确认

整套设备：**STM32 蓝板（MicroUSB 独立供电）+ ST-Link + CH341 USB-TTL + 全部 3.3V 外设**。

1. **蓝板**：MicroUSB 单独供电 MCU（独立电源，不用 ST-Link 给 MCU 供电）
2. **ST-Link**：只负责 SWD 下载调试；**3.3V / GND → 面包板**。当前推荐 **方案 A**：ST-Link **5V 闲置不用**（本仓库模块走 3.3V 轨）；日后明确标 5V 的件再拉 5V 轨
3. **必须共地**：所有设备 GND **汇集到同一条面包板地线轨**，不要串联长地线、不要多点分头造成地环路

共地只是把电位拉到同一参考点，**不会互相供电**。缺少公共地时，串口 / I2C / SPI 易乱码、模块不工作。

---

## 1. 两套硬件引脚

### 1.1 STM32F103C8T6 蓝板（MicroUSB 供电）

- 供电来源：电脑 → 蓝板 MicroUSB，**与 ST-Link 电源互相独立**
- 蓝板只接调试信号线，**不接入** ST-Link 的 3.3V / 5V
- 蓝板 GND：**从面包板公共 GND 轨**引一根短线到 SWD 排针旁 GND（不要 ST-Link GND 直连蓝板后再串到面包板）
- 蓝板 MicroUSB 自带的 GND 由电脑 USB 内部连通，无需再为 USB 口单独拉地

| 蓝板引脚 | 连接到 | 功能说明 |
|----------|--------|----------|
| PA13 | ST-Link SWDIO | 调试数据 |
| PA14 | ST-Link SWCLK | 调试时钟 |
| GND | **面包板 GND 轨** | **共地（必须接）** |
| NRST（可选） | ST-Link RESET | 软件复位；不接也可下载调试 |

**严禁**把 ST-Link 的 3.3V / 5V 接到蓝板任何电源引脚，两套电源不要并联。

### 1.2 ST-Link V2（迷你 5Pin）

| 丝印 | 功能 | 输出能力 | 用途 |
|------|------|----------|------|
| SWDIO | 调试数据 | 信号，无供电 | → 蓝板 PA13 |
| SWCLK | 调试时钟 | 信号，无供电 | → 蓝板 PA14 |
| GND | 公共参考 0 V | 地线 | **只接到面包板 GND 轨**（主汇集点） |
| **3.3V** | 3.3 V 电源输出 | 约 ≤150 mA | → 面包板 3.3V 轨（本仓库传感器） |
| **5V** | 5 V 电源输出 | 约 ≤300 mA（来自电脑 USB） | 方案 A **闲置**；仅明确标 5V 的模块再接面包板 5V 轨 |

---

## 2. 每一路 GND 怎么接

**核心原则**：面包板地线轨是公共汇集点。各设备用**短线**接到该轨，不要地线串联。

| 设备 | GND 接到 | 不要 |
|------|----------|------|
| **ST-Link** | 面包板 GND 轨 | 先接蓝板、再从蓝板引到面包板（串联） |
| **CH341** | **直接**接面包板公共 GND 轨 | 用 CH341 自身 5V/3.3V 给 MCU 或外设供电 |
| **蓝板** | 从面包板 GND 轨引一条短线到 SWD 排针 GND | ST-Link GND 只接蓝板、面包板悬空 |
| **3.3V 外设**（LSM6DS3 等） | 就近接面包板 GND 轨；VCC 接面包板 3.3V 轨 | 接 VIN / 5V / 蓝板电源脚 |

CH341 本仓库只作 USB↔TTL：**RX←PA9、TX→PA10、GND**。GND 进面包板轨；信号可经面包板再到蓝板 IO。日后若换成 CH347 等 USB-TTL，**共地接法不变**（转换器 GND 仍进面包板公共地）。

---

## 3. 拓扑简图（星型汇集）

```text
面包板 GND 轨（公共汇集点）
├─ ST-Link GND
├─ CH341 GND
├─ 蓝板 SWD 排针 GND
└─ 各个 3.3V 外设 GND

面包板 3.3V 电源轨（仅外设供电）
└─ ST-Link 3.3V（只进面包板，不接蓝板）

面包板 5V 电源轨
└─ 方案 A：ST-Link 5V 闲置；标 5V 的件才接此轨

信号线：
  ST-Link SWDIO / SWCLK → 蓝板 PA13 / PA14
  CH341 RX ← 蓝板 PA9，CH341 TX → 蓝板 PA10
  蓝板 GPIO → 面包板外设信号脚（见 stm32f103-peripherals.md）

电脑 USB → 蓝板 MicroUSB（MCU 独立供电）
电脑 USB → ST-Link USB（调试器 + 面包板 3.3V）
```

---

## 4. 供电负载限制

| 轨 | 约上限 | 适合 | 不适合 |
|----|--------|------|--------|
| ST-Link 3.3V | ≈150 mA | OLED、LSM6DS3、BMP280、BH1750 等小电流传感器 | 继电器、舵机、大功率 LED、电机 |
| ST-Link 5V | ≈300 mA | 小型 5V 传感器、小 LED 灯组（本仓库当前不用） | 大电流执行器件 |

超过上限：电压跌落、ST-Link 发热、外设死机、调试掉线。大电流外设需**单独外接电源**，只把外部电源 GND 接入面包板公共 GND 轨。

---

## 5. 实操接线顺序

1. 先在面包板拉出一条连续的横向地线轨  
2. ST-Link 的 GND 接到这条地线轨  
3. CH341 的 GND 也接到同一条地线轨  
4. 从地线轨引一根**短线**连到蓝板 SWD 排针旁的 GND  
5. 所有传感器模块的 GND 全部就近接这条地线轨  
6. ST-Link 3.3V → 面包板 3.3V 轨（**严禁**连蓝板 VDD / 5V / 3.3V）  
7. 最后接入信号线（SWD、USART1、SPI、I2C）  
8. 上电前复查：蓝板已插 MicroUSB；ST-Link 电源只进面包板  

---

## 6. 两种错误接法

### 错误 1：串联地线

`ST-Link GND → 蓝板 GND → 面包板 → CH341 GND`

地线逐级串联，电流依次流过，产生电位差，I2C / 串口易出现随机乱码。

### 错误 2：只接信号线、不接 GND

CH341 只接 TX/RX，或传感器只接 SCK/MOSI，不连 GND，大概率通信异常。

---

## 7. 实操规范与避坑

1. **两套电源互相独立**  
   蓝板由 MicroUSB 供电；ST-Link 只向外设供电。不要把 ST-Link 电源接到蓝板 VDD / 5V，避免倒灌损坏硬件。

2. **共地：星型，禁止串联**  
   面包板 GND 轨为汇集点。缩短地线杜邦线；串口 / I2C 偶发乱码时优先复查地线。

3. **GPIO 不能当电源**  
   传感器供电只用面包板上 ST-Link 引出的 3.3V（或明确的 5V 轨），不要用 PA/PB/PC 给模块供电。

4. **电平兼容**  
   - 3.3V 外设：VCC→面包板 3.3V，信号直接连 STM32 IO  
   - 5V 外设输出接到 STM32：即使部分脚标 FT，建议加分压，防止击穿

5. **SWD 引脚**  
   PA13 / PA14 仅 SWD（见 [swd-vs-usart.md](../learn/swd-vs-usart.md)），禁止改普通 GPIO 或串口。固件与文档不得占用这两脚。

6. **CH341 不供电**  
   CH341 只接信号 + 地；不要用它给蓝板或传感器供电。

---

## 8. 场景模板

### 场景 1：3.3V 传感器（LSM6DS3 / OLED / MPU6050）— 本仓库默认

| 模块脚 | 接到 |
|--------|------|
| VCC / 3V3 | 面包板 3.3V（ST-Link 3.3V） |
| GND | 面包板 GND 轨 |
| 信号（SDA/SCL、SPI 等） | 蓝板对应 GPIO（见 [stm32f103-peripherals.md](stm32f103-peripherals.md)） |

地线经面包板 GND 轨共地。

### 场景 2：5V 模块（继电器、5V 光电传感器）

| 模块脚 | 接到 |
|--------|------|
| VCC | 面包板 5V（ST-Link 5V；方案 A 未用时才接此轨） |
| GND | 面包板 GND 轨 |
| 信号输出 | STM32 GPIO（建议分压） |

本仓库当前采购清单以 3.3V 模块为主；5V 轨预留给日后明确标 5V 的外设。

---

## 9. 大负载升级（可选）

外设总电流超过 ST-Link 输出上限时：

1. 外设改用独立 USB / 外接电源供电  
2. 独立电源 GND 接入面包板公共 GND 轨  
3. 信号仍接单片机 IO  
4. ST-Link 仅做调试，不再承担外设供电  

---

## 出处与交叉引用

| 文档 | 用途 |
|------|------|
| [stm32f103-peripherals.md](stm32f103-peripherals.md) | 外设 IO、采购、连接图 |
| [getting-started.md](../getting-started.md) | 新手接线步骤 |
| [probe-rs.md](../probe-rs.md) | SWD 烧录 |
| [swd-vs-usart.md](../learn/swd-vs-usart.md) | PA13/PA14 ≠ 串口 |
| [gpio-protection-and-negative-voltage.md](../learn/gpio-protection-and-negative-voltage.md) | 共地与负压风险 |
| `.cursor/rules/hardware-power.mdc` | Agent 短规则 |
