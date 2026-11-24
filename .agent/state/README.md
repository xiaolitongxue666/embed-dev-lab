# State

Memory 记录已经发生的事实，在 [`.agent/memory/`](../memory/)。State 应该是「当前这次任务做到哪一步」。

本仓库不保存任务检查点，没有可恢复的执行快照。中断后从 [workflow](../workflows/write-build-flash.md) 判断板子和构建目录现在处于哪一步，再打开对应 Skill。不要假设上次会话的烧录或串口切换还有效：先看供电、`build/` 是否在、CH341 在哪一侧。
