# cmsis-device-f1（embed-dev-lab 子模块说明）

本目录 [`cmsis-device-f1/`](cmsis-device-f1/) 为 **git submodule**，指向 ST 维护的 [STMicroelectronics/cmsis-device-f1](https://github.com/STMicroelectronics/cmsis-device-f1)。

目标芯片：**STM32F103C8T6**（Medium-density **F103xB**；与 [`f103-blink`](../modules/f103-blink/) 的 `startup_stm32f103xb.s` 同系列）。

## 固定版本

| 项 | 值 |
|----|-----|
| Tag | **`v4.3.5`**（F1 Device 最新发布 tag；勿跟踪 `master`） |
| 配对 Core | [`cmsis-core`](cmsis-core/) 分支 **`cm3`** / **`v5.6.0_cm3`** |

> CMSIS-Core 与 CMSIS-Device **须成对升级**；ST 官方表见 [cmsis-device-f1 README](https://github.com/STMicroelectronics/cmsis-device-f1#compatibility-information) 与 [ST F1 软件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)。

## 关键路径（submodule 内）

| 文件 | 路径（相对 `vendor-pack/cmsis-device-f1/`） |
|------|---------------------------------------------|
| 设备头文件 | `Include/stm32f103xb.h` |
| GCC startup | `Source/Templates/gcc/startup_stm32f103xb.s` |
| SystemInit 模板 | `Source/Templates/system_stm32f1xx.c` |

CubeF1 全包内等价路径为 `Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/...`（目录层级不同，内容同源代际）。

## 获取与更新

```bash
git clone --recursive <repo-url>
./scripts/fetch-cmsis.sh
./scripts/fetch-cmsis.sh --verify-only
```

更新 tag 时须同步评估 [`cmsis-core`](cmsis-core.embed-dev-lab.md) 版本。

## 与本仓库 demo 的关系

[`modules/f103-blink`](../modules/f103-blink/) **不链接**本 submodule；手写 `startup` / `system_stm32f10x.c` 可与此处官方模板对照。Core 头文件见 [`cmsis-core`](cmsis-core.embed-dev-lab.md)。
