# CMSIS 子模块（embed-dev-lab）

本仓库将 ST **CMSIS-Core** 与 **CMSIS-Device F1** 作为 **git submodule** 纳入 `vendor-pack/`，版本成对固定，供对照官方模板；**不参与** [`f103-blink`](../modules/f103-blink/) 构建链接。

| Submodule | 路径 | Tag | 说明 |
|-----------|------|-----|------|
| [cmsis-core](https://github.com/STMicroelectronics/cmsis-core) | [`cmsis-core/`](cmsis-core/) | `v5.4.0_cm3` | [cmsis-core.embed-dev-lab.md](cmsis-core.embed-dev-lab.md) |
| [cmsis-device-f1](https://github.com/STMicroelectronics/cmsis-device-f1) | [`cmsis-device-f1/`](cmsis-device-f1/) | `v4.3.3` | [cmsis-device-f1.embed-dev-lab.md](cmsis-device-f1.embed-dev-lab.md) |

```bash
git clone --recursive <repo-url>
./scripts/fetch-cmsis.sh
./scripts/fetch-cmsis.sh --verify-only
```

HAL/例程等仍经 [`fetch-stm32cubef1.sh`](../scripts/fetch-stm32cubef1.sh) 获取完整 CubeF1 包（可选，与 submodule 分工不同）。

归纳文档：[ST CMSIS 组件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)

---

# cmsis-core（子模块详情）

本目录 [`cmsis-core/`](cmsis-core/) 为 **git submodule**，指向 ST 维护的 [STMicroelectronics/cmsis-core](https://github.com/STMicroelectronics/cmsis-core)。

## 固定版本

| 项 | 值 |
|----|-----|
| Tag | **`v5.4.0_cm3`** |
| 目标内核 | Cortex-M3（STM32F103） |
| 对齐 | [STM32CubeF1](STM32CubeF1/README.md) **1.8.6** 所用 CMSIS-Core 代际 |

> 勿切换到 `master` 或未配对的 Core tag；CMSIS-Core 须与 CMSIS-Device 版本成对使用，见 [ST CMSIS 组件仓库归纳](../doc/learn/stm32-cmsis-component-repos.md)。

## 关键路径（submodule 内）

| 文件 | 路径（相对 `vendor-pack/cmsis-core/`） |
|------|----------------------------------------|
| M3 内核头文件 | `Include/core_cm3.h`（ST 兼容路径） |
| CMSIS 5+ 标准路径 | `Core/Include/core_cm3.h` |
| 编译器适配 | `Include/cmsis_gcc.h`、`Include/cmsis_compiler.h` |

ST 在仓库根保留 `Include/`，内容为 `Core/Include/` 的副本，用于兼容 CMSIS 5.0 起目录树变更。

## 获取与更新

```bash
# 首次 clone 本仓库（推荐）
git clone --recursive <repo-url>

# 已 clone 但未初始化 submodule
./scripts/fetch-cmsis.sh

# 仅校验
./scripts/fetch-cmsis.sh --verify-only
```

更新到上游新 tag（须同步评估 CubeF1 / Device 兼容性）：

```bash
cd vendor-pack/cmsis-core
git fetch --tags origin
git checkout v5.4.0_cm3   # 或经评估后的新 tag
cd ../..
git add vendor-pack/cmsis-core
```

## 与本仓库 demo 的关系

[`modules/f103-blink`](../modules/f103-blink/) **不链接**本 submodule；此处仅供对照官方 `core_cm3.h`、NVIC API 与编译器头文件。Device 层见 [`cmsis-device-f1`](cmsis-device-f1.embed-dev-lab.md) submodule。
