# stm32f1xx-hal-driver（embed-dev-lab 子模块说明）

本目录 [`stm32f1xx-hal-driver/`](stm32f1xx-hal-driver/) 为 **git submodule**，指向 ST 维护的 [STMicroelectronics/stm32f1xx-hal-driver](https://github.com/STMicroelectronics/stm32f1xx-hal-driver)。

目标芯片：**STM32F103C8T6**（F1 系列 HAL + LL 外设驱动）。

## 固定版本

| 项 | 值 |
|----|-----|
| Tag | **`v1.1.8`**（F1 HAL 发布 tag；勿跟踪 `master`） |
| 配对 Core | [`cmsis-core`](cmsis-core/) 分支 **`cm3`** / **`v5.6.0_cm3`** |
| 配对 Device | [`cmsis-device-f1`](cmsis-device-f1/) tag **`v4.3.5`** |

> CMSIS-Core、CMSIS-Device 与 HAL **须成对升级**；ST 官方表见 [stm32f1xx-hal-driver README](https://github.com/STMicroelectronics/stm32f1xx-hal-driver#compatibility-information) 与 [ST F1 软件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)。

## 关键路径（submodule 内）

| 文件 | 路径（相对 `vendor-pack/stm32f1xx-hal-driver/`） |
|------|-----------------------------------------------------|
| HAL 主头 | `Inc/stm32f1xx_hal.h` |
| GPIO 驱动 | `Src/stm32f1xx_hal_gpio.c` |
| LL 头文件 | `Inc/stm32f1xx_ll_*.h` |

CubeF1 全包内等价路径为 `Drivers/STM32F1xx_HAL_Driver/`（目录层级不同，内容同源代际）。

## 获取与更新

```bash
git clone --recursive <repo-url>
./scripts/fetch-cmsis.sh
./scripts/fetch-cmsis.sh --verify-only
```

现有 clone 迁移（新增 HAL submodule 后）：

```bash
git pull
git submodule update --init vendor-pack/stm32f1xx-hal-driver
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh   # 刷新 third_party
# 可选：rm -rf .tools/stm32f1xx-hal-driver-ref
```

更新 tag 时须同步评估 [`cmsis-core`](cmsis-core.embed-dev-lab.md) 与 [`cmsis-device-f1`](cmsis-device-f1.embed-dev-lab.md) 版本。

## 与本仓库 demo 的关系

[`projects/f103-manual-reg`](../projects/f103-manual-reg/) **不链接**本 submodule；纯寄存器实现。

[`projects/f103-cmsis-hal`](../projects/f103-cmsis-hal/) 通过 `./scripts/fetch-f103-cmsis-hal-deps.sh` 从本 submodule **按需裁剪**拷贝至 [`third_party/`](../projects/f103-cmsis-hal/third_party/)（Inc 全拷、Src 仅 8 个 `.c` 链入 `.elf`）；**完整 HAL 仓库在本 submodule**，`third_party/hal/` 不是完整上游。

归纳文档：[ST F1 软件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)
