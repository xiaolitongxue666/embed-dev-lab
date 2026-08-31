# LSM6DS3 精选 MD 说明

本目录 **不是** DocID026899 的全文翻译，而是：

1. **目录索引**（`datasheet-index.md`）— 中文章节标题与 PDF 页脚页码  
2. **主题摘录**（`topics/*.md`）— 与 [`projects/f103-manual-reg`](../../../projects/f103-manual-reg) SPI IMU demo 直接相关的电气、协议与寄存器

## 页码引用约定

- 页码均指 **PDF 内页脚**（如 `DocID026899 Rev 8` 的 `x/100`）  
- 每条 topic 文件头部注明：文档号、Rev、页码范围、整理日期

## 正确性核对

主题 MD 与本地 PDF 交叉核对，例如：

- `WHO_AM_I (0Fh)` = `69h`
- SPI 最高 `fc(SPC)` = 10 MHz；时序图按 Mode 3（SPC 空闲为高）
- `CTRL3_C.IF_INC` 复位默认 1（多字节访问地址自增）
- 输出寄存器从 `OUTX_L_G (22h)` 起连续到 `OUTZ_H_XL (2Dh)`

位域与时序以 **PDF 原文** 为准；自动 PDF 文本提取可能破坏表格，本仓库 topic 中的表格均为人工整理。
