# BackblurDemo — Kawase 共享背景模糊（S1）

对应源码：`samples/BackblurDemo.cpp`（编译目标同名）。
设计文档：[KAWASE_BACKDROP_BLUR_PROPOSAL.md](../road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md)。

## 用途

演示 `BackdropBlur` 组件驱动的毛玻璃 / acrylic 背景模糊（S1 范围：单层级
L0'、固定半径档、无内容缓存），覆盖三类验收场景：

1. **分段渲染语义**：卡片墙置于 `displayLayer = -5`（分段边界以下，进入
   模糊源）；毛玻璃面板在默认层 0；前景清晰条与切换按钮在层 3（边界以上，
   保持锐利）；
2. **共享模糊链**：两块交叠面板 + 一枚胶囊（不同 tint / 圆角）全部采样同
   一条 1/2 分辨率模糊链，合并为 1 个 draw call（面板之间不互相出现在对方
   的模糊背景里）；
3. **运行中开/关切换**：按钮翻转 `BackdropBlurManager::setEnabled`，关闭后
   毛玻璃降级为 tint 半透明面板（"关闭 ≠ 删组件"），帧结构退化为现有单段
   路径，模糊子系统零成本。

卡片墙中间一行做 alpha 呼吸动画，验证模糊背景随内容实时更新。

## 运行方式

```bash
BackblurDemo                      # 交互模式，按钮切换全局开关
BackblurDemo --overlay            # 启动即开启调试面板（F3 也可切换），
                                  # 可看到 "Blur:3q/5d" 统计
BackblurDemo --report-json        # 限 5 帧，输出统计 JSON 并做 S1 验收断言
BackblurDemo --report-json --no-blur   # 以关闭状态启动并断言降级路径
BackblurDemo --frames N           # 限制渲染帧数（自动化）
```

## 验收断言（--report-json）

| 模式 | 断言 |
|---|---|
| 默认（开启） | `backdropQuads = 3`（三个模糊面片合并为 1 个 draw）；`backdropDrawCalls = 5`（downsample + kawase×2 + 回屏合成 + 面片绘制） |
| `--no-blur` | `backdropQuads = 0`、`backdropDrawCalls = 0`，且 `drawCalls == batchDrawCalls`（全部 draw 均来自普通批次，模糊子系统零成本） |

## 相关组件与 API

| 名称 | 位置 | 说明 |
|---|---|---|
| `BackdropBlur` | `src/ui/effects/BackdropBlur.h` | 组件式 opt-in：`setBlurRadius` / `setTintColor` / `setRounding(FollowOwner)`；启用时向共享链提交面片，关闭时降级为 tint 面板（Shadow 同款自持材质 + 普通通道自然顺序） |
| `BackdropBlurManager` | `src/ui/effects/BackdropBlurManager.h` | 单例：持有 backdrop RT（1/2 分辨率）+ ping-pong 对、pass 材质与全屏面片；`setEnabled` 全局开关；由 `BatchManager::renderBatches` 驱动"分段渲染 + 链 + 回屏合成"帧结构 |
| shader | `assets/shaders/backdrop*.vert/.frag` | downsample（4-tap box）/ kawase / composite（回屏拷贝）/ backdrop（面片采样 + 圆角 + tint）/ backdrop_tint（降级纯色面板） |

## 注意事项

- 属主面板应使用**透明背景**（模糊面片即面板背景）；普通子内容叠在其上；
- S1 语义取舍：模糊源只包含分段边界以下的内容（提案 §4 方案 A）；
- 与 `MR3DSceneView` 混用时，其显示面片（update 阶段直绘屏）会被回屏合成
  覆盖——该组合的兼容性留待后续阶段处理；
- 模糊面片暂不参与 clipRect 裁剪（滚动容器内使用见提案 S3）。
