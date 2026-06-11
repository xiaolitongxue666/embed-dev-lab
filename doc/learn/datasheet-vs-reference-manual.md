# Datasheet 与 Reference Manual 怎么读？

整理自开发问答：两份 ST 官方 PDF 各管什么、改 `system_stm32f10x.c` 该看哪本。

本地 PDF 与章节目录：[DS5319 索引](../reference/stm32f103/md/datasheet-index.md) · [RM0008 索引](../reference/stm32f103/md/rm0008-index.md)

---

## Q：Datasheet 和 Reference Manual 分别的主要内容是什么？

一句话：**Datasheet 定「能不能、用多少」；Reference Manual 定「寄存器怎么写」。**

| | **Datasheet（DS5319）** | **Reference Manual（RM0008）** |
|---|---|---|
| **定位** | 选型 / 硬件设计 / 快速了解芯片 | 软件开发 / 寄存器级编程 |
| **篇幅** | 约 114 页 | 约 995–1136 页（版本不同页码不同） |
| **主要内容** | 型号对比、引脚、封装、**时钟树框图**、内存容量、**最高主频**、HSE/PLL **电气参数**、功耗、时序 | **内存映射**、各外设 **寄存器地址与位域**、配置步骤、复位值、编程注意事项 |
| **典型问题** | C8 能跑 72 MHz 吗？APB1 上限多少？板载晶振几 MHz？PC13 在哪？ | `RCC_CR` bit16 是什么？`PLLMUL=0111` 写哪几位？Flash 72 MHz 要几个 wait state？ |

### Datasheet 更像「产品说明书」

- 告诉你这颗 **STM32F103C8** 有 64K Flash、20K RAM
- medium-density **最高 72 MHz**
- 时钟树长什么样、HSE 外部晶振电气要求

### Reference Manual 更像「编程字典」

- 告诉你 `0x40021000` 的 `RCC_CR`、`RCC_CFGR` **每一位**什么意思
- 先写什么、后写什么、复位值是多少

---

## Q：要自己编写 / 修改 `system_stm32f10x.c` 需要看哪个？

**以 RM0008 为主，Datasheet 为辅，两者结合用。**

[`modules/f103-blink/src/system_stm32f10x.c`](../../modules/f103-blink/src/system_stm32f10x.c) 做的事很集中：配置 **RCC** 与 **Flash 等待周期**，把系统时钟升到 72 MHz。

### 必须看 RM0008（写代码的直接依据）

| 内容 | RM0008 章节 | 对应源码操作 |
|------|-------------|--------------|
| RCC 寄存器 | **§6 RCC**（F103C8 读第 6 章，**不是**第 7 章 Connectivity line） | `HSEON`、`PLLSRC`、`PLLMUL`、`SW`、`PPRE1` 等 |
| 内存映射 / 基址 | §2 Memory map | `RCC_BASE` `0x40021000`、`FLASH_BASE` `0x40022000` |
| Flash 等待周期 | Flash 控制器章节（`FLASH_ACR.LATENCY`） | 72 MHz 设 `LATENCY=2`、预取 `PRFTBE` |

没有 RM0008，无法可靠地知道每一位该写多少、复位值是什么、配置顺序是什么。

### 用 Datasheet 确认「目标对不对」（设计约束）

| 内容 | DS5319 章节 | 作用 |
|------|-------------|------|
| 时钟树 | §2.3.7 Clocks and startup | 确认 HSE→PLL→SYSCLK 路径合理 |
| 最高频率 | 关键参数表 | 确认 72 MHz 合法 |
| APB1 限制 | 时钟树 / 说明 | 确认 APB1 ≤36 MHz，故需 `/2` |
| HSE 电气特性 | §5.3.6、§5.3.8 | 确认板载 8 MHz 晶振、启动时间量级 |
| 内存映射摘要 | §4 Memory mapping | 核对基址（细节仍以 RM 为准） |

**只看 Datasheet 写不出完整的 `system_stm32f10x.c`**：它有框图和上限，但没有 RCC 每一位的编码表。

---

## 推荐阅读顺序（针对改 `system_stm32f10x.c`）

```text
1. Datasheet §2.3.7 时钟树
   → 定目标：HSE 8 MHz × PLL9 = 72 MHz，APB1 = 36 MHz

2. RM0008 §6 RCC
   → 写 RCC_CR / RCC_CFGR 的每一步

3. RM0008 Flash 章节（FLASH_ACR）
   → 升频前设 wait state = 2

4. （可选）对照本仓库精选 MD
   doc/reference/stm32f103/md/topics/rcc-clock-hse-pll.md
   → 已与 system_stm32f10x.c 逐位核对
```

---

## 速查表

| 场景 | 看哪本 |
|------|--------|
| 改 `system_stm32f10x.c` 寄存器位、配置顺序 | **RM0008（主）** |
| 确认 72 MHz、8 MHz 晶振、APB1 分频是否合理 | **Datasheet（辅）** |
| 实际开发 | **两者结合**：DS 定方案，RM 写代码 |
| 改 GPIO / PWR / 其他外设 | 同样：**DS 定约束，RM 写寄存器** |

---

## 延伸阅读

- [STM32 裸机启动与时钟 — Q2 手写步骤](stm32-bare-metal-bootstrap.md#q2不依赖-cubemxhal怎么只读手册写出这个文件)
- [RCC：HSE → PLL → 72 MHz](../reference/stm32f103/md/topics/rcc-clock-hse-pll.md)
- [STM32F103 官方参考文档](../reference/stm32f103/README.md)
