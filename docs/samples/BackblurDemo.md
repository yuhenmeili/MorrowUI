# BackblurDemo — Kawase 共享背景模糊（S1/S2/S3）

对应源码：`samples/BackblurDemo.cpp`（编译目标同名）。
设计文档：[KAWASE_BACKDROP_BLUR_PROPOSAL.md](../road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md)。

## 用途

演示 `BackdropBlur` 组件驱动的毛玻璃 / acrylic 背景模糊，覆盖 S1/S2/S3 的验收场景：

1. **分段渲染语义**：卡片墙置于 `displayLayer = -5`（分段边界以下，进入
   模糊源）；毛玻璃面板在默认层 0；前景清晰条与切换按钮在层 3（边界以上，
   保持锐利）；
2. **三级半径分级**：轻 8px（L0'）/ 中 24px ×2 面板（L1'，验证层级内
   合批）/ 重 56px（L2'），全部采样同一条共享链，模糊强度肉眼可辨地
   递增；面板之间不互相出现在对方的模糊背景里；
3. **半径动画（S3 双层混合插值）**：`setBlurLevel(0..2)` 往返动画——层级
   固定、链不变，小数部分在相邻层级纹理间插值（每顶点混合因子，动画
   面片仍合并为 1 draw）；
4. **裁剪容器（S3 clipRect 专项）**：660px 宽容器内的 900px 模糊面板，
   溢出部分被精确剪掉（按 `currentClip` 分组施加 scissor），横向缓动验证
   运行中裁剪；
5. **backdrop 缓存（S2）**：`--static` 停止全部动画后脏标记命中，backdrop
   段与模糊链整段跳过（`Blur:6q/6d`）；动画期每帧重渲（`Blur:6q/12d`）；
6. **运行中开/关与档位切换**：按钮翻转 `setEnabled` / 循环 `setQuality`
   （Off / Standard / LowCost），关闭后毛玻璃降级为 tint 半透明面板
   （"关闭 ≠ 删组件"），档位切换懒重建 RT 链。

上采样为 9-tap tent（S3）：低分辨率层级放大在近景文字边缘更平滑。

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
| 默认（呼吸 + 半径动画，每帧脏） | `backdropQuads = 6`；`backdropDrawCalls = 11~12`（链 3 级 × 2 pass + 合成 1 + 面片 5 组：L0 / L1(B+B2) / L2 / 动画双层组 / 裁剪组；动画恰落在整数层级时并入同层组少 1） |
| `--static`（缓存命中） | `backdropQuads = 6`；`backdropDrawCalls = 6`（链与 backdrop 段整段跳过；连带 backdrop 批次不画，batchDrawCalls 显著下降） |
| `--quality off` | `backdropQuads = 0`、`backdropDrawCalls = 0`，且 `drawCalls == batchDrawCalls`（模糊子系统零成本） |

性能观测：JSON 中 `avgFrameMs` 为帧间隔均值（桌面 GL 含 vsync/FPS 控制器，
仅作方向性参考；GPU 带宽折算与 <3ms 目标须在目标 SoC 实测）。

## 相关组件与 API

| 名称 | 位置 | 说明 |
|---|---|---|
| `BackdropBlur` | `src/ui/effects/BackdropBlur.h` | 组件式 opt-in：`setBlurRadius`（量化到 L0/L1/L2：≤12 / ≤40 / >40 px）/ `setBlurLevel`（连续 0..2，半径动画双层插值）/ `setTintColor` / `setRounding(FollowOwner)`；启用时向共享链提交面片（含 currentClip），关闭时降级为 tint 面板 |
| `BackdropBlurManager` | `src/ui/effects/BackdropBlurManager.h` | 单例：三级 RT 链、脏标记缓存、面片分组绘制（层级对 × clipRect，9-tap tent 上采样 + 双层插值）、`setEnabled` / `setQuality` 档位 |
| `EngineOptions.backdropBlur` | `src/core/Engine.h` | 启动档位，缺省 Standard |
| shader | `assets/shaders/backdrop*.vert/.frag` | downsample / kawase / composite / backdrop（tent + 双层插值 + 圆角 + acrylic 混色）/ backdrop_tint（降级纯色面板） |

## 注意事项

- 属主面板应使用**透明背景**（模糊面片即面板背景，不透明替换其下的清晰
  内容）；普通子内容叠在其上；
- `tint.a` 是混色强度而非透视度（`结果 = mix(模糊背景, tint.rgb, tint.a)`）；
- S2 语义取舍：模糊源只包含分段边界以下的内容（提案 §4 方案 A）；层内半径
  不连续可调（分级量化）；
- 与 `MR3DSceneView` 混用时，其显示面片（update 阶段直绘屏）会被回屏合成
  覆盖——该组合的兼容性留待后续阶段处理；
- 模糊面片暂不参与 clipRect 裁剪（滚动容器内使用见提案 S3）。
