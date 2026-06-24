# DS5319 目录索引

| 字段 | 值 |
|------|-----|
| 文档 | DS5319 — STM32F103x8 / STM32F103xB medium-density |
| 适用型号 | **STM32F103C8**（64 KB Flash）、CB（128 KB Flash）等 |
| 本地 PDF | [`../pdf/DS5319-stm32f103x8xB-datasheet.pdf`](../pdf/DS5319-stm32f103x8xB-datasheet.pdf) |
| 官方版本 | Rev 20（2025-07），114 页 |
| 页码 | 指 PDF 页脚 `x/114` |

## F103C8 / f103-manual-reg 相关章节

| 章节 | PDF 页码 | 主题 MD |
|------|----------|---------|
| 2 Description → 2.3.7 Clocks and startup | 15 | [rcc-clock-hse-pll.md](topics/rcc-clock-hse-pll.md) |
| 2.3.14 RTC and backup registers | 17 | [backup-domain-pc13.md](topics/backup-domain-pc13.md) |
| 4 Memory mapping | 34 | [memory-map-medium-density.md](topics/memory-map-medium-density.md) |
| 5.3.6 External clock (HSE) | 50 | [rcc-clock-hse-pll.md](topics/rcc-clock-hse-pll.md) |
| 5.3.8 PLL characteristics | 56 | [rcc-clock-hse-pll.md](topics/rcc-clock-hse-pll.md) |
| 5.3.9 Memory characteristics (Flash/SRAM) | 56 | [memory-map-medium-density.md](topics/memory-map-medium-density.md) |
| 3 Pinouts（PC13 等） | 21–33 | 硬件接线参考 |

## 完整目录（Rev 20）

| 章 | 标题 | 起始页 |
|----|------|--------|
| 1 | Introduction | 9 |
| 2 | Description | 9 |
| 3 | Pinouts and pin description | 21 |
| 4 | Memory mapping | 34 |
| 5 | Electrical characteristics | 35 |
| 6 | Package information | 79 |
| 7 | Part numbering | 87 |

## 关键参数（F103C8）

| 项目 | 值 |
|------|-----|
| Flash | 64 Kbytes |
| SRAM | 20 Kbytes |
| 最高 CPU 频率 | 72 MHz（需 8 MHz HSE + PLL×9） |
| 封装（C8） | LQFP48 等，见 Table 1 Device summary |

## 官方链接（无需注册）

- `https://www.st.com/resource/en/datasheet/stm32f103c8.pdf`
- 同文档别名：`https://www.st.com/resource/en/datasheet/CD00161566.pdf`

下载：`./scripts/fetch-stm32f103-docs.sh`
