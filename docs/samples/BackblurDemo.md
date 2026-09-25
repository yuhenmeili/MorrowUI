# BackblurDemo — Kawase 共享背景模糊（S1/S2）

对应源码：`samples/BackblurDemo.cpp`（编译目标同名）。
设计文档：[KAWASE_BACKDROP_BLUR_PROPOSAL.md](../road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md)。

## 用途

演示 `BackdropBlur` 组件驱动的毛玻璃 / acrylic 背景模糊，覆盖 S1/S2 的验收场景：

1. **分段渲染语义**：卡片墙置于 `displayLayer = -5`（分段边界以下，进入
   模糊源）；毛玻璃面板在默认层 0；前景清晰条与切换按钮在层 3（边界以上，
   保持锐利）；
2. **三级半径分级**：轻 8px（L0'）/ 中 24px ×2 面板（L1'，验证层级内
   合批）/ 重 56px（L2'），全部采样同一条共享链，模糊强度肉眼可辨地
   递增；面板之间不互相出现在对方的模糊背景里；
3. **backdrop 缓存（S2）**：背景静止时（`--static`）脏标记命中，backdrop
   段与模糊链整段跳过（`Blur:4q/4d`），仅重绘回屏合成 + 面片 + 前景；
   呼吸动画则每帧重渲（`Blur:4q/10d`）；
4. **运行中开/关与档位切换**：按钮翻转 `setEnabled` / 循环 `setQuality`
   （Off / Standard / LowCost），关闭后毛玻璃降级为 tint 半透明面板
   （"关闭 ≠ 删组件"），档位切换懒重建 RT 链。

## 运行方式

```bash
BackblurDemo                            # 交互模式：呼吸动画 + 两枚切换按钮
BackblurDemo --static                   # 背景静止（验证 backdrop 缓存命中）
BackblurDemo --quality lowcost          # 1/4 基准链（低配档位）
BackblurDemo --quality off              # 全程退化路径（等价 --no-blur）
BackblurDemo --overlay                  # 启动即开启调试面板（F3 也可切换），
                                        # 可看到 "Blur:4q/10d"（--static 下 4q/4d）
BackblurDemo --report-json              # 限 5 帧，输出统计 JSON（含 avgFrameMs）
                                        # 并按模式做 S2 验收断言
BackblurDemo --frames N                 # 限制渲染帧数（自动化）
```

## 验收断言（--report-json，统计取末帧）

| 模式 | 断言 |
|---|---|
| 默认（呼吸动画，每帧脏） | `backdropQuads = 4`；`backdropDrawCalls = 10`（链 3 级 × 2 pass + 合成 1 + 面片 3 层级各 1 draw） |
| `--static`（缓存命中） | `backdropQuads = 4`；`backdropDrawCalls = 4`（链与 backdrop 段整段跳过；连带 backdrop 批次不画，batchDrawCalls 显著下降） |
| `--quality off` | `backdropQuads = 0`、`backdropDrawCalls = 0`，且 `drawCalls == batchDrawCalls`（模糊子系统零成本） |

性能观测：JSON 中 `avgFrameMs` 为帧间隔均值（桌面 GL 含 vsync/FPS 控制器，
仅作方向性参考；GPU 带宽折算与 <3ms 目标须在目标 SoC 实测）。

## 相关组件与 API

| 名称 | 位置 | 说明 |
|---|---|---|
| `BackdropBlur` | `src/ui/effects/BackdropBlur.h` | 组件式 opt-in：`setBlurRadius`（量化到 L0/L1/L2：≤12 / ≤40 / >40 px）/ `setTintColor` / `setRounding(FollowOwner)`；启用时向共享链提交面片，关闭时降级为 tint 面板（Shadow 同款自持材质 + 普通通道自然顺序） |
| `BackdropBlurManager` | `src/ui/effects/BackdropBlurManager.h` | 单例：三级 RT 链（每级 downsample + kawase 一对 RT）、脏标记缓存（批次结构等价 + Transform/Material/Mesh 版本号聚合签名）、`setEnabled` 全局开关、`setQuality` 档位（Off / Standard=1/2 基准 / LowCost=1/4 基准，懒重建） |
| `EngineOptions.backdropBlur` | `src/core/Engine.h` | 启动档位，缺省 Standard |
| shader | `assets/shaders/backdrop*.vert/.frag` | downsample（4-tap box）/ kawase / composite（回屏拷贝）/ backdrop（面片采样 + 圆角 + acrylic 混色，不透明替换）/ backdrop_tint（降级纯色面板） |

## 注意事项

- 属主面板应使用**透明背景**（模糊面片即面板背景，不透明替换其下的清晰
  内容）；普通子内容叠在其上；
- `tint.a` 是混色强度而非透视度（`结果 = mix(模糊背景, tint.rgb, tint.a)`）；
- S2 语义取舍：模糊源只包含分段边界以下的内容（提案 §4 方案 A）；层内半径
  不连续可调（分级量化）；
- 与 `MR3DSceneView` 混用时，其显示面片（update 阶段直绘屏）会被回屏合成
  覆盖——该组合的兼容性留待后续阶段处理；
- 模糊面片暂不参与 clipRect 裁剪（滚动容器内使用见提案 S3）。
