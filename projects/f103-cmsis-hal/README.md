# f103-cmsis-hal

基于 **CMSIS-Core** + **CMSIS-Device F1** + **STM32F1 HAL** 的 F103 PC13 闪烁 demo，与 [`f103-manual-reg`](../f103-manual-reg/) 功能对等、构建框架一致。

| 项 | 说明 |
|----|------|
| 芯片 | STM32F103C8T6（64 KB Flash / 20 KB RAM） |
| 实现 | HAL API（`HAL_GPIO_*`、`HAL_RCC_*` 等）；`third_party` **vendored 并入构建**（8 个 HAL `.c` 链入 `.elf`） |
| 构建 | `./scripts/build.sh f103-cmsis-hal`（CMake Preset + `embed_mcu_add_executable`，同 manual-reg） |
| 链接脚本 | CMSIS [`linker/STM32F103XB_FLASH.ld`](linker/STM32F103XB_FLASH.ld)（C8 64K 裁剪；非 manual-reg 手写版） |
| 长文档 | [`doc/projects/f103-cmsis-hal.md`](../../doc/projects/f103-cmsis-hal.md) |
| third_party 细则 | [`third_party/README.md`](third_party/README.md)（fetch 来源、注释约定） |

## 依赖获取

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
./scripts/fetch-f103-cmsis-hal-deps.sh --verify-only
```

拷贝来源：`vendor-pack/cmsis-core` + `vendor-pack/cmsis-device-f1` + `vendor-pack/stm32f1xx-hal-driver`（`v1.1.8`）；**按需裁剪**的最小子集进 `third_party/`。

## third_party 是完整 CMSIS/HAL 吗？

**不是。** `third_party/` 存放的是 **按需拷贝的最小子集**，不是 ST 官方完整软件包。完整上游仍在仓库级参考源中：

| 参考源（完整） | 路径 |
|----------------|------|
| CMSIS-Core / Device F1 | `vendor-pack/cmsis-core`、`vendor-pack/cmsis-device-f1` |
| HAL Driver F1 v1.1.8 | `vendor-pack/stm32f1xx-hal-driver` |

`fetch-f103-cmsis-hal-deps.sh` 的裁剪策略：

| 组件 | 拷贝范围 | 说明 |
|------|----------|------|
| **CMSIS 头** | **仅 7 个** → `third_party/cmsis/Include/` | PC13 demo 编译所需最小 Core+Device 头 |
| **CMSIS 模板** | **不在 third_party** | `startup/`、`linker/`、`src/system_stm32f1xx.c` 单独拷贝到工程根目录 |
| **HAL Inc** | **全部** `stm32f1xx_hal*.h` + `stm32f1xx_ll*.h` + `Legacy/` | 避免 `#include` 缺依赖；**多数无对应 .c，不占 Flash** |
| **HAL Src** | **仅 8 个 .c** → `third_party/hal/Src/` | CMake 编译并链入 `f103-cmsis-hal.elf` |

```text
vendor-pack/cmsis-core + cmsis-device-f1 + stm32f1xx-hal-driver   ← 完整 CMSIS/HAL（submodule，拷贝源）
        ↓ fetch 按需裁剪
projects/f103-cmsis-hal/third_party/             ← 工程内 vendored 最小子集
        ↓ CMake 仅链入 8 个 HAL .c
build/f103-cmsis-hal.elf                         ← 烧录到板子
```

UART、SPI、TIM 等 HAL 模块在 `hal/Inc/` 中虽有头文件，但 **无对应 `.c` 被链接**，因此不会增大固件体积。详见 [`third_party/README.md`](third_party/README.md)。

## 构建与烧录

```bash
./scripts/build.sh f103-cmsis-hal build
./scripts/build.sh f103-cmsis-hal flash
./scripts/build-flash.sh f103-cmsis-hal
```

产物：`build/f103-cmsis-hal.elf` · probe-rs chip **`STM32F103C8Tx`**

## USART1 串口（无 printf）

本工程**不用 `printf`**，串口走 `USART1_WriteStr` → `HAL_UART_Transmit`（无 `syscalls.c`）。若要用 libc 格式化输出，见 [`doc/learn/newlib-nosys-stdio-retarget.md`](../../doc/learn/newlib-nosys-stdio-retarget.md) 与 [`f103-manual-reg`](../f103-manual-reg/) 的 `syscalls.c`。

| 项 | 说明 |
|----|------|
| 引脚 | PA9 TX，PA10 RX；CH341 RX←PA9，GND 共地 |
| 波特率 | 1500000 8N1 |
| 换行 | 字符串写 `\n`；`USART1_WriteStr` 自动补 `\r` |

---

## 带注释的目录树

> 不含 `build/`（CMake 生成物）。`#` 后为该路径在本 demo 中的作用。

```text
f103-cmsis-hal/
├── CMakeLists.txt              # 源文件列表、STM32F103xB/USE_HAL_DRIVER 宏、include 与链接脚本
├── CMakePresets.json           # Ninja + arm-none-eabi 工具链 preset
├── README.md                   # 本文件（工程结构权威说明）
│
├── linker/
│   └── STM32F103XB_FLASH.ld    # GNU ld：Flash 64K/RAM 20K、段布局；导出 _sidata/_sdata/_sbss/_ebss/_estack
│
├── startup/
│   └── startup_stm32f103xb.s   # 向量表 @ 0x08000000；Reset：SystemInit→.data→.bss→main；跳过 __libc_init_array
│
├── src/                        # 工程维护应用层（fetch 不覆盖，除 system_stm32f1xx.c）
│   ├── main.c                  # HAL_Init、72 MHz 时钟、Backup 域 PC13、USART1、闪烁主循环
│   ├── main.h                  # 包含 stm32f1xx_hal.h；声明 Error_Handler
│   ├── usart.c / usart.h       # USART1 初始化 + USART1_WriteStr（HAL_UART_Transmit，无 printf/syscalls）
│   ├── stm32f1xx_hal_conf.h    # HAL 模块裁剪（GPIO/RCC/PWR/FLASH/CORTEX/UART/DMA）、HSE_VALUE
│   ├── stm32f1xx_hal_msp.c     # HAL MSP 回调（本 demo 空实现）
│   ├── stm32f1xx_it.c          # Cortex-M3 异常处理；SysTick_Handler → HAL_IncTick
│   ├── stm32f1xx_it.h          # 异常处理函数声明
│   └── system_stm32f1xx.c      # CMSIS SystemInit 模板；PLL 时钟在 main.c HAL 中配置
│
└── third_party/                # vendored CMSIS+HAL（fetch 拷贝；见 third_party/README.md）
    ├── README.md               # third_party 总览、版本、编译参与范围
    │
    ├── cmsis/
    │   ├── README.md           # CMSIS 拷贝清单与 include 关系
    │   └── Include/            # 编译期头文件，不单独生成 .o
    │       ├── core_cm3.h           # Cortex-M3 内核寄存器、NVIC、SysTick
    │       ├── cmsis_compiler.h     # CMSIS 编译器抽象入口
    │       ├── cmsis_version.h      # CMSIS 版本标识
    │       ├── cmsis_gcc.h          # GCC/Arm Compiler intrinsic 与 barrier
    │       ├── stm32f1xx.h          # F1 设备系列入口；USE_HAL_DRIVER、芯片密度宏
    │       ├── stm32f103xb.h        # F103xB 外设寄存器映射（HAL 底层读写 RCC/GPIO/PWR 等）
    │       └── system_stm32f1xx.h   # SystemInit / SystemCoreClockUpdate 声明（实现在 src/）
    │
    └── hal/
        ├── README.md           # HAL 拷贝策略、调用链、Inc/Src 区别
        ├── Inc/                # HAL API 声明；多数仅 #include 依赖，不占 Flash
        │   ├── stm32f1xx_hal.h         # HAL 主头（main.h 包含）；聚合各模块 API
        │   ├── stm32f1xx_hal_def.h     # HAL 公共类型与宏（HAL 内部依赖）
        │   ├── stm32f1xx_hal_rcc.h     # ★ RCC 时钟 API 声明（main SystemClock_Config）
        │   ├── stm32f1xx_hal_rcc_ex.h  # ★ F103 PLL 扩展声明（配合 hal_rcc.c）
        │   ├── stm32f1xx_hal_gpio.h    # ★ GPIO API 声明（PC13 Init/WritePin）
        │   ├── stm32f1xx_hal_gpio_ex.h # GPIO 扩展/重映射声明（HAL 内部依赖）
        │   ├── stm32f1xx_hal_pwr.h     # ★ 电源与 Backup 域 DBP（PC13 前置）
        │   ├── stm32f1xx_hal_flash.h   # ★ Flash 等待周期 API（ClockConfig LATENCY_2）
        │   ├── stm32f1xx_hal_flash_ex.h# Flash 扩展操作声明（RCC 配置依赖）
        │   ├── stm32f1xx_hal_cortex.h  # ★ NVIC/SysTick 相关声明（HAL_Init）
        │   ├── stm32f1xx_hal_conf_template.h  # ST 官方 conf 模板；本工程用 src/stm32f1xx_hal_conf.h
        │   ├── stm32f1xx_hal_*.h       # 其余 HAL 模块头（见下表「未链入模块」）
        │   ├── stm32f1xx_ll_*.h        # LL 层寄存器直写 API（见下表「LL 层」；本 demo 未使用）
        │   └── Legacy/
        │       ├── stm32_hal_legacy.h           # HAL 旧 API 名称兼容
        │       ├── stm32f1xx_hal_can_legacy.h   # CAN 旧 API 兼容
        │       └── stm32f1xx_hal_can_ex_legacy.h
        │
        └── Src/                # ★ 以下 9 个 .c 由 CMake 编译并链入 f103-cmsis-hal.elf
            ├── stm32f1xx_hal.c           # HAL_Init、SysTick、HAL_IncTick、HAL_GetTick
            ├── stm32f1xx_hal_cortex.c      # NVIC 优先级、SysTick 配置（HAL_InitTick）
            ├── stm32f1xx_hal_gpio.c        # HAL_GPIO_Init / WritePin / ReadPin
            ├── stm32f1xx_hal_rcc.c         # HAL_RCC_OscConfig / ClockConfig
            ├── stm32f1xx_hal_rcc_ex.c      # F103 PLL/HSE 扩展配置
            ├── stm32f1xx_hal_pwr.c         # HAL_PWR_EnableBkUpAccess（Backup 域 DBP）
            ├── stm32f1xx_hal_flash.c       # Flash 等待周期设置
            ├── stm32f1xx_hal_flash_ex.c    # Flash 扩展操作（时钟配置路径依赖）
            └── stm32f1xx_hal_uart.c        # HAL_UART_Transmit（USART1 调试口）
```

★ = 本 demo 直接 `#include` 链上的头文件（经 `stm32f1xx_hal_conf.h` 或 `main.h`）。

### third_party/hal/Inc — 未链入的 HAL 模块头（仅依赖解析，无 .c，不占 Flash）

| 分类 | 头文件 |
|------|--------|
| 模拟/转换 | `hal_adc.h` · `hal_adc_ex.h` · `hal_dac.h` · `hal_dac_ex.h` · `hal_crc.h` |
| 通信 | `hal_can.h` · `hal_usart.h` · `hal_uart.h` · `hal_spi.h` · `hal_i2c.h` · `hal_i2s.h` · `hal_smartcard.h` · `hal_irda.h` · `hal_cec.h` |
| 存储/总线 | `hal_sd.h` · `hal_mmc.h` · `hal_nand.h` · `hal_nor.h` · `hal_sram.h` · `hal_pccard.h` |
| USB/以太网 | `hal_pcd.h` · `hal_pcd_ex.h` · `hal_hcd.h` · `hal_eth.h` |
| 定时/看门狗 | `hal_tim.h` · `hal_tim_ex.h` · `hal_iwdg.h` · `hal_wwdg.h` |
| DMA/中断线 | `hal_dma.h` · `hal_dma_ex.h` · `hal_exti.h` |
| RTC | `hal_rtc.h` · `hal_rtc_ex.h` |

### third_party/hal/Inc — LL 层头（本 demo 未使用）

| 头文件 | 对应外设/功能 |
|--------|----------------|
| `ll_adc.h` | ADC 寄存器直写 |
| `ll_bus.h` | 总线时钟使能宏 |
| `ll_cortex.h` | 内核/NVIC 底层 |
| `ll_crc.h` | CRC |
| `ll_dac.h` | DAC |
| `ll_dma.h` | DMA |
| `ll_exti.h` | EXTI |
| `ll_fsmc.h` | FSMC 外部存储 |
| `ll_gpio.h` | GPIO 寄存器直写 |
| `ll_i2c.h` | I2C |
| `ll_iwdg.h` | 独立看门狗 |
| `ll_pwr.h` | 电源/Backup |
| `ll_rcc.h` | RCC 寄存器直写 |
| `ll_rtc.h` | RTC |
| `ll_sdmmc.h` | SD/MMC |
| `ll_spi.h` | SPI |
| `ll_system.h` | 系统配置 |
| `ll_tim.h` | 定时器 |
| `ll_usart.h` | USART |
| `ll_usb.h` | USB |
| `ll_utils.h` | 延时/UID 等工具 |
| `ll_wwdg.h` | 窗口看门狗 |

---

## 编译与链接关系

```text
  startup_stm32f103xb.s ──┐
  src/system_stm32f1xx.c  │
  src/main.c              ├── compile (.c/.s → .o)
  src/stm32f1xx_hal_msp.c │
  src/stm32f1xx_it.c      │
  third_party/hal/Src/    │   （8 个 HAL .c）
       × 8                ──┘
                              │
                              ▼
                    linker/STM32F103XB_FLASH.ld
                              │
                              ▼
                    build/f103-cmsis-hal.elf

  third_party/cmsis/Include/*.h  ── 仅编译期 #include（寄存器定义、内核抽象）
  third_party/hal/Inc/*.h        ── 仅编译期 #include（API 声明；无对应 .c 的不占 .text）
```

CMake 定义见 [`CMakeLists.txt`](CMakeLists.txt)：`F103_HAL_SOURCES`（L19–33）、`INCLUDE_DIRS`（`src/` + CMSIS + HAL Inc）、宏 `STM32F103xB` / `USE_HAL_DRIVER` / `HSE_VALUE=8000000U`。

Reset 后执行顺序：`startup`（SystemInit → .data → .bss）→ `main`（HAL_Init → 时钟 → GPIO → 闪烁）。

---

## 维护边界（fetch 覆盖范围）

| 路径 | fetch 覆盖 | 说明 |
|------|------------|------|
| `third_party/cmsis/` · `third_party/hal/` | **是** | vendor 拷贝；顶注释由 `apply-f103-cmsis-hal-comments.sh` 恢复 |
| `startup/` · `linker/` · `src/system_stm32f1xx.c` | **是** | CMSIS 模板 + C8 裁剪 + 中文注释补丁 |
| `src/`（除 `system_stm32f1xx.c`）· 本 `README.md` | **否** | 工程维护 |
| `build/` | — | CMake 生成，不纳入版本库 |

重跑 fetch 后若 startup/linker 注释变英文：`./scripts/fetch-f103-cmsis-hal-deps.sh`（已集成 apply 补丁）。

---

## 延伸阅读

- [`doc/projects/f103-cmsis-hal.md`](../../doc/projects/f103-cmsis-hal.md) — 模块定位、与 manual-reg 对照、IDE F5 调试
- [`third_party/README.md`](third_party/README.md) — vendored 依赖、版本配对、注释约定
- [`doc/learn/cmsis-overview.md`](../../doc/learn/cmsis-overview.md) — CMSIS 与手写裸机边界
- [`doc/learn/linker-vma-lma.md`](../../doc/learn/linker-vma-lma.md) — `.data` LMA/VMA 与 startup 协作
