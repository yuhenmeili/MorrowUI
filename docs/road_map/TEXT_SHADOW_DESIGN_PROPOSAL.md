# 文字阴影方案（Text Shadow Design Proposal）

> **背景**：widget 级 SDF 软阴影（见 [UI_SHADOW_DESIGN_PROPOSAL.md](UI_SHADOW_DESIGN_PROPOSAL.md)）
> 落地后，阴影形状 = 圆角矩形。文字（`MRLabel` / `MRRichTextLabel` / `MRTextEdit` 内文）
> 的属主网格是**逐字形 quad**，不是单个圆角矩形——直接套用会得到"每个字形一个圆角
> 方块"的斑点阴影，观感不可接受。本文给出文字阴影的专门方案与实施路径。

> **实施状态（2026-09-06，最终方案）**
>
> - **方案 C（SDF 图集升级）已落地，SDF 生成采用 coverage + EDT**：
>   - `DynamicFont::GenerateGlyphToAtlas`：按"字形框 ± padding(5)"先用
>     `stbtt_MakeGlyphBitmapSubpixel` 光栅化 coverage 位图，再做
>     **Felzenszwalb 精确欧氏距离变换**编码为 SDF（onedge 180 / dist 36，
>     常量见 `DynamicFont.h`，与 `font.frag` / `font_shadow.frag` 内常量一致）；
>     字形放置语义（bearing/UV）与旧 coverage 路径兼容，消费方零改动；
>   - `FontTexture` 图集过滤 NEAREST → **LINEAR**（SDF 重建必需）；
>   - `font.frag` 改 SDF 采样：`d = (r - 180/255) * 255/36`，
>     `coverage = smoothstep(-0.5, 0.5, d)`（±0.5px 抗锯齿）；
>   - 新增 `font_shadow` 变体（vert 几何偏移 + frag SDF 高斯衰减/blur/spread）；
>     `MRLabel` 新增 `setTextShadowColor/Offset/Blur/Spread`，先阴影后文字的
>     自然顺序提交（零排序机制）。
> - **为什么不直接用 `stbtt_GetGlyphSDF`**：实测 stb_truetype v1.26 的 SDF
>   对部分字体产生碎片伪影（Arial 正常，**MorrowSansCN 与 SimHei 全部碎裂**，
>   而 coverage 路径对同样字体正常）。曾短暂切换 Arial 规避，但 Arial 无
>   CJK 字形、中文全部变 notdef 方框，项目必须中文支持——最终回归引擎侧
>   解决：coverage + EDT 对**任意字体**稳定（边缘定位误差 ≤ 0.5px，实测
>   中文渲染与阴影均正常）。默认字体维持 MorrowSansCN。
> - 顺带修复：`MaterialUtil::resolveIncludes` 输出串未清空导致同一 Material
>   二次 `setShader` 时源码拼接（属性重定义、编译失败）——入口处 `resolved.clear()`。

## 1. 文字渲染管线现状（决定方案的事实）

| 事实 | 出处 | 对方案的影响 |
|---|---|---|
| 字形光栅化 = stb_truetype `stbtt_MakeGlyphBitmap`，8-bit **coverage 位图**（非 SDF） | `DynamicFont.cpp:163` | 软阴影/描边不能直接解析求值，需要图集侧配合（方案 B/C） |
| coverage 打入**动态图集**（`DynamicFont`/`FontTexture`），运行时按需增量上传 | `DynamicFont.cpp:235` | 图集内容可扩展（模糊变体/SDF 化），但打包策略要重新设计 |
| `font.frag`：`coverage = source.r`，`fragColor = vec4(fontColor.rgb, coverage * fontColor.a * alpha)` | `font.frag` | 阴影变体可复用同一采样逻辑，只改输出与偏移 |
| 文字 = **逐字形 quad 网格**，存在属主的 `MeshFilter` 中（含图集 UV） | `MRLabel::createTextMesh` | 二次绘制阴影可复用同一网格与 UV |
| 文字属 SSBO 批渲染 family（model + fontColor + fontAttr 每实例） | `FontSSBOLayout` | 阴影 pass 可与统一实例布局衔接 |
| 仓库自带 `stb_truetype.h` 已含 **`stbtt_GetGlyphSDF`** | `stb_truetype.h:948` | SDF 图集升级**零新增依赖**（无需 msdf-atlas-gen 外部工具链） |

## 2. 目标与约束

1. HMI 核心诉求是**文字可读性**：浅色文字压深浅不一的背景（地图/视频/图片上叠加），
   需要柔和的暗色背景衬垫（阴影/光晕），而非装饰性投影；
2. 零偏移的"柔光（glow）"与带方向偏移的"投影（drop）"都应支持；
3. 文字量大（仪表/列表/告警），阴影 pass 必须保持批渲染、避免逐字符开销；
4. 车载多 DPI：文字在不同分辨率/缩放下保持清晰是长期诉求（与 SDF 化顺路）；
5. 不引入外部工具链（沿用引擎轻量化定位）。

## 3. 方案对比

| 方案 | 思路 | 观感 | 成本 | 结论 |
|---|---|---|---|---|
| **A. 二次绘制覆盖阴影（hard text-shadow）** | 新增 font_shadow 变体：同一 glyph 网格 + 同一图集，采样 coverage 乘阴影色，vert 加偏移 | 字形正确的**硬**阴影（CSS text-shadow 同款），无模糊 | 最小：一个 shader 变体 + 文字组件一次额外提交 | **P1 推荐**，覆盖可读性诉求的 80% |
| B. 预模糊图集变体（blur mip） | 图集为每个字形额外烘焙 1~2 级高斯模糊版本，阴影 pass 采样模糊级 | 字形正确的**软**阴影（固定档位） | 图集内存 ×(1+N)、打包/上传逻辑改动、运行时多一次 draw | 折中；不动采样数学，但图集管理复杂 |
| **C. SDF 图集升级** | 光栅化改用 `stbtt_GetGlyphSDF`（stb 自带），font.frag 改 SDF 采样；阴影 = 偏移 UV 采样 + 距离外扩 | 字形正确的**任意 blur/spread 软阴影** + **描边（outline）**；附带：文字任意缩放清晰 | 中：光栅化/图集/采样全链路改造 + 观感回归；图集内存增大（padding） | **P2 推荐**，需要软阴影或描边时实施；多 DPI 清晰度是长期收益 |
| D. FBO 离屏模糊 | 文字层渲到纹理再高斯模糊 | 最好 | 动态文字每帧模糊成本高，静态缓存复杂 | 否决 |
| E. 通用 Shadow 组件直接套用 | 属主网格复用 + 圆角矩形 SDF | 每字形一个圆角方块斑点 | — | 否决（本文动机） |

## 4. 方案 A 详设：二次绘制覆盖阴影（P1）

### 4.1 shader 变体

新增 `font_shadow`（独立 frag，或 font.frag 的 `MORROW_TEXT_SHADOW` define 变体，
复用 §统一实例布局/include 机制）：

```glsl
// vert：与 font.vert 相同的 glyph quad，叠加偏移
vec2 offsetPosition = a_position.xy * scale + u_textShadowOffset.xy;

// frag：同一图集、同一 UV，输出阴影色
float coverage = texture(u_atlas, v_texCoord).r;
fragColor = vec4(u_shadowColor.rgb, coverage * u_shadowColor.a * u_alpha);
```

- 硬阴影无 blur，coverage 直接作为形状；
- `u_textShadowOffset` / `u_shadowColor` / `u_alpha` 走材质 uniform（与 widget
  Shadow 的参数命名保持一致）。

### 4.2 绘制顺序：先阴影后文字（自然顺序，零排序机制）

与 widget 阴影的方案 A′ 同理但方向相反——文字阴影必须在**自己的文字之前**绘制
（glyph 内部 coverage=1，阴影若后画会用阴影色盖住文字本体）：

```text
收集顺序：... 卡片/背景 → [文字组件的阴影项] → [文字项] → 后续组件 ...
```

文字组件在自己的更新中**先提交阴影渲染项、再提交文字渲染项**即可：阴影画在
先绘制的内容（卡片/背景）之上、画在自己的文字之下，天然满足层序，
**不需要 subOrder/排序机制**。

实现位置建议：不做进通用 `Shadow` 组件（它复用属主 MeshFilter 且不带图集采样），
而是作为文字组件内置能力——`MRLabel` / `MRRichTextLabel` / `MRTextEdit` 在
`update` 中按需提交两笔（阴影材质 + 文字材质）。

### 4.3 API 草案

```cpp
// MRLabel / MRRichTextLabel（命名与 widget Shadow 对齐）
void setShadowColor(const Vector4& color);      // 默认全透明 = 不启用阴影
void setShadowOffset(const Vector2& offset);    // 默认 (2, 2)，屏幕方向
void setShadowAlpha(float alpha);
```

默认不启用（shadowColor 全透明），开启零成本差异（不提交阴影渲染项）。

### 4.4 合批与性能

- 每个开启阴影的文字组件多 1 个渲染项 + 1 个批次（阴影材质相同可互相合批，
  但被文字项隔断——与 widget 阴影交错同样的既有问题，量级小，暂不治理）；
- glyph 顶点数翻倍只发生在开启阴影的组件上；
- SSBO：阴影 pass 初期用非 SSBO 标准路径即可；后续（widget 阴影 P3 同款）
  可为 font_shadow 加统一实例布局的 SSBO 变体，stateAttr 槽位放 offset。

## 5. 方案 C 详设：SDF 图集升级（P2）

### 5.1 光栅化改造

`DynamicFont` 的字形烘焙分支切换到 stb 自带 SDF：

```cpp
unsigned char* sdf = stbtt_GetGlyphSDF(
    &m_fontInfo, scale, glyphIndex,
    padding /*建议 4~6px*/, 255 /*onedge_value*/,
    pixel_dist_scale /*= 1/sdfRange*/, &w, &h, &xoff, &yoff);
```

- 图集 padding 需要加大（SDF 有效半径 ~ padding）；
- `pixel_dist_scale` 决定存储距离的像素单位，与 shader 采样半径换算一致；
- 多字号缓存：现有 per-fontSize 缓存结构不变，SDF 图集对字号不再敏感，
  远期可合并为单一距离场缓存（内存反降，属额外优化）。

### 5.2 font.frag 采样改造

```glsl
float d = (texture(u_atlas, v_texCoord).r - 0.5) * u_sdfRange;  // 转回 px
float coverage = smoothstep(-0.5, 0.5, d);                       // 文字本体
// 阴影变体：采样偏移 UV + 距离外扩
float dShadow = (texture(u_atlas, v_texCoord - u_offsetUV).r - 0.5) * u_sdfRange + u_spread;
float shadowAlpha = exp(-pow(max(dShadow, 0.0) / u_blur, 2.0) * 3.0);
```

- 附带收益 1：文字本体任意缩放/多 DPI 下边缘清晰（SDF 的原生优势）；
- 附带收益 2：**描边（outline）**= `|d| < w` 的覆盖带，HMI 白字黑描边零成本获得；
- 附带收益 3：与 widget 阴影的 SDF 数学同族，风格统一。

### 5.3 风险

| 风险 | 缓解 |
|---|---|
| 文字观感变化（SDF 与 coverage 的抗锯齿风格差异） | 双图集并存灰度切换 + 截图回归对比 |
| 图集内存/上传量增大（padding、SDF 位深） | 单通道 R8；padding 4~6px 实测标定；必要时降低 SDF 半径 |
| 小字号 SDF 精度 | 小字号保留 coverage 路径或提高 pixel_dist_scale |
| `stbtt_GetGlyphSDF` 生成耗时（首次烘焙） | 现有动态图集已按需生成 + 磁盘缓存可扩展 |

## 6. 推荐路径

```text
P1  方案 A：font_shadow 二次绘制（硬阴影/柔光需后续 C）
    —— 一个 shader 变体 + 文字组件一次额外提交，先落地可读性诉求
P2  方案 C：stbtt SDF 图集升级
    —— 软阴影 + 描边 + 多 DPI 清晰度，一步到位；P1 的 font_shadow
       变体退化为 C 的阴影采样模式（参数兼容，API 不变）
B（blur mip 图集）仅当"要软阴影但不想动采样数学"时作为折中
```

## 7. 测试要点

1. **层序**：带阴影文字分别放在窗口背景、卡片、图片上；阴影必须在自己文字
   之下、背景内容之上；两个相邻带阴影 label 的阴影互不穿透；
2. **形状**：阴影随字形（'O' 与 'L' 阴影形状不同）、随 offset 方向、随字号缩放；
3. **合批**：开启阴影前后 draw call / 批次数统计（DebugDemo 场景扩展用例）；
4. **（P2）观感回归**：SDF 切换前后同字号同 DPI 截图对比；小字号可读性专项；
5. **（P2）描边**：白字黑描边在地图/视频叠加场景的对比度验收。
