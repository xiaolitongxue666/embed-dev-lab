# stm32f1xx-hal-driver（参考仓库说明）

本仓库**未**将 [STMicroelectronics/stm32f1xx-hal-driver](https://github.com/STMicroelectronics/stm32f1xx-hal-driver) 列为 submodule；此处仅作 **HAL/LL 层** 上游参考。

## 是什么

ST **STM32CubeF1 MCU Component** 中的 `hal_driver` 部分，提供 F1 系列 **HAL + LL** 外设驱动（`stm32f1xx_hal_gpio.c`、`stm32f1xx_ll_gpio.h` 等）。

HAL 全包内路径（fetch 后）：`Drivers/STM32F1xx_HAL_Driver/`

## 版本配对（须与 CMSIS 一致）

摘自 [stm32f1xx-hal-driver README](https://github.com/STMicroelectronics/stm32f1xx-hal-driver#compatibility-information)（节选）：

| HAL Driver F1 | CMSIS Device F1 | CMSIS Core | STM32CubeF1 |
|---------------|-----------------|------------|-------------|
| v1.1.8 | v4.3.3 | v5.4.0_cm3 | v1.8.4 |

与 embed-dev-lab 当前 CMSIS submodule（`v5.6.0_cm3` + `v4.3.5`）对照时，HAL 宜查阅 [stm32f1xx-hal-driver Release Notes](https://github.com/STMicroelectronics/stm32f1xx-hal-driver) 确认兼容性；ST 文档记载的旧配对为 **v1.1.8**（见上表）。

## 与本仓库 demo 的关系

[`modules/f103-blink`](../modules/f103-blink/)（STM32F103C8T6）**不链接** HAL，采用纯寄存器实现。若后续做 HAL 实验，建议：

1. 单独 clone 本仓库，或
2. 使用 `fetch-stm32cubef1.sh` 获取全包，或
3. 新建独立模块（如 `modules/f103-hal-blink`），与现有 demo 解耦

## 获取

```bash
git clone https://github.com/STMicroelectronics/stm32f1xx-hal-driver.git
git checkout v1.1.8   # 与当前 CMSIS submodule 成对时
```

全包（含 HAL + Middleware + 例程）：[`fetch-stm32cubef1.sh`](../scripts/fetch-stm32cubef1.sh) → [STM32CubeF1/README.md](STM32CubeF1/README.md)

归纳文档：[ST CMSIS 与 F1 软件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)
