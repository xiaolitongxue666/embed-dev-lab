# 拉电流 / 灌电流驱动 LED（STM32F103，3.3 V）

整理自学习笔记，并对照本地 [DS5319](../reference/stm32f103/md/datasheet-index.md) Rev 20 电气章节。说明 **推挽 GPIO 点亮 LED 的两种接线**、电流方向、限流电阻，以及与本仓库板载 **PC13 LED** 的关系。GPIO 模式见 [gpio-eight-modes.md](gpio-eight-modes.md)；PC13 Backup 限制见 [backup-domain-pc13.md](../reference/stm32f103/md/topics/backup-domain-pc13.md)。

---

## 一句话总结

拉电流 = 电流从引脚 **流出**（输出高、PMOS 接到 VDD）；灌电流 = 电流 **流入** 引脚（输出低、NMOS 接到 VSS）。两种接法都可行，**都必须串联限流电阻**。F103 普通 GPIO 的拉/灌额定是 **对称** 的；板载 PC13 LED 是灌电流，且 Backup 开关把 PC13–PC15 限制在约 **±3 mA**。

---

## 1. 两种接线拓扑

约定：**电流方向 = 正电荷移动方向（电路标准定义）**。驱动脚均用 **推挽输出**。

### 1.1 拉电流（电流流出 GPIO）

接线：`GPIO 推挽 → 限流电阻 → LED 正极，LED 负极 → GND`

- 点亮条件：GPIO 输出高电平（约 3.3 V）
- 电流路径：芯片内部 VDD → **PMOS 上拉管** → 引脚向外流出 → 电阻 → LED → GND
- 形象理解：引脚把电流 **拉出来**

```text
【拉电流】
MCU PIN(3.3V) ──→── R ──→── LED+    LED- ──→── GND
               电流流出引脚
```

### 1.2 灌电流（电流流入 GPIO）

接线：`3.3 V → 限流电阻 → LED 正极，LED 负极 → GPIO 推挽`

- 点亮条件：GPIO 输出低电平（约 0 V）
- 电流路径：3.3 V → 电阻 → LED → **流入引脚** → **NMOS 下拉管** → 芯片内部 VSS
- 形象理解：外部电流 **灌进** 引脚

```text
【灌电流】
3.3V ──→── R ──→── LED+    LED- ──→── MCU PIN(0V)
                                 电流流入引脚
```

拉 / 灌只是电流进出引脚的方向不同，由推挽输出时内部导通的 MOS 管决定。

---

## 2. DS5319 电气参数（勿用 10 mA / 20 mA 口诀）

教材里常见「拉 10 mA、灌 20 mA」（按 51 / AVR 一类不对称口诀）。**STM32F103 不是这样。** 以本地 DS5319 Rev 20 为准。

### 2.1 工作额定（§5.3.13 Output driving current，PDF p.63）

> The GPIOs … can **sink or source up to ±8 mA**, and sink or source up to **±20 mA (with a relaxed VOL/VOH)** except **PC13, PC14 and PC15**, which can sink or source up to **±3 mA**.

| 对象 | 拉电流（source） | 灌电流（sink） | 说明 |
|------|------------------|----------------|------|
| 普通 GPIO | **±8 mA** | **±8 mA** | 保证 CMOS `VOL ≤ 0.4 V` / `VOH ≥ VDD−0.4 V`（Table 37，8 脚同时） |
| 普通 GPIO（放宽电平） | **±20 mA** | **±20 mA** | `VOL` 可到 **1.3 V**，`VOH` 可到 **VDD−1.3 V**（Table 37，characterization） |
| **PC13–PC15** | **±3 mA** | **±3 mA** | Backup 开关供电；输出建议 ≤ 2 MHz、负载 ≤ 30 pF |

拉与灌在手册里 **同一组数字**，不是「PMOS 10 mA、NMOS 20 mA」。

### 2.2 绝对最大值（Table 7，PDF p.36–37）

| 符号 | 含义 | Max |
|------|------|-----|
| `IIO` | 任一 I/O / 控制脚灌入或拉出 | **±25 mA** |
| `IVDD` | 流入 VDD/VDDA 的总电流 | **150 mA** |
| `IVSS` | 流出 VSS 的总电流 | **150 mA** |

绝对最大值不是设计目标。超过后手册不保证器件完好。多脚同时点灯时，还要合计不超过 `IVDD` / `IVSS`。

### 2.3 实验电流怎么取

| 场景 | 建议工作电流 | 原因 |
|------|--------------|------|
| 面包板外接 LED（普通 GPIO，非 PC13–15） | **5～8 mA** | 落在 ±8 mA 保证电平区内，留余量 |
| 普通 GPIO 想「更亮」 | 仍避免把 **20 mA** 当日常目标 | 20 mA 已是放宽 `VOL`/`VOH`；`VOL=1.3 V` 时 LED 压差变小 |
| **板载 / 外接占用 PC13** | **远低于 3 mA** | Backup 开关限流；手册写明不宜当电流源驱 LED |
| 任何接法 | **禁止裸接 LED** | 见下一节 |

---

## 3. 为什么必须串联限流电阻

LED 导通后正向电压被钳在约 `Vf`（红光常见 ≈1.8～2.2 V），导通电阻很小。没有电阻时，3.3 V 与 LED 近似短路，瞬间电流可到数十 mA，超过 `IIO` 与 PC13 的 3 mA，内部 MOS 过热、击穿，引脚报废。

> 灌电流额定更高（在 51 口诀或 F103 的 20 mA 放宽档）**不等于可以去掉电阻**。两种接法都要限流。

板载 PC13 LED 的限流电阻在核心板上，不要再并联一颗无电阻的 LED 到同一脚。

---

## 4. 限流电阻计算（两种接法公式相同）

\[
R = \frac{V_{\mathrm{supply}} - V_f}{I_{\mathrm{LED}}}
\]

| 符号 | 含义 |
|------|------|
| \(V_{\mathrm{supply}}\) | 回路供电，本仓库 **3.3 V** |
| \(V_f\) | LED 正向压降（红光常见 ≈1.9 V；蓝 / 白常见 ≈3.0～3.2 V） |
| \(I_{\mathrm{LED}}\) | 设定工作电流（不超过上一节上限） |

阻值越大，电流越小，灯越暗。选型：**宁大勿小**。

### 红光 LED（3.3 V）常用阻值

按 \(V_f \approx 1.9\,\mathrm{V}\) 估算：

| 接法 / 目标电流 | 计算 | 常用标称 |
|-----------------|------|----------|
| 约 5.2 mA | \((3.3-1.9)/0.0052 \approx 269\,\Omega\) | **270 Ω** |
| 约 6.4 mA | \((3.3-1.9)/0.0064 \approx 219\,\Omega\) | **220 Ω** |

蓝光 / 白光 `Vf` 接近 3.3 V，压差不足，3.3 V 下两种接法都偏暗，不推荐用本仓库 3.3 V 轨直接驱。

---

## 5. 对照与本仓库

| 项目 | 拉电流 | 灌电流 |
|------|--------|--------|
| 点亮电平 | 输出高 | 输出低 |
| 电流走向 | 引脚向外流出 | 外部流入引脚 |
| 内部管子 | PMOS 上拉 | NMOS 下拉 |
| F103 普通 GPIO 额定 | 与灌电流相同（±8 / ±20 mA） | 与拉电流相同 |
| 电阻 | **不能省** | **不能省** |
| 本仓库板载 PC13 | 少数板可能高电平亮 | **多数核心板：低电平点亮** |

`f103-manual-reg` / `f103-cmsis-hal` 按厂商例程：**置低灯亮、置高灯灭**（灌电流）。串口字符串 `LED on` / `LED off` 指 **GPIO 电平**，不是灯的物理亮灭。见 [f103-manual-reg.md](../projects/f103-manual-reg.md)。

面包板另接指示灯时：用 **普通 GPIO**（不要用 PC13–PC15 / PA13 / PA14），推挽 + 限流电阻；拉、灌都可以。不要为了「灌电流更强」去逼近 20 mA。

---

## 6. 核心结论

1. 拉 / 灌由推挽内部导通的 MOS 决定电流进出方向，电路原理都成立。
2. **无一例外必须限流电阻。**
3. F103 普通脚拉/灌对称；口诀「拉 10 / 灌 20」不要套到本芯片。
4. 板载 LED 已是灌电流，且 PC13 只有约 **3 mA** 能力，不要再当大电流驱动脚。

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| GPIO 八种模式 | [gpio-eight-modes.md](gpio-eight-modes.md) |
| 引脚保护与负压 | [gpio-protection-and-negative-voltage.md](gpio-protection-and-negative-voltage.md) |
| Backup 域与 PC13 | [backup-domain-pc13.md](../reference/stm32f103/md/topics/backup-domain-pc13.md) |
| LQFP48 Note 5（PC13–15 电流） | [lqfp48-pinout.md](../reference/stm32f103/md/topics/lqfp48-pinout.md) |
| 板级接线 | [stm32f103-peripherals.md](../hardware/stm32f103-peripherals.md) |
| DS5319 电气 | [datasheet-index.md](../reference/stm32f103/md/datasheet-index.md)（Table 7、§5.3.13、Table 37） |
