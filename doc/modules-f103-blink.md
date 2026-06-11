# f103-blink 模块

**STM32F103C8T6** 核心板 **PC13 LED 闪烁** demo，纯寄存器实现，对齐厂商例程 `install_packet/.../核心板测试程序(PC13闪烁)`。

## 目录结构

```text
modules/f103-blink/
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── main.c              # GPIO 初始化与闪烁主循环
│   ├── system_stm32f10x.c  # SystemInit，HSE→72 MHz
│   └── gpio_like51.h       # PCout(n) 位带宏
├── startup/
│   └── startup_stm32f103xb.s   # 向量表、.data/.bss、Reset_Handler
└── linker/
    └── stm32f103c8.ld      # 64K Flash / 20K RAM
```

## 构建与烧录

```bash
./scripts/build.sh f103-blink          # configure + build
./scripts/build.sh f103-blink build
./scripts/build.sh f103-blink flash
```

产物：

- `modules/f103-blink/build/f103-blink.elf`
- `modules/f103-blink/build/f103-blink.hex`

probe-rs chip：**`STM32F103C8Tx`**

## 时钟

`SystemInit()`（startup 在 `main` 前调用）尝试 **HSE 8 MHz × PLL9 → 72 MHz**。HSE 超时则保持复位默认 **HSI 8 MHz**，避免无晶振板卡死。

## PC13 与 Backup 域

PC13 属于 **Backup 域** GPIO，配置前必须：

1. `RCC_APB1ENR.PWREN` — 开启 PWR 时钟  
2. `PWR_CR.DBP` — 解除 Backup 域写保护  

否则 `GPIOC_CRH` 写入无效，LED 不亮。见 `src/main.c` 中 `gpio_configuration()`。

详细寄存器说明与 PDF 页码：[Backup 域与 PC13](reference/stm32f103/md/topics/backup-domain-pc13.md)

## LED 极性

多数核心板 **低电平点亮**：

- `PCout(13) = 1` → 灭  
- `PCout(13) = 0` → 亮  

闪烁逻辑：高 → 延时 → 低 → 延时（与厂商 `main.c` 一致）。

## 硬件参考

| 项目 | 说明 |
|------|------|
| LED 引脚 | PC13 |
| SWD | SWDIO=PA13, SWCLK=PA14 |
| 厂商例程 | `install_packet/STM32F103C8T6核心板/.../核心板测试程序(PC13闪烁)/` |

## 调试

- IDE：**F103 Probe-rs Debug**（`.vscode/launch.json`）
- CLI：见 [probe-rs.md](probe-rs.md)

## 新增模块参考

1. 复制 `modules/f103-blink/` 结构  
2. 使用 `cmake/mcu-config.cmake` 中 `embed_mcu_add_executable()`  
3. `./scripts/build.sh <新模块名>`
