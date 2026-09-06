# UI 阴影渲染演进方案（层序跟随属主 + SDF 软阴影）

> **背景**：ImageDemo 重构时的实测暴露了当前阴影方案的两个结构性问题——
> ① 层序：阴影走 underlay 全局垫底，"阴影投在卡片/面板上"无法表达（被后画的
> 不透明面板整个覆盖）；② 视觉：`shadow.frag` 只做矩形边缘渐变，不支持圆角、
> 模糊半径与扩展，与圆角化的 UI 风格不匹配。本文给出层序与视觉两条演进的
> 设计方案与实施计划。

> **实施状态（2026-09-06）**
>
> - **S1 已落地**：SDF 圆角软阴影（`shadow.vert/frag` 重写：圆角矩形 SDF +
>   高斯衰减 + 几何外扩 + **outer-only 裁剪**）；`Shadow` 组件新增
>   `setShadowBlur` / `setShadowSpread` / `setShadowRounding`（圆角默认跟随
>   属主材质）并改走**普通通道**（属主后自然顺序，不再使用 underlay）。
>   默认阴影 = 零偏移黑色柔光。实测截图验证：阴影投在卡片上可见、不污染属主、
>   圆角跟随正确；DebugDemo 验收基准更新为 14/5/1/4/5。
> - S2（重叠感知组合并）、S3（shadow SSBO 变体）、备选 Behind 模式：未实施。

## 1. 现状机制

### 1.1 渲染链路

```text
Widget::update
  ├─ MeshRenderer::update → BatchManager::addRenderable(owner, underlay=false)
  └─ Shadow::update       → BatchManager::addRenderable(shadowMaterial, owner的mesh/transform, underlay=true)
                               （同一 mesh、同一 Transform，仅材质不同）

BatchManager::renderBatches
  ├─ buildFromItems(m_underlayRenderables)   // 先构建
  ├─ buildFromItems(m_renderables)           // 后构建 → 批次顺序 = 列表顺序
  └─ 顺序绘制所有批次                          // underlay 整体先于一切普通内容
```

- `Shadow` 组件持有独立材质（`shadow` shader），复用属主的 `MeshFilter` 与
  `Transform`，几何偏移在 shadow.vert 内做（`u_shadowOffset`，对象空间平移）；
- `BatchBuilder::build()` **不做排序**：按收集顺序对相邻项分组，
  `canBatch = displayLayer 相同 && clipRect 相同 && batchKey 相同 && 位置相邻`；
- 阴影材质无纹理、同 shader → 所有阴影的 batchKey 相同，underlay 先画使它们
  天然相邻 → **全部阴影合为 1 个批次**（这是当前方案的合批优点）。

### 1.2 视觉公式（shadow.frag）

```glsl
// 半径渐变：距矩形边缘 distance 处，从 0 渐变到 1，渐变宽度 = shadowOffsetFade
float shadowAlpha = mix(smoothstep(0, fadeY, distY), smoothstep(0, fadeX, distX), isXEdge);
fragColor = u_shadowColor;  fragColor.a *= u_alpha * shadowAlpha;
```

阴影四边形与属主同尺寸、整体平移 offset；可见部分只有属主外的 offset 像素条带，
渐变恰好铺满条带——是"硬边矩形 + 线性羽化"，不是软阴影。

### 1.3 实测问题（2026-09-06，ImageDemo / DebugDemo）

| 问题 | 实测证据 |
|---|---|
| 阴影被不透明面板覆盖 | ImageDemo 中阴影组件垫在 `MRColor` 卡片上，underlay 先画 → 卡片后画盖住阴影，完全不可见 |
| 偏移小时不可辨 | DebugDemo（无遮挡、白底）offset=5px，在 2x DPI 下仅剩 1~2 物理像素的浅灰渐变 |
| 矩形阴影与圆角 UI 不匹配 | 属主 `setRounding(20)` 后阴影仍是直角矩形；也无"模糊半径/扩展"概念 |
| float 相等判断 | `distance == distanceToXEdge` 依赖浮点相等选择混合权重，脆弱 |

---

## 2. 层序演进：阴影跟随属主绘制序

### 2.1 目标语义

阴影必须满足两条：

1. 画在**属主之前**（被属主覆盖）；
2. 画在**所有视觉上位于属主背后的内容之后**（卡片、面板、背景）。

即：阴影的正确位置 = 收集顺序中属主的前一个位置。underlay（全局垫底）与
"全局置顶"都无法表达，唯一正确语义是**相对属主的插入**。

### 2.2 目标语义的重新审视：outer-only 裁剪可以消解"属主前后"问题

阴影必须满足两条：

1. 不覆盖**属主**；
2. 画在所有视觉上位于属主背后的内容（卡片、面板、背景）之后、
   属主之前的内容之前——即相对**其他组件**保持属主的绘制序。

第 1 条其实可以由 shader 自己保证，而不依赖绘制顺序：SDF 阴影做
**outer-only 裁剪**——属主轮廓内部（`d < 0`，含 blur 内扩区）的片元
`alpha = 0`，阴影只渲染轮廓外的光晕环：

```glsl
// outer-only：d < 0 的区域（属主覆盖区）完全透明
float shadowAlpha = u_shadowColor.a * u_alpha * falloff(d) * smoothstep(-1.0, 0.0, d);
```

此时"先画组件、再画阴影"完全正确——而这恰好是引擎的**自然收集顺序**
（`MeshRenderer::update` 先于 `Shadow::update`），**无需任何排序机制**。
阴影紧随属主绘制，天然满足第 2 条（属主之后绘制的兄弟组件会盖住光晕，
属主之前绘制的内容被光晕覆盖）。

两个约束与代价：

- 阴影仍不能放进"全局最后"的后置 pass：属主之后绘制的组件在视觉上位于
  属主之前，其内容必须盖住阴影光晕，全局后置会违反这一点；
- **透明像素语义变化**：underlay 下阴影会透过属主的透明像素（如图片四周
  透明区）显示；outer-only 下被轮廓裁剪，轮廓内不出现阴影。对纯色圆角矩形
  组件（MRColor/MRButton 等）无差别；
- 合批交错问题依旧存在（见 2.4），P2 组合并仍然需要。

### 2.3 方案 A′（推荐）：outer-only SDF + 属主后自然顺序

即 §3 的 SDF 软阴影 shader 采用 outer-only 裁剪，层序保持自然收集顺序
（Shadow 组件去掉 `underlay=true`，其余零改动）。特点：

- **零排序开销**：不需要 subOrder 字段、不需要稳定排序、不触碰
  BatchBuilder 的分组逻辑；
- "阴影投在卡片/面板上"直接可用——阴影光晕画在卡片之上（卡片先画）、
  画在属主之下（outer-only 裁剪保证不碰属主）；
- 与 S1（SDF shader）是同一个改动，**S1 完成即层序问题消解**；
- 依赖 S1 的 SDF 圆角轮廓与属主形状一致（rounding 跟随属主），否则轮廓
  内外的分界会穿帮。

若个别场景需要"阴影透过透明内容"的旧语义，可为 Shadow 组件保留
`shadowClipMode = OuterOnly / Behind`（Behind 模式走 2.4 的 subOrder 插入），
默认 OuterOnly。

### 2.4 方案 A（备选）：RenderItem 子序号（subOrder）

若不采用 outer-only（例如需要保留阴影透过透明像素的语义），则退回
显式插入方案——给 `RenderItem` 增加一个子序号字段，构建批次前做一次稳定排序：

```cpp
struct RenderItem {
    // ... 现有字段
    int8_t subOrder = 0;   // 同 insertionIndex 内的绘制次序；阴影 = -1
};

// buildFromItems 内，BatchBuilder::build 之前：
std::stable_sort(items.begin(), items.end(), [](const RenderItem& a, const RenderItem& b) {
    if (a.displayLayer != b.displayLayer) return a.displayLayer < b.displayLayer;
    if (a.insertionIndex != b.insertionIndex) return a.insertionIndex < b.insertionIndex;
    return a.subOrder < b.subOrder;
});
```

- `Shadow::update` 调用 `addRenderable(..., underlay=false)`，并携带
  `subOrder = -1` 与**属主的 insertionIndex**（`addRenderable` 增加一个可选参数，
  由 Shadow 传入属主索引）；
- 排序后阴影紧贴属主之前：`卡片 → 阴影 → 属主 → ...`，语义精确；
- `isSameRenderableAs` / 增量比对逻辑同步加入 subOrder（不变量：每帧插入
  位置相同，等价性判断不受影响）；
- underlay 通道保留（shadow 之外还有真正需要全局垫底的内容），Shadow 组件
  不再使用它。

**合批代价（必须正视）**：排序后阴影插入属主之间，把原本相邻的同材质项隔断：

```text
现状（underlay）：  [s1 s2 ... s10] [i1 i2 ... i10]           → 2 个批次
方案 A（交错）：    s1 i1 s2 i2 ... s10 i10                  → 20 个批次
```

10 张图 + 阴影的极端场景从 2 次 draw call 恶化到 20 次。缓解与治理见 2.6。
方案 A′ 同样交错（属主后插入），合批代价与 A 相同。

### 2.5 被否决的替代

| 方案 | 否决理由 |
|---|---|
| shadow band（按 displayLayer 分带，阴影画在带首） | 带首仍早于同层的卡片，"投在卡片上"依旧不成立 |
| 阴影随属主合成一个 RenderBatch 双 pass | 需要扩展 RenderBatch 携带第二材质与两套实例数据，SSBO 统一布局下复杂度高；作为远期选项保留（见 2.7） |
| 全局后置 pass（所有阴影画在最后） | 属主之后绘制的兄弟组件（视觉在属主前面）会被光晕覆盖，层序错误 |
| 阴影 mesh 挖洞（inverted frame geometry，先属主后阴影） | 需按属主形状生成环形网格，文字/图集等多 mesh 属主无法适用；交错合批代价与 A′ 相同，收益不覆盖成本 |
| 约束用户用 displayLayer 分层（卡片 -1） | 把引擎设计缺陷转嫁给用户，且层数一多层号耗尽 |

### 2.6 合批治理：重叠感知的组合并（P2）

交错带来的批次碎片可以靠"不重叠即可换序"恢复。观察：

```text
s1 i1 s2 i2 ... （阴影与不相邻的属主互不重叠）
→ 安全重排为 [s1 s2 ... s10] [i1 i2 ... i10]    → 2 个批次
```

规则：构建批次后，若相邻两个同 batchKey 组之间隔着的其他组，其成员的
**屏幕空间 AABB 与被移动组的 AABB 并集互不相交**，则可把同 key 组前移合并。
绘制语义不变（2D 画家算法下，不重叠的元素交换顺序不影响输出）。

- AABB 由 `Transform::getWorldMatrix()` 与 mesh 尺寸计算，构建期（cache miss）
  一次性计算，O(n²) 最坏但 n 为数百级、仅在批次重建时执行；
- 结果缓存进现有的增量比对机制（列表等价 → 不重建）；
- `BatchStatistics` 增加 `mergedGroups` 计数便于观测。

### 2.7 SSBO 阴影（P3，与统一实例布局衔接）

阴影材质目前是非 SSBO shader，N 个阴影 = N 次标准绘制。统一 UIInstanceData
落地后，可以为 `shadow` shader 增加 `ENABLE_SSBO` 变体，槽位映射：

```text
color0    = shadowColor
geomAttr  = displaySize / rounding / alpha
stateAttr = (offsetX, offsetY, blur, spread)
extraAttr = 预留
```

配合 2.4 的组合并，阴影与普通内容一样进入 SSBO 实例化路径。

### 2.8 实施顺序（层序部分）

| 阶段 | 内容 | 验收 |
|---|---|---|
| P1 | SDF shader 采用 outer-only 裁剪（见 2.3 方案 A′）+ Shadow 改普通通道（属主后自然顺序，去掉 `underlay=true`） | ImageDemo 阴影投在卡片上可见且不污染属主；DebugDemo 验收基准更新（阴影批次位置变化） |
| P2 | 重叠感知的组合并 + 统计计数 | 图片墙 + 阴影场景 draw call 恢复到接近合并前水平 |
| P3（可选） | shadow shader 的 SSBO 变体 | 统计中阴影进入 ssboBatches |
| 备选 | 若需要"阴影透过透明像素"语义，追加 subOrder 插入通道（2.4 方案 A）作为 Behind 模式 | 切换语义后截图对比 |

---

## 3. 视觉演进：SDF 圆角软阴影

### 3.1 方案对比

| 方案 | 思路 | 结论 |
|---|---|---|
| **SDF 解析软阴影（推荐）** | 圆角矩形 SDF + 高斯衰减，单 quad、全参数化 | 零纹理内存、参数实时可调、与圆角 UI 完全匹配；片元开销可接受 |
| 九宫格预模糊纹理 | 离线生成一张模糊圆角矩形，9-slice 拉伸 | 片元最便宜；但模糊半径/颜色固定、过渡生硬、需管理纹理资源 |
| FBO 高斯模糊 | 每个阴影实时模糊 | 带宽与内存开销大，车载 GPU 不划算，否决 |
| 多次叠加 quad 模拟 | N 层半透明 quad 递减 alpha | N 倍顶点与填充率，效果仍不如 SDF，否决 |

### 3.2 SDF 公式

圆角矩形 SDF（与 `image_normal` 的圆角裁剪同族数学，风格统一）：

```glsl
float sdRoundedBox(vec2 position, vec2 halfSize, float rounding) {
    vec2 q = abs(position) - halfSize + rounding;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - rounding;
}
```

片元阶段（`v_position` 为对象空间坐标，几何体已按"扩展 + 偏移"放大平移）：

```glsl
float d = sdRoundedBox(v_position, halfSize + spread, rounding);
// 高斯衰减：d <= 0 全强度，d > 0 按 blur 半径衰减
float t = max(d, 0.0) / max(blur, 0.001);
float falloff = exp(-t * t * 3.0);            // 或多项式近似避免 exp
// outer-only：属主轮廓内部（d < 0）完全透明，
// 阴影不污染属主 → 先画属主后画阴影即正确（见 2.3 方案 A′）
float outerOnly = smoothstep(-1.0, 0.0, d);
float shadowAlpha = u_shadowColor.a * u_alpha * falloff * outerOnly;
fragColor = vec4(u_shadowColor.rgb, shadowAlpha);
```

**几何放大是关键细节**：模糊使阴影足迹超出属主矩形 `blur + spread` 像素，
vert 阶段需把 quad 放大 `2 * (blur + spread)` 并同步平移 offset，否则衰减区
被 quad 边缘截断（当前实现正是用"渐变宽度 = 偏移量"回避了这个问题）。

### 3.3 参数与语义

| 参数 | 默认 | 说明 |
|---|---|---|
| `shadowOffset` | (0, 0) | 现有，屏幕方向 (+x 右, +y 下) |
| `shadowBlur` | `max(offset.x, offset.y) * 2` | 模糊半径（px），决定衰减宽度 |
| `shadowSpread` | 0 | 阴影相对属主的扩展/收缩（负值收缩） |
| `shadowRounding` | 跟随属主 rounding | 阴影圆角；跟随属主可由 Shadow 组件在 update 时读取属主材质参数自动填充 |
| `shadowColor` / alpha | 黑 0.5 | 现有 |

`Shadow` 组件对外接口保持增量扩展（新增 setter，不改已有签名）。

### 3.4 开销评估

- 片元级 `length + exp`：阴影 quad 面积有限（属主 + blur 边缘），以 1080p
  满屏阴影的极端情况估算也在百万片元以内；车载 GPU 上 `exp` 可换成
  `1 - smoothstep` 或二次多项式近似（视觉差异微小），提供编译期开关；
- 顶点数不变（仍 4 顶点 quad + 属主 mesh）；
- 无纹理、无 FBO、无额外批次。

---

## 4. 实施计划

| 阶段 | 内容 | 依赖 |
|---|---|---|
| S1 | SDF 软阴影 shader（含几何放大 + **outer-only 裁剪**），Shadow 组件新增 blur/spread/rounding 参数并改走普通通道（属主后自然顺序） | 无，可立即做；完成后层序问题随之消解（2.3 方案 A′） |
| S2 | 重叠感知组合并 + 统计 | S1（交错顺序落地后才有合并需求） |
| S3（可选） | shadow SSBO 变体（统一 UIInstanceData 槽位） | S1/S2 + 统一实例布局已落地 |
| 备选 | subOrder 属主前插入（Behind 语义：阴影透过透明像素） | 仅当 outer-only 语义不满足需求时实施 |

S1 一步同时解决视觉与层序两个问题；S2 视实际 draw call 压力决定优先级。

## 5. 测试要点

1. **层序**：阴影组件分别放在窗口背景、卡片、面板上，截图对比阴影可见性；
   阴影光晕必须覆盖先绘制的内容、被后绘制的内容覆盖；
2. **outer-only 轮廓**：属主 rounding 0/20/44 时，阴影不得出现在属主轮廓内部
   （含透明像素区域，切换 Behind 模式对照）；blur 大值扫描时光晕不被 quad
   边缘截断；spread 正负值正确收缩/扩展；
3. **合批**：DebugDemo 验收基准更新；图片墙 + 阴影场景在 S2 前后统计
   draw call 变化（预期交错后退化、S2 合并后恢复）；
4. **性能**：满屏阴影极端用例的帧耗对比（SDF vs 旧渐变），确认片元开销可接受。

## 6. 风险

| 风险 | 缓解 |
|---|---|
| 交错顺序导致 draw call 恶化 | S2 重叠感知合并兜底；过渡期在文档标注"大量阴影组件慎用" |
| outer-only 对透明像素内容"不投影" | 提供 Behind 模式（subOrder 插入）作为语义开关；默认语义与 CSS box-shadow 外阴影一致 |
| shadow 轮廓与属主形状不一致穿帮 | rounding 默认跟随属主材质参数；非矩形属主（文字）按字形 quad 各自 SDF |
| SDF `exp` 在低端车载 GPU 的片元开销 | 提供多项式近似编译期开关 |
| 属主 rounding 自动跟随读取失败（材质参数缺失） | shadowRounding 缺省独立设置，跟随逻辑只作为便利层 |
