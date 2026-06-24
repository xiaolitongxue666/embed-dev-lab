# STM32F103 官方参考文档

ST 官方 **Datasheet（DS5319）** 与 **Reference Manual（RM0008）**，供 `projects/f103-manual-reg` 及后续 F103 模块查阅。

## 与 vendor-pack 的区别

| 位置 | 内容 |
|------|------|
| [`vendor-pack/STM32F103C8T6核心板/`](../../../vendor-pack/) | 板级 MDK 例程、PCB、厂商测试程序（本地资料） |
| [`vendor-pack/STM32CubeF1/`](../../../vendor-pack/STM32CubeF1/) | ST 官方 F1 固件包 CMSIS/HAL（`fetch-stm32cubef1.sh`） |
| **本目录** | ST 官网 Datasheet / Reference Manual（权威寄存器与电气特性） |

## 文档清单

| 文件（本地） | 文档号 | 说明 |
|--------------|--------|------|
| `pdf/DS5319-stm32f103x8xB-datasheet.pdf` | DS5319 | STM32F103x8/xB medium-density（含 **F103C8**） |
| `pdf/RM0008-stm32f10xxx-reference-manual.pdf` | RM0008 | STM32F10xxx 全族参考手册（F103 寄存器细节） |

官方 URL 见 [`scripts/fetch-stm32f103-docs.sh`](../../../scripts/fetch-stm32f103-docs.sh) 内常量。

## 下载

PDF **不提交 Git**（见根目录 `.gitignore`）。首次使用或更新文档时：

```bash
./scripts/fetch-stm32f103-docs.sh
./scripts/fetch-stm32f103-docs.sh --verify-only   # 仅校验本地 PDF
./scripts/fetch-stm32f103-docs.sh --force          # 强制重新下载
./scripts/fetch-stm32f103-docs.sh --no-proxy       # 禁用默认代理
```

**ST 官网无需注册、无需登录**，文档页可直接点 PDF 下载。若 curl 因网络超时失败，可用浏览器打开直链保存至 `pdf/` 目录后执行 `--verify-only`。

| 本地文件 | 典型来源 | 版本说明 |
|----------|----------|----------|
| `DS5319-...pdf` | ST 直链 `stm32f103c8.pdf` | DS5319 Rev 20（114 页） |
| `RM0008-...pdf` | ST 直链 或 Keil 镜像 | 脚本优先 ST Rev 21；Keil 镜像可能为 Rev 9（995 页），寄存器一致、页码不同 |

下载完成后 SHA256 写入 [`checksums.sha256`](checksums.sha256)。

## Markdown 精选

[`md/`](md/) 目录含章节目录索引与 **f103-manual-reg 相关主题** 摘录（附 PDF 页码，已与源码寄存器核对）：

| 文档 | 内容 |
|------|------|
| [md/datasheet-index.md](md/datasheet-index.md) | DS5319 目录索引 |
| [md/rm0008-index.md](md/rm0008-index.md) | RM0008 目录索引（标注 F103 相关章节） |
| [md/topics/backup-domain-pc13.md](md/topics/backup-domain-pc13.md) | Backup 域、PWR、PC13 GPIO |
| [md/topics/rcc-clock-hse-pll.md](md/topics/rcc-clock-hse-pll.md) | RCC、HSE→72 MHz |
| [md/topics/memory-map-medium-density.md](md/topics/memory-map-medium-density.md) | 64K Flash / 20K RAM |

编写规范见 [md/README.md](md/README.md)。
