# 固件验收

Agent 说做完了，不算做完。用下面的确定性检查。没有第二套模型打分。

## 编译

目标模块：

```bash
./scripts/build.sh <module> build
```

`build/` 不存在或刚 `clean` 过时，改用 `./scripts/build.sh <module>`。两条路线的 demo 都改了时：

```bash
./scripts/build.sh both build
```

通过：命令退出码为 0，且 `projects/<module>/build/<module>.elf` 存在。编译失败则任务未完成。

## 烧录

只在用户要求烧录，或任务本身要在板上验证时做。通过：`./scripts/build.sh <module> flash` 退出码为 0，且没有未经确认的全片擦除。

## 串口

需要看日志时，按 [serial-console](../skills/serial-console/SKILL.md) 读取。demo 在 2026-09-21 台架上的形态是（波特以当前固件为准，当时为 1500000）：

- 启动有绿色 `[I]`
- 约每秒一行青色 DEBUG：`[HH:MM:SS][D] LED on|off  KEY high|low  knob raw=…  clock 00:00:00:00`
- `LED on|off` 是 GPIO 电平。`KEY high|low` 是 IDR

BMP280 在那块台上经常初始化失败，温度位置为 `0.00C`。不要把「没有温度」单独当成这次改动失败，除非任务改的就是 BMP280。

乱码先按串口 Skill 对波特和宿主，不要把乱码当成功能已通过。
