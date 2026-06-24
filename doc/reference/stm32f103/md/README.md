# STM32F103 精选 MD 说明

本目录 **不是** RM0008 / DS5319 的全文转换，而是：

1. **目录索引**（`datasheet-index.md`、`rm0008-index.md`）— 仅列章节与 PDF 页码  
2. **主题摘录**（`topics/*.md`）— 与 [`projects/f103-manual-reg`](../../../projects/f103-manual-reg) 直接相关的寄存器与流程

## 页码引用约定

- 页码均指 **PDF 内页脚**（如 `DS5319 Rev 20` 的 `x/114`，RM0008 的 `x/1136`）  
- 每条 topic 文件头部注明：文档号、Rev、页码范围、整理日期

## 正确性核对

主题 MD 在整理时与源码交叉核对，例如：

- `RCC_BASE = 0x40021000`，`PWR_BASE = 0x40007000`，`GPIOC_BASE = 0x40011000`
- `RCC_APB1ENR.PWREN` bit 28，`PWR_CR.DBP` bit 8
- `RCC_APB2ENR.IOPCEN` bit 4

若 ST 发布新版本导致页码偏移，请重新运行 `./scripts/fetch-stm32f103-docs.sh --force` 并对照 PDF 更新 topic 页码。

## 免责声明

寄存器位域以 **PDF 原文** 为准；自动 PDF 文本提取可能破坏表格布局，本仓库 topic 文件中的表格均为人工整理。开发时请以本地 `pdf/` 原文为最终依据。
