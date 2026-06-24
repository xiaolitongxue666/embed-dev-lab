# RM0008 目录索引

| 字段 | 值 |
|------|-----|
| 文档 | RM0008 Reference manual |
| 本地 PDF | [`../pdf/RM0008-stm32f10xxx-reference-manual.pdf`](../pdf/RM0008-stm32f10xxx-reference-manual.pdf) |
| 页码 | 指 PDF 页脚 `x/995` 或 `x/1136`（视版本而定） |
| 说明 | 当前本地副本可能为 **Rev 9（995 页，Keil 镜像）** 或 ST 官网 **Rev 21（1136 页）**；寄存器地址一致，页码以本地 PDF 为准 |

## F103 / f103-manual-reg 相关章节（优先阅读）

| 章节 | PDF 页码 (Rev 9) | 主题 MD |
|------|------------------|---------|
| 2 Memory and bus architecture → 2.3 Memory map；Boot configuration / 启动重映射 | 41–44 | [memory-map-medium-density.md](topics/memory-map-medium-density.md)（含 BOOT、System memory、外设/PPB 区）；MMIO 叙述 [stm32f103-mmio-basics.md](../../../learn/stm32f103-mmio-basics.md) |
| 4 Power control (PWR) → 4.1.2 Battery backup domain | 54–55 | [backup-domain-pc13.md](topics/backup-domain-pc13.md) |
| 4.4 PWR registers (PWR_CR DBP) | 62–65 | [backup-domain-pc13.md](topics/backup-domain-pc13.md) |
| 5 Backup registers (BKP) | 66–70 | [backup-domain-pc13.md](topics/backup-domain-pc13.md) |
| 6 RCC (medium-density) → 6.2 Clocks / 6.3 Registers | 74–102 | [rcc-clock-hse-pll.md](topics/rcc-clock-hse-pll.md) |
| 8 GPIOs and AFIOs | 138–167 | [backup-domain-pc13.md](topics/backup-domain-pc13.md) |

> **Connectivity line（F105/F107）** 使用第 7 章 RCC，**F103C8 请读第 6 章**。

## 完整目录（Rev 9 本地 PDF 摘录）

| 章 | 标题 | 起始页 |
|----|------|--------|
| 1 | Documentation conventions | 37 |
| 2 | Memory and bus architecture | 38 |
| 3 | CRC calculation unit | 50 |
| 4 | Power control (PWR) | 53 |
| 5 | Backup registers (BKP) | 66 |
| 6 | Low-/medium-/high-density RCC | 74 |
| 7 | Connectivity line RCC (F105/F107) | 104 |
| 8 | GPIOs and AFIOs | 138 |
| 9 | Nested vectored interrupt controller | 181 |
| 10 | External interrupt/event controller (EXTI) | 196 |
| 11–25 | 定时器、串口、SPI、I2C、ADC、DMA 等 | 见 PDF 目录 |
| 26+ | USB、Flash、DBG 等 | 见 PDF 目录 |

完整 995 页目录见 PDF 第 1–20 页 Contents。

## 官方链接

- ST 直链（无需注册）：`https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf`
- Keil 镜像（备用）：`https://www.keil.com/dd/docs/datashts/st/stm32f10xxx.pdf`
