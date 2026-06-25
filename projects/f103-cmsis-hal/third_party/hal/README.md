# hal — STM32F1 HAL Driver（最小子集）

> **注意**：本目录 **不是** 完整 HAL 驱动仓库。完整 HAL 在 `vendor-pack/stm32f1xx-hal-driver`（**git submodule**，`v1.1.8`）。fetch 策略：**Inc 全拷**（`stm32f1xx_hal*.h` / `ll*.h` / `Legacy/`），**Src 仅拷本 demo 需要的 8 个 .c** 并由 CMake 链入固件；其余模块仅有头文件、无 `.c`，不占 Flash。

## 拷贝策略

`fetch-f103-cmsis-hal-deps.sh` 从 `vendor-pack/stm32f1xx-hal-driver` submodule：

- **`Inc/`**：拷贝全部 `stm32f1xx_hal*.h` 与 `stm32f1xx_ll*.h`（及 `Legacy/`），避免缺依赖头。
- **`Src/`**：仅拷贝本 demo **链接** 的 8 个 `.c`（见根 [`README.md`](../README.md)）。

未拷贝的 HAL 模块（UART、SPI、TIM 等）头文件在 `Inc/` 中存在，但无对应 `.c`，**不会增加 Flash 占用**。

## 本 demo 调用链（应用 → HAL → 寄存器）

```text
main.c
  HAL_Init()                    → hal.c / hal_cortex.c
  HAL_RCC_OscConfig()           → hal_rcc.c, hal_rcc_ex.c
  HAL_RCC_ClockConfig()         → hal_rcc.c, hal_flash.c (latency)
  HAL_PWR_EnableBkUpAccess()    → hal_pwr.c
  HAL_GPIO_Init / WritePin      → hal_gpio.c
  SysTick_Handler → HAL_IncTick → hal.c

stm32f1xx_hal_conf.h (src/)     ← 模块使能、HSE_VALUE、assert_param
stm32f1xx_hal_msp.c (src/)      ← MSP 回调（本 demo 空实现）
```

## Inc/ 与 Src/ 区别

| 目录 | 参与链接 | 说明 |
|------|----------|------|
| `Inc/*.h` | 否 | 声明与 inline 宏；编译期展开 |
| `Src/*.c`（上述 8 个） | **是** | 进入 `.text` / `.rodata` |

## 注释约定

- ST 原版 `@brief` / `@verbatim` **保持英文**；fetch 覆盖后无需手工翻译。
- 每个参与链接的 `Src/*.c` 及 `Inc/stm32f1xx_hal.h` 顶部的 **embed-dev-lab 中文块** 说明在本工程中的用途；由 `scripts/lib/apply-f103-cmsis-hal-comments.sh` 在 fetch 后注入。

## 来源

```bash
vendor-pack/stm32f1xx-hal-driver/   # git submodule @ v1.1.8
```

须先运行 `./scripts/fetch-cmsis.sh` 初始化 submodule。
