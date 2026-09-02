# 供电、共地与 SWD 接线

> 本仓库硬件供电规范。STM32F103C8T6 蓝板用 **MicroUSB 独立供电**；ST-Link 迷你 5Pin **只做下载调试**，并把 3.3V / 5V / GND 拉到面包板作外设电源轨。**共地不可省略。**

相关：[外设与接线](stm32f103-peripherals.md) · [快速上手](../getting-started.md) · [SWD ≠ USART](../learn/swd-vs-usart.md) · [probe-rs](../probe-rs.md)

---

## 需求确认

1. **蓝板**：MicroUSB 单独供电（独立电源，不用 ST-Link 给 MCU 供电）
2. **ST-Link**：只负责下载调试；引出的 5V、3.3V、GND 接到面包板，作为外设电源轨
3. **必须共地**（否则串口 / I2C / SPI 通信易乱码、模块不工作）

---

## 1. 两套硬件引脚

### 1.1 STM32F103C8T6 蓝板（MicroUSB 供电）

- 供电来源：电脑 → 蓝板 MicroUSB，**与 ST-Link 电源互相独立**
- 蓝板只接调试信号线，**不接入** ST-Link 的 3.3V / 5V

| 蓝板引脚 | 连接到 ST-Link | 功能说明 |
|----------|----------------|----------|
| PA13 | SWDIO | 调试数据 |
| PA14 | SWCLK | 调试时钟 |
| GND | GND | **共地（必须接）** |
| NRST（可选） | RESET | 软件复位；不接也可下载调试 |

**严禁**把 ST-Link 的 3.3V / 5V 接到蓝板任何电源引脚，两套电源不要并联。

### 1.2 ST-Link V2（迷你 5Pin）

| 丝印 | 功能 | 输出能力 | 用途 |
|------|------|----------|------|
| SWDIO | 调试数据 | 信号，无供电 | → 蓝板 PA13 |
| SWCLK | 调试时钟 | 信号，无供电 | → 蓝板 PA14 |
| GND | 公共参考 0 V | 地线 | ① 连蓝板 GND；② 拉到面包板 GND 轨 |
| **3.3V** | 3.3 V 电源输出 | 约 ≤150 mA | → 面包板 3.3V 轨（小传感器） |
| **5V** | 5 V 电源输出 | 约 ≤300 mA（来自电脑 USB） | → 面包板 5V 轨（标 5V 的模块） |

面包板电源轨直接取自 ST-Link：**5V、3.3V、GND**。

---

## 2. 完整接线拓扑

```text
电脑 USB → ST-Link
 ├─ 信号：SWDIO → PA13，SWCLK → PA14
 ├─ GND：一路连蓝板 GND；另一路拉到面包板 GND
 ├─ 3.3V → 面包板 3.3V 轨（给 3.3V 外设）
 └─ 5V  → 面包板 5V 轨（给 5V 外设）

电脑 USB → 蓝板 MicroUSB（独立给 MCU 供电）
蓝板 GPIO 信号线 → 面包板外设信号引脚
```

**核心要点**：蓝板 GND 与面包板 GND 经 ST-Link GND 连通。缺少这条地线时，MCU 与外设没有共同电压基准，串口 / I2C / SPI 大概率乱码或模块不工作。

CH341 USB-TTL：只接 **RX←PA9、TX→PA10、GND**；GND 必须进同一公共地（蓝板 / 面包板 / ST-Link）。CH341 **不从** ST-Link 取电。

本仓库现有传感器（LSM6DS3、计划 OLED / BMP280 等）接 **面包板 3.3V 轨**，勿接 VIN / 5V。

---

## 3. 供电负载限制

| 轨 | 约上限 | 适合 | 不适合 |
|----|--------|------|--------|
| ST-Link 3.3V | ≈150 mA | OLED、MPU6050、LSM6DS3、BH1750 等小电流传感器 | 继电器、舵机、大功率 LED、电机 |
| ST-Link 5V | ≈300 mA | 小型 5V 传感器、小 LED 灯组 | 大电流执行器件 |

超过上限：电压跌落、ST-Link 发热、外设死机、调试掉线。大电流外设需**单独外接电源**，只把外部电源 GND 接入面包板公共 GND 轨。

---

## 4. 实操规范与避坑

1. **两套电源互相独立**  
   蓝板由 MicroUSB 供电；ST-Link 只向外设供电。不要把 ST-Link 电源接到蓝板 VDD / 5V，避免倒灌损坏硬件。

2. **共地不可省略**  
   ST-Link 的 GND 同时接蓝板 GND 与面包板 GND，**一根地连通三处**；CH341 GND 也接入同一地。

3. **GPIO 不能当电源**  
   传感器供电只用面包板上 ST-Link 引出的 5V / 3.3V，不要用 PA/PB/PC 给模块供电。

4. **电平兼容**  
   - 3.3V 外设：VCC→面包板 3.3V，信号直接连 STM32 IO  
   - 5V 外设输出接到 STM32：即使部分脚标 FT，建议加分压，防止击穿

5. **SWD 引脚**  
   PA13 / PA14 仅 SWD（见 [swd-vs-usart.md](../learn/swd-vs-usart.md)），禁止改普通 GPIO 或串口。固件与文档不得占用这两脚。

---

## 5. 场景模板

### 场景 1：3.3V 传感器（LSM6DS3 / OLED / MPU6050）

| 模块脚 | 接到 |
|--------|------|
| VCC / 3V3 | 面包板 3.3V（ST-Link 3.3V） |
| GND | 面包板 GND |
| 信号（SDA/SCL、SPI 等） | 蓝板对应 GPIO（见 [stm32f103-peripherals.md](stm32f103-peripherals.md)） |

地线经公共 GND 自动共地。

### 场景 2：5V 模块（继电器、5V 光电传感器）

| 模块脚 | 接到 |
|--------|------|
| VCC | 面包板 5V（ST-Link 5V） |
| GND | 面包板 GND |
| 信号输出 | STM32 GPIO（建议分压） |

本仓库当前采购清单以 3.3V 模块为主；5V 轨预留给日后明确标 5V 的外设。

---

## 6. 大负载升级（可选）

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
