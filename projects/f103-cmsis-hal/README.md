# f103-cmsis-hal

基于 **CMSIS-Core** + **CMSIS-Device F1** + **STM32F1 HAL** 的 F103 PC13 闪烁 demo，与 [`f103-manual-reg`](../f103-manual-reg/) 功能对等、构建框架一致。

| 项 | 说明 |
|----|------|
| 芯片 | STM32F103C8T6 |
| 构建 | `./scripts/build.sh f103-cmsis-hal`（CMake Preset，同 manual-reg） |
| 链接脚本 | CMSIS `linker/STM32F103XB_FLASH.ld`（非 manual-reg 手写版） |
| 文档 | [`doc/projects/f103-cmsis-hal.md`](../../doc/projects/f103-cmsis-hal.md) |

## 依赖

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
```

拷贝来源：

- CMSIS：`vendor-pack/cmsis-core` + `vendor-pack/cmsis-device-f1`
- HAL 参考：`stm32f1xx-hal-driver@v1.1.8`（clone 至 `.tools/`，最小 Inc/Src 进 `third_party/`）

## 构建与烧录

```bash
./scripts/build.sh f103-cmsis-hal build
./scripts/build.sh f103-cmsis-hal flash
./scripts/build-flash.sh f103-cmsis-hal
```
