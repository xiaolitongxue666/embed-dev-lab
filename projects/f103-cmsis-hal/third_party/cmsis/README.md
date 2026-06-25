# cmsis — CMSIS-Core / CMSIS-Device 头文件

## 拷贝清单（`fetch-f103-cmsis-hal-deps.sh`）

| 文件 | 层级 | 本 demo 作用 |
|------|------|----------------|
| `core_cm3.h` | CMSIS-Core | Cortex-M3 内核寄存器、NVIC、SysTick 定义 |
| `cmsis_compiler.h` | CMSIS-Core | 编译器抽象入口 |
| `cmsis_version.h` | CMSIS-Core | CMSIS 版本号 |
| `cmsis_gcc.h` | CMSIS-Core | GCC/Arm Compiler 内联 intrinsic |
| `stm32f1xx.h` | CMSIS-Device | 设备系列入口；定义 `USE_HAL_DRIVER`、`STM32F103xB` |
| `stm32f103xb.h` | CMSIS-Device | **寄存器映射**（RCC、GPIO、PWR 等基地址与位域） |
| `system_stm32f1xx.h` | CMSIS-Device | `SystemInit` / `SystemCoreClockUpdate` 声明 |

## 与工程其他部分的关系

```text
stm32f1xx.h
    └── stm32f103xb.h          ← 外设寄存器（HAL 底层读写）
    └── system_stm32f1xx.h
            └── src/system_stm32f1xx.c   ← SystemInit 实现（CMSIS 模板）

core_cm3.h                     ← startup 向量表、Fault 异常编号
```

- **编译宏**：CMake 定义 `STM32F103xB`、`USE_HAL_DRIVER`，使 `stm32f1xx.h` 包含 HAL 而非纯寄存器 API。
- **不包含 .c**：CMSIS 设备层仅头文件；`system_stm32f1xx.c` 在 `src/` 维护。
- **注释**：ST 英文 Doxygen 保留；文件顶可有 embed-dev-lab 中文 `@note`（fetch 后 apply 恢复）。

## 来源路径

```text
vendor-pack/cmsis-core/Include/     → third_party/cmsis/Include/
vendor-pack/cmsis-device-f1/Include/ → third_party/cmsis/Include/
```

Tag：`cm3` / `v5.6.0_cm3`（Core），`v4.3.5`（Device F1）。
