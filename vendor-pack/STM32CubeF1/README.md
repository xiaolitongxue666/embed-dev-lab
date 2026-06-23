# STM32CubeF1 固件包（本地）

ST 官方 **STM32Cube MCU Package for F1**，含 CMSIS-Core/Device、HAL/LL 驱动、Middleware 与例程。

## 目标版本

**1.8.6**（与 GitHub [STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1) `version-license.txt` 一致）

## 获取

```bash
# 从仓库根目录
./scripts/fetch-stm32cubef1.sh
```

手动方式：从 [ST 官网](https://www.st.com/en/embedded-software/stm32cubef1.html) 下载 ZIP → 放入 `archives/` → 再运行上述脚本或：

```bash
./scripts/fetch-stm32cubef1.sh --from-zip vendor-pack/STM32CubeF1/archives/STM32CubeF1-1.8.6.zip
```

**注意**：GitHub 页面「Download ZIP」不含 submodule，CMSIS 等会缺失；请用官网 ZIP 或 `git clone --recursive`。

## CMSIS 关键路径（解压 / clone 后）

相对固件包根目录：

| 文件 | 路径 |
|------|------|
| GCC startup | `Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s` |
| SystemInit 模板 | `Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c`（旧包可能为 `system_stm32f10x.c`） |
| 设备头文件 | `Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h` |

## 与本仓库 demo 的关系

[`modules/f103-blink`](../../modules/f103-blink) 使用**手写**精简版 `startup_stm32f103xb.s` 与 `system_stm32f10x.c`，不链接 HAL，不依赖本目录构建。此处固件包供 **CMSIS-Device** 层对照（启动文件、设备头文件、SystemInit 模板）。

**CMSIS-Core** 与 **CMSIS-Device F1** 为独立 submodule：[`cmsis-core`](../../vendor-pack/cmsis-core.embed-dev-lab.md)（`v5.4.0_cm3`）、[`cmsis-device-f1`](../../vendor-pack/cmsis-device-f1.embed-dev-lab.md)（`v4.3.3`），与 CubeF1 包内 `Drivers/CMSIS/` 同源代际，便于分层查阅。

CMSIS 分层、ST 组件仓库与手写兼容边界见 [`doc/learn/cmsis-overview.md`](../../doc/learn/cmsis-overview.md)、[`doc/learn/stm32-cmsis-component-repos.md`](../../doc/learn/stm32-cmsis-component-repos.md)。
