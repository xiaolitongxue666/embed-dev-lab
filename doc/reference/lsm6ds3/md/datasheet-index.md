# DocID026899 目录索引

| 字段 | 值 |
|------|-----|
| 文档 | DocID026899 — LSM6DS3 iNEMO：always-on 3D 加速度计与 3D 陀螺仪 |
| 适用型号 | **LSM6DS3**（Tray）、**LSM6DS3TR**（Tape & Reel） |
| 本地 PDF | [`../pdf/DocID026899-lsm6ds3tr-datasheet.pdf`](../pdf/DocID026899-lsm6ds3tr-datasheet.pdf) |
| 版本 | Rev 8（2016-02），100 页 |
| 页码 | 指 PDF 页脚 `x/100` |
| 整理日期 | 2026-08-31 |

## f103-manual-reg SPI demo 相关章节

| 章节 | PDF 页码 | 主题 MD |
|------|----------|---------|
| 4.1 Mechanical characteristics（灵敏度） | 20–22 | [registers-whoami-imu.md](topics/registers-whoami-imu.md) |
| 4.2 Electrical characteristics（Vdd） | 23 | [electrical-spi-timing.md](topics/electrical-spi-timing.md) |
| 4.4.1 SPI timing | 25 | [electrical-spi-timing.md](topics/electrical-spi-timing.md) |
| 6 Digital interfaces / 6.2 SPI | 34–39 | [spi-protocol.md](topics/spi-protocol.md) |
| 8 Register mapping | 42–45 | [registers-whoami-imu.md](topics/registers-whoami-imu.md) |
| 9.11–9.13 / 9.26 / 9.28–9.39 WHO_AM_I、CTRL、STATUS、OUT | 53–66 | [registers-whoami-imu.md](topics/registers-whoami-imu.md) |

## 完整目录（Rev 8）

| 章 | 标题（中文意译） | 起始页 |
|----|------------------|--------|
| 1 | 概述 Overview | 15 |
| 2 | 嵌入式低功耗特性 Embedded low-power features | 16 |
| 3 | 引脚说明 Pin description | 17 |
| 4 | 模块规格 Module specifications | 20 |
| 5 | 功能 Functionality | 29 |
| 6 | 数字接口 Digital interfaces | 34 |
| 7 | 应用提示 Application hints | 40 |
| 8 | 寄存器映射 Register mapping | 42 |
| 9 | 寄存器说明 Register description | 46 |
| 10 | 嵌入功能寄存器映射 Embedded functions register mapping | 83 |
| 11 | 嵌入功能寄存器说明 Embedded functions registers description | 85 |
| 12 | 焊接信息 Soldering information | 95 |
| 13 | 封装信息 Package information | 96 |
| 14 | 修订历史 Revision history | 99 |

## 关键参数摘要

| 项目 | 值 |
|------|-----|
| 加速度满量程 | ±2 / ±4 / ±8 / ±16 g |
| 角速率满量程 | ±125 / ±245 / ±500 / ±1000 / ±2000 dps |
| 模拟供电 Vdd | 1.71 V – 3.6 V |
| 接口 | SPI（3/4 线）与 I2C |
| WHO_AM_I | `69h` |
| 封装 | LGA-14L（2.5 × 3 × 0.83 mm） |
