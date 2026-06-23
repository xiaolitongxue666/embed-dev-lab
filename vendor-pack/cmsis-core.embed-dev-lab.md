# CMSIS 子模块（embed-dev-lab）

本仓库将 ST **CMSIS-Core** 与 **CMSIS-Device F1** 作为 **git submodule** 纳入 `vendor-pack/`，版本成对固定，供对照官方模板；**不参与** [`f103-blink`](../modules/f103-blink/) 构建链接。

目标芯片：**STM32F103C8T6**（Cortex-M3，F103xB，`startup_stm32f103xb.s`）。

| Submodule | 路径 | 分支 / Tag | 说明 |
|-----------|------|------------|------|
| [cmsis-core](https://github.com/STMicroelectronics/cmsis-core) | [`cmsis-core/`](cmsis-core/) | 分支 **`cm3`** · `v5.6.0_cm3` | 本节下文 |
| [cmsis-device-f1](https://github.com/STMicroelectronics/cmsis-device-f1) | [`cmsis-device-f1/`](cmsis-device-f1/) | tag **`v4.3.5`** | [cmsis-device-f1.embed-dev-lab.md](cmsis-device-f1.embed-dev-lab.md) |

```bash
git clone --recursive <repo-url>
./scripts/fetch-cmsis.sh
./scripts/fetch-cmsis.sh --verify-only
```

HAL / 全包参考：[stm32f1xx-hal-driver](stm32f1xx-hal-driver.embed-dev-lab.md)、[STM32CubeF1](STM32CubeF1/README.md)（可选 fetch）。

归纳文档：[ST F1 软件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)

---

# cmsis-core（子模块详情）

本目录 [`cmsis-core/`](cmsis-core/) 为 **git submodule**，指向 ST 维护的 [STMicroelectronics/cmsis-core](https://github.com/STMicroelectronics/cmsis-core)。

## 固定版本

| 项 | 值 |
|----|-----|
| 分支 | **`cm3`**（ST 为 Cortex-M3 维护的内核专用分支，见 `.gitmodules`） |
| Tag | **`v5.6.0_cm3`**（当前 `cm3` 分支发布点） |
| 配对 Device | [`cmsis-device-f1`](cmsis-device-f1.embed-dev-lab.md) **`v4.3.5`** |

> 勿切换到 `master` 或未配对的 Core tag；升级须与 Device 同步，见 [ST F1 软件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)。

## 关键路径（submodule 内）

| 文件 | 路径（相对 `vendor-pack/cmsis-core/`） |
|------|----------------------------------------|
| M3 内核头文件 | `Include/core_cm3.h`（ST 兼容路径） |
| CMSIS 5+ 标准路径 | `Core/Include/core_cm3.h` |
| 编译器适配 | `Include/cmsis_gcc.h`、`Include/cmsis_compiler.h` |

ST 在仓库根保留 `Include/`，内容为 `Core/Include/` 的副本，用于兼容 CMSIS 5.0 起目录树变更。

## 获取与更新

```bash
git clone --recursive <repo-url>
./scripts/fetch-cmsis.sh
./scripts/fetch-cmsis.sh --verify-only
```

更新到上游（须同步评估 Device / HAL 兼容性）：

```bash
cd vendor-pack/cmsis-core
git fetch origin cm3
git checkout v5.6.0_cm3   # 或经评估后的 cm3 上 tag
cd ../..
git add vendor-pack/cmsis-core
```

## 与本仓库 demo 的关系

[`modules/f103-blink`](../modules/f103-blink/) **不链接**本 submodule；此处仅供对照官方 `core_cm3.h`、NVIC API 与编译器头文件。Device 层见 [`cmsis-device-f1`](cmsis-device-f1.embed-dev-lab.md)。
