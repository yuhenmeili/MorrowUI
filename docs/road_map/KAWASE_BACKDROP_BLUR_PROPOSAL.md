# 全屏多 Widget 的 Kawase 背景模糊方案（设计提案）

> **背景**：毛玻璃 / acrylic 风格面板是车载 UI 的常见需求。当前的模糊类效果
> 只有 SDF 解析式阴影（`Shadow` 组件，`shadow.frag`）与个别业务 shader 自带
> 的局部效果，引擎没有"对已绘制内容做真实卷积模糊再采样"的能力。本文针对
> **全屏、大量 UIWidget 同时需要背景模糊**这一最重场景，给出基于 Kawase
> 模糊的分阶段落地方案。

> **状态**：S1、S2、S3（前三项）已实施（2026-09-25，验收见
> `samples/BackblurDemo.cpp` 与 [BackblurDemo.md](../samples/BackblurDemo.md)）；
> S3 第 4 项（方案 C 多分段点）维持按需。分析基线：dev 分支 `8b1dc5d`
> （2026-09-24）。文中所有代码位置均已逐一核实。
>
> **S1 实现偏差**（相对 §5/§6 原文，两处有意取舍）：
> 1. 模糊面片未走"SSBO 实例布局扩展 + backdropUV 字段"路线，改为
>    `BackdropBlurManager` 在 CPU 侧把全部面片烘焙进一个自管 VBO（每顶点
>    携带采样 UV / 圆角 / tint）一次绘制。同样满足"全部面片 1 个 draw
>    call"，且不触发布局 static_assert / 全宿主 shader 同步，也无非 SSBO
>    回退分叉；S2 分级时再评估是否并入统一实例布局。
> 2. tint 降级路径按 §5.7 语义实现为：组件复用属主 MeshFilter/Transform 走
>    普通通道注册（Shadow 同款自然顺序），而非"共享 tint 材质合并 1 批"。

---

## 1. 文档定位与核心结论（TL;DR）

**目标场景**：1080p~2K 全屏 UI；数十个面板/卡片同时开启背景模糊；模糊半径
各不相同；模糊面片带圆角；QNX（GLES 3.x，tile-based GPU）为主、Windows/Linux
GL 为开发宿主；60fps 预算内模糊子系统总成本目标 **< 3ms（GPU 带宽折算）**，
静止场景零额外成本。

**核心结论**：

1. **逐 Widget 独立模糊不可行**。N 个模糊 Widget 意味着 N 次 backdrop
   拷贝与 N 条 pass 链，全屏场景成本随 Widget 数量线性放大。正确思路是
   **一次共享模糊链服务全部模糊 Widget**。
2. **推荐架构 = "分段渲染 + 共享模糊链 + 分级采样"**：
   - 帧内按 `displayLayer` 边界分两段：边界以下内容先画进离屏 RT（backdrop）；
   - 对 backdrop 跑一条降采样 + Kawase 逐级加深的 pass 链，保留中间层级；
   - 回屏恢复背景后，所有模糊 Widget 用**同一个 backdrop 采样材质**画自己的
     矩形区域，按半径采样链上不同层级 → **全部模糊面片仍然合成 1 个 SSBO 批次**，
     不破坏现有合批架构。
3. **静止场景零成本**依赖两道缓存：request-render 门控（已有）+ backdrop
   内容未变则整链跳过（新增脏标记）。UI 大多数时间静止，这是车载场景的关键。
4. Kawase 选型本身没有悬念：每 pass 4~8 tap、无大核采样数组、带宽友好，
   是 GLES 上大半径模糊的公认方案（iOS backdrop、各大游戏 UI 同款思路）。
5. **毛玻璃与普通 UI 天然混存、可分级降级**：模糊是按 Widget 粒度的组件式
   opt-in，普通 Widget 零感知；全局开关 / 启动档位 / 单 Widget 三级控制，
   关闭时毛玻璃降级为 tint 半透明面板（iOS"降低透明度"同款思路），
   零成本回到普通路径（见 §5.7）。

---

## 2. Kawase 模糊原理与选型

### 2.1 算法

Kawase 模糊用**多 pass 小核近似大高斯**：每个 pass 在"十字 + 对角"位置采
4~5 个 tap 取平均，采样偏移随 pass 序号递增（`offset = pass + 0.5` 像素 ×
当前分辨率下的缩放）。由于每 pass 之后的模糊核近似为上一轮的卷积叠加，
K 个 pass 后等效半径约 `K²` 量级增长——半径翻倍只需多 1 个 pass，而不是
成倍增加 tap 数。

```glsl
// kawase pass 片段核心（示意，texel = 1/分辨率）
vec3 c = texture(u_src, v_uv).rgb * 4.0;
c += texture(u_src, v_uv + vec2( u_offset,  u_offset) * texel).rgb;
c += texture(u_src, v_uv + vec2(-u_offset,  u_offset) * texel).rgb;
c += texture(u_src, v_uv + vec2( u_offset, -u_offset) * texel).rgb;
c += texture(u_src, v_uv + vec2(-u_offset, -u_offset) * texel).rgb;
fragColor = vec4(c / 8.0, 1.0);
```

完整链路（经典 dual-Kawase 变体，按本项目需求裁剪）：

```text
backdropRT (全屏或 1/2)
   │  downsample（box/bilinear 4 tap）
   ▼
RT_L0 (1/2)  ──kawase(offset=0.5)──▶ RT_L0'
   │  downsample
   ▼
RT_L1 (1/4)  ──kawase(offset=1.5)──▶ RT_L1'
   │  downsample                     │ ← 保留为"大半径"采样层
   ▼
RT_L2 (1/8)  ──kawase(offset=2.5)──▶ RT_L2'   ← "特大半径"采样层
```

模糊 Widget 按半径映射到 L0'/L1'/L2' 之一采样（bilinear 上采样回屏）。

### 2.2 方案对比

| 方案 | pass 数 / tap 数 | 带宽 | 质量 | 结论 |
|---|---|---|---|---|
| 大核 Gaussian（可分离） | 2 pass × 2R+1 tap | 随半径线性涨 | 最好 | 半径 64 时 130 tap，GLES 上不现实 |
| 多 pass Box 叠加 | 3~4 pass × 9 tap | 中 | 中（网格伪影） | 可用但同等质量下 pass 更多 |
| **Kawase（本方案）** | 3~5 pass × 4~5 tap | 最低 | 良好（面板背景够用） | **选定** |
| Dual-Kawase（上下行两条链） | 上行 5 tap + 下行 8 tap | 低 | 更好 | 上采样质量增益明确，列为 S3 增强 |

车载 UI 的背景模糊对象是面板背后的内容，不是精确高斯复刻——Kawase 的
"略偏轻、无伪影"特性恰好匹配。

---

## 3. 现状盘点（代码核实）

### 3.1 已具备、可直接复用的基础设施

| 能力 | 位置 | 对本方案的意义 |
|---|---|---|
| 离屏渲染目标 | `src/renderer/resource/OffscreenRenderTarget.h`（FBO + 颜色纹理，create/resize/destroy） | backdrop RT 与模糊链 ping-pong 直接用它 |
| RT 绑定命令 | `RenderDeviceProxy::bindRenderTarget / unbindRenderTarget`（`src/renderer/device/RenderDeviceProxy.h:122-128`） | 帧内切换绘制目标的原语已存在 |
| 离屏绘制先例 | `MR3DSceneView.cpp:475-505`：bind → 渲染 3D → unbind → 用颜色纹理画显示面片 | 分段渲染 + 回屏合成的完整参照 |
| SSBO 实例化合批 | `src/renderer/resource/ssbo/SSBOLayoutBuilder.h:30-50`；合批键 = displayLayer + clipRect + 材质/网格哈希 + 相邻性（`src/core/BatchBuilder.cpp:6-8`） | 所有模糊面片共用同一纹理与材质 → 天然合 1 批 |
| 每实例圆角 | `instance_accessors.glsl` 的 `instanceRounding()` + `rounded_clip.glsl` | 模糊面片的圆角遮罩零成本获得 |
| 按需渲染门控 | `src/core/Engine.cpp:188-196`（`consumeRenderRequest`，191 行） | 静止时整帧跳过，模糊链天然不执行 |
| 增量合批 | `BatchManager::isRenderableListUnchanged`（`src/core/BatchManager.cpp:132-140`） | backdrop 脏标记可复用同一思想 |
| Shader 内嵌 | `cmake/EmbedShaders.cmake`（`assets/shaders/**` 全部编译期内嵌） | 新增 kawase shader 只需加文件 |
| Widget 层级 | `Widget::setDisplayLayer`（`src/ui/base/Widget.cpp:117-119`，clamp -10..10） | 分段边界的天然表达 |

### 3.2 关键缺口

1. **帧内无分段绘制机制**：当前 `BatchManager::renderBatches`
   （`src/core/BatchManager.cpp:62-97`）一次性把 underlay 批次 + 普通批次全部
   画到当前绑定的目标上，没有"画一半 → 切 RT → 处理 → 回来画另一半"的钩子。
2. **无后处理 pass 基础设施**：没有 pass 图 / 后处理材质队列，模糊链需要
   自建一个最小的 `PostProcessPass` 概念。
3. **SSBO 实例布局没有 backdrop UV 字段**：`extraAttr` 已被标注为自定义动画
   参数使用（如 `bounce.vert:16`），模糊面片需要"自身矩形 → backdrop 采样 UV"
   的每实例数据（4 个 float），需要扩展实例布局（涉及
   `SSBOLayoutBuilder.h` 的 `static_assert` 偏移约束，须与
   `SSBO_LAYOUT_AUTOMATION.md` 的机制协同）。

---

## 4. 全屏多 Widget 的核心矛盾与方案对比

**矛盾**：模糊的输入是"该 Widget 身后已绘制的内容"。Widget 数量多、分布在
各处、半径不同时，朴素实现（每个 Widget 各自拷贝身后区域再模糊）成本 =
`O(N × 区域面积 × pass 数)`，全屏下完全不可行。

| 方案 | 描述 | 成本 | 背景保真度 | 结论 |
|---|---|---|---|---|
| A. 共享模糊链 | 整个 backdrop 层模糊一次，全部 Widget 采样同一条链的不同层级 | 1 条链，与 Widget 数量无关 | 所有 Widget 共享同一模糊源（各自区域正确，重叠遮挡语义简化） | **基线，推荐** |
| B. 脏区域 + 缓存 | backdrop 未变则跳过整链；只重模糊变化区域 | 静止为 0；动画期为链成本 × 脏比例 | 同 A | **叠加在 A 上的优化** |
| C. 按绘制序分组 backdrop | 每个遮挡组各自拷贝 backdrop + 各自一条链 | O(组数 × 链) | 完全正确（Widget 甲可以出现在乙的模糊背景里） | 复杂度高，除非需求明确否则不做 |

**推荐：A + B**。方案 A 的语义取舍是：**模糊源只包含"模糊边界层以下"的
内容**，模糊 Widget 之间的互相遮挡不会进入对方的模糊背景。这与 iOS/安卓
毛玻璃的实际观感一致（面板背景是"下层世界"的模糊），且换来合批与成本优势。
若未来确需方案 C 的保真度，可在 A 的分段机制上把"单个分段点"推广为
"多个分段点"，基础设施是同一套。

---

## 5. 推荐架构详细设计

### 5.1 帧结构

在现有管线中插入 backdrop 段与模糊链（伪代码，位置对应
`Engine::render` 的阶段 3 与 `BatchManager::renderBatches`）：

```text
updateWidgets / lateUpdateWidgets          // 不变：收集 RenderItem（含模糊面片标记）

── 阶段 3a：backdrop 段 ──────────────────────────────────────────
bindRenderTarget(RT_backdrop)               // 全屏或 1/2 分辨率
  绘制 underlay 批次（阴影等，语义不变）
  绘制 displayLayer < BLUR_BOUNDARY 的普通批次
unbindRenderTarget()

── 阶段 3b：模糊链（ping-pong）──────────────────────────────────
downsample + kawase × K → RT_L0' / L1' / L2'（分辨率 1/2、1/4、1/8）
（backdrop 脏标记为 clean 时，本阶段整段跳过）

── 阶段 3c：回屏合成 ────────────────────────────────────────────
全屏面片拷贝 RT_backdrop → 默认帧缓冲（恢复背景，等价于背景直接画屏）
绘制 displayLayer ≥ BLUR_BOUNDARY 的普通批次，其中：
  模糊面片 = backdrop 采样材质，UV = 自身屏幕矩形映射到 RT 的 UV，
             采样层级由每实例半径决定，圆角走 instanceRounding()
  面片属主（面板内容）在其后绘制（同层内收集顺序保证）
```

要点：

- **背景恢复用拷贝而不是画两遍**：阶段 3a 的内容只画一次（进 RT），回屏
  用一次全屏面片拷贝（GLES 上避免依赖 `glBlitFramebuffer` 的格式限制）。
- **模糊面片与属主的次序**：沿用 `Shadow` 组件的成熟模式——组件复用属主
  的 `MeshFilter` 与 `Transform`，仅材质不同；组件先于属主注册渲染项，同层
  内收集顺序即绘制顺序，模糊面片自然垫在面板内容之下（与
  `UI_SHADOW_DESIGN_PROPOSAL.md` S1 的"普通通道 + 自然顺序"结论一致）。
- **执行线程**：所有 bind/unbind/pass 绘制均以命令形式走 `RENDERINGTHREAD`
  编码，与现有批次一致，主线程只做收集与链参数计算。

### 5.2 半径分级与采样层级

链上保留 3 个稳定层级（1/2、1/4、1/8 分辨率 + 各一次 kawase），半径映射：

| Widget 请求半径 | 采样层级 | 等效模糊观感 |
|---|---|---|
| 0（关闭） | 不采样链 | tint-only 半透明面板（见 §5.7） |
| 1 ~ 12 px | L0'（1/2 链） | 轻 |
| 12 ~ 40 px | L1'（1/4 链） | 中（面板常用） |
| 40 px 以上 | L2'（1/8 链） | 重 |

半径在层级内不再连续可调（分级量化）。这是刻意取舍：连续半径需要每半径
一条链或运行时改 pass 数，成本失控；三级 + tint 混色已覆盖 UI 需求。
半径动画（如面板展开时模糊渐显）用"层级固定 + alpha/tint 过渡"表达，
不做跨层级插值（S3 再评估 tent 上采样 + 双层混合）。

### 5.3 合批保持

模糊面片材质三要素全组一致：同一 shader（`backdrop.frag`）+ 同一纹理
（层级如何不同？见下）+ 同层。**采样层级不同的面片需要不同的
`sampler2D` 绑定，会断批**。两个选择：

- **推荐：每层级一个批次，共 ≤ 3 批**。批内依旧 SSBO 实例化（同层级几十个
  面片 1 draw call）。3 个 draw call 的成本可忽略，实现最简单（材质按层级
  分三份，实例归入对应批次）。
- 备选：三级纹理打包进 2D 纹理数组或手动三级 mip，单材质内用每实例
  `stateAttr.y` 选层。省 2 个 draw call，但引入纹理数组路径与 mip 约束，
  收益太小，不建议首期做。

每实例数据扩展（`UIInstanceData` 新增或占用空闲字段）：

```glsl
// backdrop.frag 内的每实例语义（示意）
vec4 uvRect    = instanceBackdropUV();  // xy = RT 采样 UV 原点, zw = 尺寸
float tintMix  = ...;                    // acrylic 混色强度（可并入 color1）
```

`SSBOLayoutBuilder.h` 的偏移 `static_assert` 需同步更新，非 SSBO 回退路径
（`u_extraAttr` uniform）同步补齐——这条改动要对照 `SSBO_LAYOUT_AUTOMATION.md`
的流程走，避免破坏布局自动化测试。

### 5.4 组件 API（沿用 Shadow 模式）

```cpp
auto panel = MRImage::create();
auto backdrop = panel->addComponent<BackdropBlur>();   // 组件持有 backdrop 材质
backdrop->setBlurRadius(24.0f);                        // 量化到 L0/L1/L2
backdrop->setTintColor(0.96f, 0.97f, 1.0f, 0.55f);     // acrylic 混色
backdrop->setRoundingFollowOwner(true);                // 默认跟随属主圆角
```

- 组件 `update` 中注册渲染项（普通通道、先于属主），并把自己的屏幕矩形
  写入实例的 backdropUV；
- 无组件的 Widget 完全不感知本特性，零开销；
- `BLUR_BOUNDARY` 由首个 `BackdropBlur` 组件的 `displayLayer` 决定
  （初始化时取 min），帧结构对无模糊场景完全退化为现有单段路径。

### 5.5 缓存与失效（方案 B）

| 状态 | 行为 |
|---|---|
| 静止（request-render 未请求） | 整帧不渲染，模糊链不执行（已有机制，零改动） |
| 动画只发生在边界层以上 | backdrop 段批次序列与上一帧一致 → 脏标记 clean → 跳过阶段 3a/3b，仅重绘 3c |
| backdrop 层内容变化 | 重跑 3a + 3b |

脏标记实现：backdrop 段的 `isRenderableListUnchanged` 已能覆盖"列表结构
不变"（纯位移动画仍会误判为 unchanged——列表等价比较含位置吗？需核实
`areRenderItemListsEquivalent` 的比较深度；若不含位置，需叠加 Transform
revision 聚合校验，S2 处理）。

### 5.6 新增 shader 清单

| 文件 | 用途 |
|---|---|
| `assets/shaders/backdrop_downsample.frag/.vert` | 4-tap box 降采样（复用全屏面片顶点） |
| `assets/shaders/backdrop_kawase.frag/.vert` | Kawase pass（uniform：offset、texel） |
| `assets/shaders/backdrop_composite.frag/.vert` | 回屏背景恢复拷贝 |
| `assets/shaders/backdrop.frag/.vert` | 模糊面片：采样层级纹理 + uvRect + 圆角 + tint |

全部走 `EmbedShaders.cmake` 编译期内嵌，运行期无 IO。

### 5.7 毛玻璃与普通 UI 混存、降级与开关

#### 混存语义

毛玻璃是**按 Widget 粒度的可选能力**，不是全局渲染模式：

- 普通 Widget 不挂 `BackdropBlur` 组件即零感知、零成本，走完全现有的
  收集与合批路径；
- 帧内混存由分段边界表达：**想在玻璃背景里被模糊的普通 UI 放边界以下，
  想压在玻璃上保持清晰的放边界以上**，`displayLayer`（-10..10）即排布
  工具；
- 成本隔离：只有"边界以下内容 + 一次回屏拷贝"为模糊买单（§7），边界以上
  的普通 UI 不付任何代价；全帧无模糊组件时帧结构整体退化为现有单段
  直绘路径。

局限（§4 方案 A 的语义取舍）："普通 UI 夹在两块玻璃之间、只被下面那块
模糊"的层序单分段点表达不了，出现该需求时再评估方案 C（多分段点）。

#### 三级开关

| 层级 | API / 配置 | 关闭后的形态 |
|---|---|---|
| 全局运行时 | `BackdropBlurManager::setEnabled(false)` | 毛玻璃降级为 tint 半透明面板，帧结构走退化路径 |
| 启动档位 | `EngineOptions` 新增 `backdropBlur` 档位：Off / Standard / LowCost（LowCost = 1/4 基准链），沿用 `src/core/Engine.h:19` 现有配置项模式 | Off 档全程退化路径，RT 链不分配 |
| 单 Widget | `setBlurRadius(0)` | 该面板 tint-only，不参与层级采样批次 |

关键设计：**关闭 ≠ 删组件**。组件仍注册自己的面片，但材质从 backdrop
采样切换为纯 tint 纯色（`setTintColor` 的色值直接作面板底色）。效果等同
iOS"降低透明度"——视觉层级与可读性保留，成本归零（无 RT、无链、无
回屏拷贝）。适配三类真实需求：用户在系统设置关闭动效、低配车型档位、
运行时性能降级策略（如动态帧率保护）。

#### 运行中切换

- 切换仅翻转标志位，下一帧自动走对应路径，**不重建 Widget 树**；
- RT 链（约 7MB，§7）首次开启时懒分配；关闭后可短暂保留以便快速重开，
  或立即走现有 fence 异步销毁通道（`OES_TEXTURE_FENCE_DESIGN.md`）；
- 批次形态变化：开启时模糊面片按层级 ≤ 3 批（§5.3）；关闭后全部面片
  共用 tint 材质，合并为 1 批。

---

## 6. 分阶段实施计划

### S1：最小可用（单层级、全屏链、无缓存）✅ 已实施（2026-09-25）

- `BackdropBlurManager`：持有 `RT_backdrop`（1/2 分辨率）+ ping-pong 对、
  pass 材质与全屏面片网格；实现"分段渲染 + 链 + 回屏合成"完整帧结构；
- `BatchManager::renderBatches` 拆分为分段执行（新增 `displayLayer` 分段点
  参数，保持无模糊场景的现有路径与批次统计不变）；
- `BackdropBlur` 组件 + `backdrop.frag`（单层级 L0'，固定半径档）；
- 全局开关 `setEnabled` 与 tint 降级材质路径（关闭 = 面片切纯色材质、
  帧结构走退化路径，见 §5.7）；
- 实例布局扩展：backdropUV 字段（含非 SSBO uniform 回退路径）——
  **实现为 CPU 烘焙每顶点数据，未扩展 UIInstanceData**（见文首偏差说明）；
- 验收：新增 `BackblurDemo`（全屏卡片墙 + 若干模糊面板叠加，含普通 UI
  分置边界上/下的用例与运行中开/关切换的用例），截图确认模糊正确、
  圆角正确、模糊面片合批（`BatchStatistics` 面板确认模糊面片 1 批）；
  关闭后 GPU 帧时间与无模糊基线一致；DebugDemo / ImageDemo 回归无变化。
  —— DebugDemo 回归通过（14/5/5 基线一致）；ImageDemo 在桌面 GL
  多线程模式下存在**与本案无关的存量段错误**（字体纹理上传命令超出
  CommandBuffer 容量，基线同样复现，另行处理）。

### S2：分级 + 缓存 + 分辨率策略 ✅ 已实施（2026-09-25）

- 三层级链（1/2、1/4、1/8）与半径映射表；每层级材质分批（≤ 3 批）——
  实现为每层级一个 CPU 合并 VBO、1 draw（BackblurDemo 4 面片 → 3 draw），
  链按本帧最大所需层级裁剪（无 L2 面片时低层级整段跳过）；
- `EngineOptions` 启动档位（Off / Standard / LowCost，LowCost = 1/4 基准链）
  与运行中切换的 RT 生命周期（懒分配 / 短暂保留 / 按序命令释放）；
- backdrop 脏标记（列表等价 + 版本号聚合），clean 跳过 3a/3b——签名覆盖
  帧参数 / 批次结构 / Transform 世界版本 / Material uniform 修订 / Mesh
  几何修订；为此给 Material 数值 setter 补了值相等检查（与 Transform 既有
  语义对齐：无变化不递增修订号，否则标签类控件每帧的重复 set 会让缓存
  永远失效）；
- 窗口 resize 时 RT 链重建（复用 `OffscreenRenderTarget::resize`，尺寸变化
  经签名自动触发全量重渲）；
- tint/acrylic 混色、边缘 clamp 采样——混色修正为不透明替换语义
  （`结果 = mix(模糊背景, tint.rgb, tint.a)`，tint.a 为混色强度；S1 版在
  shader 内 mix 后又走 alpha 混合导致 tint.a 被平方），边缘由 RT 纹理
  CLAMP_TO_EDGE 兜底；
- 验收：静止场景（--static）backdrop 段与链整段跳过（4q/4d，连带 backdrop
  批次不画、batchDrawCalls 显著下降），帧间隔与无模糊基线一致（桌面 GL
  观测 20.2 vs 20.1ms，受 vsync/FPS 控制器主导）；动画期链满载 4q/10d；
  桌面实测 avgFrameMs 已进 demo JSON，< 3ms GPU 预算仍须目标 SoC 实测。

### S3：质量与高级特性（按需）

前三项已实施（2026-09-25）；方案 C 维持"出现明确需求后再评估"。

- ✅ 上采样升级为 9-tap tent——实现在 `backdrop.frag` 的面片采样端（非独立
  上行链 pass）：十字 + 对角 9 tap 加权，仅模糊面片覆盖区域付出成本；
- ✅ 半径动画（层级固定 + 双层混合插值）——`BackdropBlur::setBlurLevel(0..2)`
  连续层级：整数部分取该层级纹理，小数部分在相邻层级间插值（混合因子走
  Tangent.w 顶点属性，动画面片仍按层级对合并为 1 draw）；层级固定、链与
  缓存不受动画影响；tint/alpha 过渡仍可用 `setTintColor` + Tween 表达；
- ✅ 滚动容器 / clipRect 裁剪正确性专项——面片提交时携带
  `frameState->currentClip`，按（层级对 × clipRect）分组施加 scissor
  （与 `BatchManager::applyClipRect` 同款 GL Y 翻换算）；BackblurDemo
  900px 容器内 1200px 面板用例验证：溢出部分被精确剪掉，横向缓动下
  裁剪边缘稳定；
- 方案 C（多分段点分组 backdrop）仅在出现明确需求后评估。

---

## 7. 性能预算估算（1920×1080，RGBA8）

带宽粗算（读 + 写计双倍流量；数字为量级参考，须实测修正）：

| 环节 | 分辨率 | 单次流量（R+W） |
|---|---|---|
| backdrop 段渲染 | 1/2 屏（960×540） | ≈ 2×2.1MB ≈ 4.2MB（本身也是绘制，省了全屏直绘的一半） |
| downsample → L0 | 1/2 → 1/2 | ≈ 4.2MB |
| kawase L0（ping-pong） | 1/2 | ≈ 4.2MB |
| downsample → L1 + kawase | 1/4 | ≈ 1.0MB |
| downsample → L2 + kawase | 1/8 | ≈ 0.3MB |
| 回屏背景恢复拷贝 | 1/2 → 全屏 | ≈ 10.4MB |
| **链合计（不含 backdrop 绘制本身）** | | **≈ 20~24MB / 帧** |

按车载 SoC 共享带宽 10~25GB/s 估算，GPU 时间约 **1~2.5ms**，满足 < 3ms
预算；backdrop 段用 1/2 分辨率渲染的质量损失被模糊本身掩盖（输出本来就是
模糊的），是关键的成本杠杆。2K 屏按面积比例 ×1.8。

内存常驻：RT_backdrop(1/2) + ping-pong×4（1/2、1/4、1/8 各一对，层级产物
即链中间结果）≈ 2.1×3 + 0.5 + 0.1 ≈ **7MB**，可接受。

---

## 8. 风险与开放问题

1. **绘制顺序语义**：分段边界把"层序"提升为帧结构概念。当前批次顺序 =
   收集顺序（`BatchBuilder` 不排序），同层内 Widget 的先后依赖更新顺序，
   分段后"边界以下全部先于边界以上"是有意为之的语义收紧——需在文档与
   API 注释中明确，避免业务依赖被静默改变。
2. **underlay（阴影）归属**：阴影属主在边界以上时，其 underlay 面片仍画在
   backdrop 段（underlay 全局先画），模糊背景里会出现"还没画的属主"的
   阴影。与 Shadow 方案的 underlay 语义联动，S1 验收用例须覆盖，必要时
   把 underlay 拆分为"backdrop 段 underlay / 前景段 underlay"。
3. **透明像素混合**：backdrop 段含半透明 UI 时，RT 的 alpha 通道语义
   （premultiplied 与否）要与背景恢复拷贝、模糊面片混色一致，防止发灰。
4. **实例布局变更**：`UIInstanceData` 加字段触发布局偏移约束
   （`SSBOLayoutBuilder.h:50`）与所有宿主 shader 的 `instance.glsl` 同步，
   走 `SSBO_LAYOUT_AUTOMATION.md` 的校验流程。
5. **`areRenderItemListsEquivalent` 比较深度**：若不含 Transform 位置变化，
   S2 的脏标记需补位置校验，否则纯位移动画下 backdrop 会用到过期内容。
6. **QNX tile-based GPU 的 resolve 行为**：频繁 bind/unbind FBO 会触发
   tile resolve；本方案每帧固定 1 次切换 + 链上若干次（均为全屏 pass，
   resolve 成本可预测），不构成结构性风险，但须在目标板实测确认。

---

## 9. 与现有文档的关系

- `PERFORMANCE_OPTIMIZATION_ROADMAP.md` 第 3 节"已经做对的部分"清单
  （按需渲染、增量合批、SSBO 实例化、版本号缓存）是本方案的前提，设计
  全程不破坏这些机制；
- `UI_SHADOW_DESIGN_PROPOSAL.md` 确立的"组件复用属主 mesh/transform + 普通
  通道自然顺序"模式被 `BackdropBlur` 组件直接沿用；其 S2/S3（重叠合并、
  SSBO 变体）与本方案的层级材质分批思路可互相借鉴；
- `SSBO_LAYOUT_AUTOMATION.md` 约束实例布局扩展的实施方式；
- `OES_TEXTURE_FENCE_DESIGN.md` 的 fence 回收机制覆盖模糊链 RT 的生命周期
  管理（异步销毁复用现有通道）。
