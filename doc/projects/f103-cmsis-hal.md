# f103-cmsis-hal 工程

**STM32F103C8T6** 核心板 **PC13 LED 闪烁** demo，使用 **CMSIS + HAL** 实现，功能对齐 [`f103-manual-reg`](f103-manual-reg.md)。

## 模块定位

| 项 | 说明 |
|----|------|
| 目标芯片 | STM32F103C8T6（Cortex-M3，Medium-density F103xB） |
| 实现方式 | 工程内 vendored 最小 **CMSIS-Core/Device** + **STM32F1 HAL**；`HAL_GPIO_*` / `HAL_RCC_*` |
| 构建框架 | 与 `f103-manual-reg` **相同**：`CMakePresets.json` + `embed_mcu_add_executable()` + `./scripts/build.sh` |
| 链接脚本 | **CMSIS 官方** `linker/STM32F103XB_FLASH.ld`（自 `cmsis-device-f1` 拷贝，C8 64K 裁剪） |
| 对照工程 | [`f103-manual-reg`](f103-manual-reg.md)（全手写寄存器，不链接 CMSIS/HAL） |

## 目录结构

```text
projects/f103-cmsis-hal/
├── CMakeLists.txt
├── CMakePresets.json
├── linker/STM32F103XB_FLASH.ld   # CMSIS 模板 + C8 64K
├── startup/startup_stm32f103xb.s # CMSIS GCC 启动
├── third_party/
│   ├── cmsis/Include/            # 最小 Core+Device 头
│   └── hal/Inc/ + hal/Src/       # 最小 HAL（v1.1.8）
└── src/
    ├── main.c                    # HAL_Init、时钟、PC13 闪烁
    ├── system_stm32f1xx.c        # CMSIS SystemInit 模板
    ├── stm32f1xx_hal_conf.h
    ├── stm32f1xx_hal_msp.c
    └── stm32f1xx_it.c
```

## 依赖获取

首次构建前须拷贝最小 CMSIS/HAL 子集（参考源为 `vendor-pack` submodule + `stm32f1xx-hal-driver@v1.1.8`）：

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
./scripts/fetch-f103-cmsis-hal-deps.sh --verify-only
```

## 构建与烧录

与 `f103-manual-reg` 命令形式一致：

```bash
./scripts/build.sh f103-cmsis-hal          # configure + build
./scripts/build.sh f103-cmsis-hal build
./scripts/build.sh f103-cmsis-hal flash
./scripts/build-flash.sh f103-cmsis-hal
```

产物：

- `projects/f103-cmsis-hal/build/f103-cmsis-hal.elf`
- `projects/f103-cmsis-hal/build/f103-cmsis-hal.hex`

probe-rs chip：**`STM32F103C8Tx`**

## 与 f103-manual-reg 对照

| 行为 | f103-manual-reg | f103-cmsis-hal |
|------|-----------------|----------------|
| 系统时钟 | 手写 RCC → HSE×9 72 MHz | `HAL_RCC_OscConfig` / `HAL_RCC_ClockConfig` |
| PC13 Backup 域 | `PWREN` + `DBP` + `GPIOC_CRH` | `HAL_PWR_EnableBkUpAccess` + `HAL_GPIO_Init` |
| 闪烁 | `PCout` + 忙等 | `HAL_GPIO_WritePin` + 同等忙等 |
| 链接脚本 | 手写 `STM32F103C8_FLASH.ld` | CMSIS `STM32F103XB_FLASH.ld` |
| startup | 精简手写 | CMSIS 官方（跳过 `__libc_init_array`） |

## PC13 与 Backup 域

与 manual-reg 相同：须 `PWR` 时钟 + `DBP` 后再配置 GPIOC。见 [`main.c`](../../projects/f103-cmsis-hal/src/main.c) 中 `MX_GPIO_Init()`。

## 版本配对

| 组件 | Tag |
|------|-----|
| CMSIS-Core | `cm3` / `v5.6.0_cm3` |
| CMSIS-Device F1 | `v4.3.5` |
| HAL Driver F1 | `v1.1.8` |

详见 [ST F1 软件仓库归纳](../learn/stm32-cmsis-component-repos.md) §3.1。

## 注释与语言

| 范围 | 语言 | 说明 |
|------|------|------|
| `src/`、`startup/`、`linker/` | 中文 | 工程维护源码；**仅改注释，不改代码** |
| `third_party/**` ST 正文 | 英文（vendor） | fetch 自 ST/CMSIS；不逐行翻译 |
| `third_party/**` 顶部的 embed-dev-lab 块 | 中文 | 说明该文件在本 demo 中的角色；fetch 后 apply 恢复 |
| `third_party/README.md` 等 | 中文 | 目录级说明，fetch 不覆盖 |
| 终端 / CLI / 日志 | 英文 | build.sh、probe-rs 等输出 |

`fetch-f103-cmsis-hal-deps.sh` 拷贝模板后会调用 `scripts/lib/apply-f103-cmsis-hal-comments.sh` 恢复 startup/linker/system 中文注释及 third_party 说明头。详见 [`third_party/README.md`](../../projects/f103-cmsis-hal/third_party/README.md)。

## 排错速查

| 现象 | 优先检查 |
|------|----------|
| configure/build 缺头文件 | 运行 `fetch-f103-cmsis-hal-deps.sh` |
| 链接 `assert_param` / `_init` | 确认 `stm32f1xx_hal_conf.h` 含 `assert_param`；startup 已跳过 `__libc_init_array` |
| 烧录成功但 LED 不闪 | PWR+DBP；先 `build` 再 `flash` |
| fetch 后 startup/linker 注释变英文 | 重新 `./scripts/fetch-f103-cmsis-hal-deps.sh`（含中文注释补丁） |

## 调试

- IDE：**F103 CMSIS-HAL Probe-rs Debug**（F5；`.vscode/launch.json`）
- 调试前须已 `./scripts/fetch-f103-cmsis-hal-deps.sh`；preLaunchTask 自动 `build.sh f103-cmsis-hal`
- OpenOCD 备选：**F103 CMSIS-HAL OpenOCD Debug**
- 详见 [ide-debug.md](../ide-debug.md)

## 延伸阅读

- [CMSIS 标准与手写裸机边界](../learn/cmsis-overview.md) §6
- [f103-manual-reg 模块说明](f103-manual-reg.md)
- [f103-manual-reg 编译流程](../learn/f103-module-build-flow.md)
