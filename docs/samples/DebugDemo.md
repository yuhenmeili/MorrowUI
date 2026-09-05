# DebugDemo

- 对应源码：`samples/DebugDemo.cpp`
- 编译目标：`DebugDemo`
- 拆分说明：本 demo 的能力原内嵌在旧 `ImageDemo` 中，现独立出来专门演示引擎调试设施

## Demo 用途

集中演示 `Engine` 的调试 / 观测设施，同时提供一个**合批回归验收基准**：

| 设施 | 用法 | 说明 |
|---|---|---|
| 批渲染统计 JSON | `--report-json` | 退出时输出 `batchStatistics`，并断言合批预期，失败返回码 2 |
| 帧数限制 | `--frames N` | 渲染 N 帧后退出，适合自动化 / 截图测试 |
| 按需渲染 | `--request-render` | 无脏事件不重绘，验证 request-render 路径 |
| 调试 overlay | `--overlay` 或运行中按 **F3** | 屏幕上显示 FPS / 批次 / draw call |
| 对象快照 | `--object-snapshot PATH` | 退出时写出对象注册表快照（自动追加 `-frame-N` 后缀） |
| 快照命令文件 | `--object-snapshot-command PATH` | 外部工具写 `snapshot path=<file>` 触发运行期快照（命令文件消费后即删除） |

## 启动参数

```text
DebugDemo [options]
  --report-json                    print batch statistics JSON and assert batch acceptance
  --frames N                       stop after N frames
  --request-render                 on-demand rendering mode
  --overlay                        start with the debug overlay visible (F3 toggles)
  --object-snapshot PATH           write object registry snapshot on exit
  --object-snapshot-command PATH   heartbeat command file triggering runtime snapshots
```

## 运行方式

```bash
cmake --build build --target DebugDemo --parallel 8

# 最常用：跑 5 帧输出统计并验收
./build/DebugDemo.exe --frames 5 --report-json

# 打开调试 overlay + 退出时导出对象快照
./build/DebugDemo.exe --overlay --object-snapshot snapshot.json
```

## 验收基准

场景刻意保持简单且确定：**10 张共享同一纹理的 `MRImage` + 1 个带阴影的 `MRColor`
+ 1 个带阴影的 `MRImage`**。桌面 GL（SSBO 可用）下的预期统计：

```json
{"renderItems":12,"batches":4,"ssboBatches":1,"standardBatches":3,"batchDrawCalls":5,"drawCalls":5,"cacheHits":1,"cacheMisses":0,"ssboSupported":true}
```

- 10 张图片同纹理同材质 → 合并为 **1 个 SSBO 批次**（`ssboBatches=1`）；
- 色块材质与两个阴影材质各自成批（`standardBatches=3`）；
- `--report-json` 对以上数字做硬断言，任何合批回归（材质哈希、SSBO 布局、
  批次拆分逻辑的改动）都会使命令以返回码 2 失败。

## 示例做了什么

1. 解析调试参数并填入 `EngineOptions`（`maxFrames` / `enableRequestRender` /
   `debugOverlayVisible` / `objectSnapshotPath` / `objectSnapshotCommandPath`）。
2. 搭建验收基准场景（见上）。
3. 启动时打印操作提示（F3 切 overlay、命令文件格式等）。
4. `engine->render()` 主循环；`--report-json` 时输出统计 JSON 并执行断言。

## 相关组件

### `EngineOptions` 调试字段
- `maxFrames`：0 表示一直运行到窗口关闭；配合 `--report-json` 会自动取 5。
- `enableRequestRender`：按需渲染开关。
- `objectSnapshotPath`：退出时经 `Engine::writeObjectSnapshot` 写出
  `ObjectRegistry` 快照（JSON，带帧号）。
- `objectSnapshotCommandPath`：心跳命令文件，内容 `snapshot path=<file>` 或
  `snapshot <file>`，消费一次即删除；供外部分析工具在运行期取快照。

### 调试 overlay
- 运行中按 **F3**（或 `--overlay` 起始可见）切换，显示 FPS / 批次数 /
  draw call 等实时统计；构建开关 `MORROW_ENABLE_OBJECT_DIAGNOSTICS` /
  `MORROW_ENABLE_DEBUG_OVERLAY`。

### `batchStatistics`
- `FrameState::batchStatistics` 提供 renderItems / batches / ssboBatches /
  standardBatches / cacheHit/Miss 等计数，是验证 SSBO 批渲染是否生效的直接手段；
  本 demo 展示了把它固化成回归断言的模式，其他 demo 也可复制。
