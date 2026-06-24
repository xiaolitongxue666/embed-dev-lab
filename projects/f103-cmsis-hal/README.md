# f103-cmsis-hal（占位）

## 规划定位

基于官方 **CMSIS-Core** + **CMSIS-Device F1** + **STM32F1 HAL/LL** 的 F103 固件小工程，与 [`f103-manual-reg`](../f103-manual-reg/) 并列、彼此独立。

| 组件 | 本仓库路径 / 参考 |
|------|-------------------|
| CMSIS-Core | [`vendor-pack/cmsis-core`](../../vendor-pack/cmsis-core/)（分支 `cm3` / `v5.6.0_cm3`） |
| CMSIS-Device F1 | [`vendor-pack/cmsis-device-f1`](../../vendor-pack/cmsis-device-f1/)（tag `v4.3.5`） |
| HAL/LL 驱动 | [stm32f1xx-hal-driver](https://github.com/STMicroelectronics/stm32f1xx-hal-driver) · [`vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md`](../../vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md) |
| 全包（可选） | [`vendor-pack/STM32CubeF1`](../../vendor-pack/STM32CubeF1/README.md) |

目标芯片：**STM32F103C8T6**（与 `f103-manual-reg` 相同）。

## 当前状态

- **未实现**：本目录暂无 `CMakeLists.txt`，未加入根 `CMakeLists.txt` 的 `add_subdirectory`。
- 人类说明索引：[`doc/projects/f103-cmsis-hal.md`](../../doc/projects/f103-cmsis-hal.md)。

## 后续预期

- `startup` / `system_stm32f1xx.c` 来自 CMSIS-Device 模板（或 CubeMX 引用路径）。
- 时钟：`HAL_RCC_OscConfig` / `HAL_RCC_ClockConfig`（或 LL 等价接口）。
- 外设：如 PC13 LED 使用 `HAL_GPIO_*` 或 `LL_GPIO_*`。
- 构建：`./scripts/build.sh f103-cmsis-hal build`（实现后）。

## 版本配对

CMSIS-Core 与 CMSIS-Device **须成对升级**，见 [`cmsis-device-f1.embed-dev-lab.md`](../../vendor-pack/cmsis-device-f1.embed-dev-lab.md) 与 [ST F1 软件仓库归纳](../../doc/learn/stm32-cmsis-component-repos.md)。
