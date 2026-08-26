# third_party — f103-cmsis-hal 依赖说明

本目录存放 **vendored 最小 CMSIS + HAL 子集**，供 PC13 闪烁 demo 编译链接。  
**不是** ST 官方完整 CMSIS/HAL 软件包；**不由 git submodule 管理**；由 `./scripts/fetch-f103-cmsis-hal-deps.sh` 从 `vendor-pack` 与 HAL ref **按需裁剪**拷贝。

## 是否为完整 CMSIS/HAL？

**否 — 只 copy 了本 demo 需要的部分。** 完整上游在：

| 参考源 | 路径 | 说明 |
|--------|------|------|
| CMSIS-Core | `vendor-pack/cmsis-core` | 完整 submodule（`cm3` / `v5.6.0_cm3`） |
| CMSIS-Device F1 | `vendor-pack/cmsis-device-f1` | 完整 submodule（`v4.3.5`） |
| HAL Driver F1 | `vendor-pack/stm32f1xx-hal-driver` | 完整 HAL submodule（`v1.1.8`） |

### fetch 裁剪策略（`scripts/fetch-f103-cmsis-hal-deps.sh`）

| 目标 | 拷贝内容 | 是否链入 `.elf` |
|------|----------|-----------------|
| `third_party/cmsis/Include/` | **7 个头文件**（见 [`cmsis/README.md`](cmsis/README.md)） | 否（编译期 `#include`） |
| `startup/` · `linker/` · `src/system_stm32f1xx.c` | CMSIS GCC 模板（**不在 third_party 内**） | startup/system 是 |
| `third_party/hal/Inc/` | **全部** `stm32f1xx_hal*.h`、`stm32f1xx_ll*.h`、`Legacy/*.h` | 否（仅声明；避免缺头文件） |
| `third_party/hal/Src/` | **仅 9 个 .c**（GPIO/RCC/PWR/Flash/Cortex/UART 等） | **是** |

```text
vendor-pack/cmsis-core + cmsis-device-f1 + stm32f1xx-hal-driver
        ↓ fetch 按需裁剪（非整包复制）
third_party/  +  startup/ + linker/ + src/system_stm32f1xx.c
        ↓ CMake：9 个 HAL .c + 工程 src/startup
f103-cmsis-hal.elf
```

**结论**：`third_party` 是工程内 **最小 vendored 依赖** — CMSIS 只留 7 个必需头；HAL **头全拷、实现只拷 9 个 .c**（含 `hal_uart.c`）。新增外设模块时须在 fetch 脚本的 `HAL_SRC_FILES` 与 `CMakeLists.txt` 中追加对应 `.c`。

## 目录结构

```text
third_party/
├── README.md           ← 本文件（工程维护，fetch 不覆盖）
├── cmsis/
│   ├── README.md
│   └── Include/        ← CMSIS-Core + CMSIS-Device 头文件
└── hal/
    ├── README.md
    ├── Inc/            ← ST HAL 头（拷贝 ref 中全部 stm32f1xx_hal*.h / ll*.h）
    └── Src/            ← 本 demo 实际参与链接的 9 个 HAL 源文件
```

## 版本与来源

| 组件 | Tag / 来源 | 拷贝目标 |
|------|------------|----------|
| CMSIS-Core | `cm3` / `v5.6.0_cm3` | `vendor-pack/cmsis-core` |
| CMSIS-Device F1 | `v4.3.5` | `vendor-pack/cmsis-device-f1` |
| HAL Driver F1 | `v1.1.8` | `vendor-pack/stm32f1xx-hal-driver` |

## 编译参与范围

CMake [`CMakeLists.txt`](../CMakeLists.txt) 仅链接 **9 个 HAL .c**；`hal/Inc` 中其余头文件供 `#include` 依赖解析，**不进入 .text**。

| 路径 | 本 demo 用途 |
|------|----------------|
| `hal/Src/stm32f1xx_hal.c` | `HAL_Init`、SysTick、`HAL_IncTick` |
| `hal/Src/stm32f1xx_hal_cortex.c` | NVIC / SysTick 配置辅助 |
| `hal/Src/stm32f1xx_hal_gpio.c` | `HAL_GPIO_Init` / `WritePin`（PC13） |
| `hal/Src/stm32f1xx_hal_rcc.c` | `HAL_RCC_OscConfig` / `ClockConfig`（72 MHz） |
| `hal/Src/stm32f1xx_hal_rcc_ex.c` | F103 PLL 扩展配置 |
| `hal/Src/stm32f1xx_hal_pwr.c` | `HAL_PWR_EnableBkUpAccess`（Backup 域） |
| `hal/Src/stm32f1xx_hal_flash.c` | Flash 等待周期（LATENCY_2） |
| `hal/Src/stm32f1xx_hal_flash_ex.c` | Flash 扩展操作 |
| `hal/Src/stm32f1xx_hal_uart.c` | `HAL_UART_Transmit`（USART1） |

工程维护配置在 **`src/stm32f1xx_hal_conf.h`**（模块裁剪、`assert_param`），不在 `third_party` 内。

`HAL_DMA_MODULE_ENABLED` 在 conf 中开启但**未**链入 `hal_dma.c`：当前仅阻塞 UART；若启用 UART DMA，须同步 fetch/`CMakeLists.txt`/conf。

## 注释与语言约定

| 范围 | 语言 | 说明 |
|------|------|------|
| ST 源码正文注释 | **英文（vendor 原文）** | fetch 会覆盖，不逐行翻译 |
| 文件顶 `embed-dev-lab` 块 | **中文** | 说明该文件在本 demo 中的角色；fetch 后由 apply 脚本恢复 |
| `README.md` | **中文** | 目录级说明，fetch 不覆盖 |

## 维护命令

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
./scripts/fetch-f103-cmsis-hal-deps.sh --verify-only
```

重跑 fetch 会覆盖 `cmsis/Include` 与 `hal/`，以及 `startup/`、`linker/`、`src/system_stm32f1xx.c`。  
`README.md` 与 `src/` 中除 `system_stm32f1xx.c` 外的应用源码不受影响。

## 延伸阅读

- [doc/projects/f103-cmsis-hal.md](../../doc/projects/f103-cmsis-hal.md)
- [doc/learn/cmsis-overview.md](../../doc/learn/cmsis-overview.md)
- [doc/learn/stm32-cmsis-component-repos.md](../../doc/learn/stm32-cmsis-component-repos.md)
