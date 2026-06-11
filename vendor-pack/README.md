# vendor-pack — 本地厂商参考资料

本目录存放**厂商提供的本地资料包**（驱动、板级例程、STM32Cube 固件等），与 [`doc/reference/`](../doc/reference/)（ST 官方 PDF + 精选 MD）分工不同。

## 目录说明

| 子目录 | 内容 | Git |
|--------|------|-----|
| `STLink/STLink/USBDriver/` | ST-Link WinUSB 驱动（probe-rs 需要） | 可提交 |
| `STM32F103C8T6核心板/` | 核心板 MDK 例程、PCB 等 | 忽略（体积大，仅本地） |
| `STM32CubeF1/` | ST 官方 F1 固件包（CMSIS/HAL/例程） | 仅 README 提交；ZIP 与解压目录忽略 |

## 获取 STM32CubeF1

```bash
./scripts/fetch-stm32cubef1.sh              # 优先解压 archives/*.zip，否则 git clone
./scripts/fetch-stm32cubef1.sh --from-zip PATH   # 解压从 ST 官网下载的 ZIP
./scripts/fetch-stm32cubef1.sh --clone        # git clone --recursive v1.8.6
./scripts/fetch-stm32cubef1.sh --verify-only  # 检查 CMSIS startup 是否存在
```

官网：[STM32CubeF1](https://www.st.com/en/embedded-software/stm32cubef1.html) → Get Software → ZIP（可能需 ST 账号）。下载后放到 `STM32CubeF1/archives/` 再运行 fetch 脚本。

## 相关文档

- [probe-rs 与 WinUSB](../doc/probe-rs.md)
- [f103-blink 模块](../doc/modules-f103-blink.md)
- [STM32 裸机入门笔记](../doc/learn/stm32-bare-metal-bootstrap.md)
