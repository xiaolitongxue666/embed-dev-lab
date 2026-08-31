# LSM6DS3 / LSM6DS3TR 官方参考文档

ST **LSM6DS3**（Tray）与 **LSM6DS3TR**（Tape & Reel）为同一颗硅片的不同包装，供 `projects/f103-manual-reg` SPI IMU demo 查阅。

## 文档清单

| 文件（本地） | 文档号 | 说明 |
|--------------|--------|------|
| `pdf/DocID026899-lsm6ds3tr-datasheet.pdf` | DocID026899 | LSM6DS3 / LSM6DS3TR 数据手册（英文） |

官方英文直链（更新对照用）：[https://www.st.com/resource/en/datasheet/lsm6ds3.pdf](https://www.st.com/resource/en/datasheet/lsm6ds3.pdf)

| 项 | 本仓库本地 PDF |
|----|----------------|
| DocID | DocID026899 |
| Rev | **8**（2016-02） |
| 页数 | 100 |
| 来源 | 模块附带 `LSM6DS3TR-数据手册.pdf` 拷贝入库 |

## PDF 存放约定

PDF **不提交 Git**（见根目录 `.gitignore` 中 `doc/reference/lsm6ds3/pdf/*.pdf`）。首次克隆后若缺 PDF，从模块资料或 ST 官网下载，保存为：

```text
doc/reference/lsm6ds3/pdf/DocID026899-lsm6ds3tr-datasheet.pdf
```

## 与 LSM6DS3TR-C 的区别

| 型号 | WHO_AM_I | 说明 |
|------|----------|------|
| **LSM6DS3 / LSM6DS3TR** | **`0x69`** | 本模块与本目录手册 |
| LSM6DS3TR-C | `0x6A` | 另一颗芯片；勿混用手册与期望 ID |

## Markdown 精选

[`md/`](md/) 为**中文**目录索引与 demo 相关主题摘录（附 PDF 页脚页码），**不是**全书全文翻译：

| 文档 | 内容 |
|------|------|
| [md/datasheet-index.md](md/datasheet-index.md) | 章节目录索引 |
| [md/topics/electrical-spi-timing.md](md/topics/electrical-spi-timing.md) | 供电、上电、SPI 时序上限 |
| [md/topics/spi-protocol.md](md/topics/spi-protocol.md) | 4 线 SPI 读写帧、CS、`IF_INC` |
| [md/topics/registers-whoami-imu.md](md/topics/registers-whoami-imu.md) | WHO_AM_I、CTRL、STATUS、OUT、灵敏度 |

编写规范见 [md/README.md](md/README.md)。

## 补充中文资料（非本目录权威源）

ST 中文社区 AN4650 译文（应用笔记，可浏览/下附件）：

- [https://shequ.stmicroelectronics.cn/thread-629614-1-1.html](https://shequ.stmicroelectronics.cn/thread-629614-1-1.html)

英文 AN4650：[https://www.st.com/resource/en/application_note/dm00157511.pdf](https://www.st.com/resource/en/application_note/dm00157511.pdf)

寄存器与电气特性以本目录 **DocID026899 PDF** 为准；AN4650 仅作用法补充（如上电 20 ms boot）。
