# 文本与字体优化现状分析

## 1. 概述

对照 ARCHITECTURE.md §15.13 列出的 6 条文本/字体优化方向，逐条检查当前实现状态，标注已完成项和未完成项，并给出下一步建议。

---

## 2. 逐条对照

### 2.1 字形 Atlas 分页和回收 — ❌ 未实现

**当前实现**：`DynamicFont` 使用**单一纹理图集**（初始 512×512），采用简单的逐行装箱（row-packing）。当图集空间不足时调用 `ExpandTextureAtlas()` 将尺寸翻倍（512→1024→2048…），并**丢弃旧纹理、从零重新生成所有字形**。

```cpp
// DynamicFont.cpp — ExpandTextureAtlas()
bool DynamicFont::ExpandTextureAtlas() {
    int32_t newWidth = m_atlasWidth * 2;
    int32_t newHeight = m_atlasHeight * 2;
    return CreateTextureAtlas(newWidth, newHeight);
    // 将所有 m_glyphCache 标记为 generated=false，触发完全重建
    // 旧 GPU 纹理被销毁，新建更大纹理
}
```

**问题**：
- 扩容代价是 O(已有字形数)，在中文字体场景（数千字形）下会产生可感知的卡顿
- 旧纹理被废弃后无法复用，浪费 GPU 显存
- 无分页机制：所有字形挤在一张纹理上，无法按使用频率分层管理

**是否有分页**：否。无 LRU 淘汰、无多纹理页管理、无 glyph eviction。

### 2.2 文本布局结果缓存 — ⚠️ 部分实现

**当前实现**：MRLabel 有脏标记机制，文本/字体/换行策略变化时设 `m_isTextLayoutDirty = true`，但**没有独立的布局缓存**：

```cpp
// MRLabel.cpp — update()
void MRLabel::update(FrameStateSharedPtr frameState) {
    if (m_isTextLayoutDirty || m_isAlignDirty) {
        processTextLayout();   // 每次脏都完整重算
        applyAlignment();
        createTextMesh();      // 每次脏都完整重建 Mesh
        m_isTextLayoutDirty = false;
        m_isAlignDirty = false;
    }
}
```

**已实现部分**：文本不变时，`processTextLayout()` 和 `createTextMesh()` 被完全跳过——这直接解答了"未变化文本避免重新生成几何"。

**未实现部分**：
- 没有将布局结果（行列表、字形位置）作为可复用的缓存对象。两个相同文本内容但不同容器宽度的 MRLabel 需要各自独立计算
- `processTextLayout()` 中每字符调用 `m_font->GetGlyph(c)` 产生 O(n) 的 `unordered_map` 查找，没有为整串文本做批量字形查找优化

### 2.3 未变化文本避免重新生成几何 — ✅ 已实现

**当前实现**：通过双层脏标记完全实现了此优化：

```cpp
// MRLabel.h
bool m_isTextLayoutDirty = true;  // 文本内容/字体变化
bool m_isAlignDirty = true;       // 对齐/尺寸变化

// 每个 setter 都做变更检测：
void MRLabel::setText(const std::wstring& text, const std::string& fontName) {
    if (m_text != text || m_fontName != fontName) {
        m_text = text;
        m_fontName = fontName;
        m_isTextLayoutDirty = true;
        requestRender("setText");
    }
}
```

**完整跳过链**：

```
MRLabel::update()
  → 文本/对齐未变化 → 跳过 processTextLayout() + createTextMesh()
  → Mesh revision 不变
  → VertexArray::needsMeshUpload() 返回 false → 零 VBO 上传
  → BatchManager 批次缓存命中 → 零 Draw Call 重建
```

**结论**：这条完全不需要再优化。

### 2.4 相同字体和 Atlas 的文本连续合批 — ✅ 已实现

**当前实现**：BatchCompatibilityKey 已包含 Shader variant 和 Texture 集合：

```
BatchCompatibilityKey:
├── Shader 名称 + Variant
├── Texture 集合
├── Blend 状态
├── Cull 状态
├── Vertex Layout
└── ...
```

使用同一 `font.vert`/`font.frag` shader 且绑定同一张 `FontTexture` 图集的文本 Quad 自动共享 `BatchCompatibilityKey`，落入同一批次。

**结论**：当前合批框架已天然支持此优化，不需要额外工作。

### 2.5 降低多语言和动态字号导致的 Atlas 抖动 — ❌ 未实现

**当前实现**：各字号独立创建 `DynamicFont` 实例，各自维护独立图集：

```cpp
// FontManager 支持按 (name, size) 注册独立字体实例
// 字号 16/24/32/48 → 4 个 DynamicFont → 4 张独立图集
```

**问题**：
- 多字号场景下每张图集预生成独立 ASCII 字形，存在冗余
- 动态切换字号时新字号图集为空，需要逐帧生成字形，产生可见延迟
- 无可配置的图集尺寸策略（如根据目标语言预估字形数）
- 单图集扩容时丢弃旧纹理的设计加剧了"抖动"——扩容 → 全量重建 → 再次扩容 → 再次全量重建

**需要引入**：
- 图集大小预估策略：根据字体文件包含的字形数量预设合理初始尺寸
- 分页机制：避免扩容时重建已有字形

### 2.6 记录字形上传、Atlas 命中和文本重建统计 — ❌ 未实现

**当前实现**：仅通过通用的批次级 `BatchStatistics` 记录 cache hit/miss，无文本/字体专用统计：

```cpp
struct BatchStatistics {
    uint32_t cacheHitCount = 0;
    uint32_t cacheMissCount = 0;
    // 无：glyphGeneratedCount, atlasHitCount, atlasMissCount, textRebuildCount
};
```

**缺失的统计指标**：
| 指标 | 含义 | 当前状态 |
|---|---|---|
| 字形生成次数/帧 | `GenerateGlyphToAtlas()` 调用次数 | 未统计 |
| 字形缓存命中率 | `m_glyphCache` 命中 vs 未命中 | 未统计 |
| 图集扩容次数 | `ExpandTextureAtlas()` 调用次数 | 未统计 |
| 文本重建次数/帧 | `createTextMesh()` 调用次数 | 未统计 |
| 字形上传字节数 | `FlushPendingUploads()` 上传量 | 未统计 |
| 字形上传调用次数 | `glTexSubImage2D` 因字体触发的次数 | 未统计 |
| 待上传队列峰值 | `m_pendingUploads.size()` 最大值 | 未统计 |

---

## 3. 汇总

```
§15.13 优化方向                                  状态
─────────────────────────────────────────────────────
字形 Atlas 分页和回收                             ❌ 未实现
文本布局结果缓存                                  ⚠️ 部分实现（脏标记有，独立缓存层无）
未变化文本避免重新生成几何                         ✅ 已实现（双层脏标记 + Mesh revision）
相同字体和 Atlas 的文本连续合批                     ✅ 已实现（BatchCompatibilityKey 天然支持）
降低多语言和动态字号导致的 Atlas 抖动               ❌ 未实现
记录字形上传、Atlas 命中和文本重建统计               ❌ 未实现
```

---

## 4. 建议

### 4.1 立即可做（低风险）

**添加字形/文本统计**：
- 在 `DynamicFont` 中增加计数器（`glyphGenerated`, `glyphCacheHit`, `atlasExpansions`, `uploadBytes`）
- 在 `MRLabel` 中增加 `textRebuildCount`
- 接入 `BatchStatistics` 或 `DebugPlane` 显示
- 工作量：~30 行，零架构影响

### 4.2 按需推进（出现瓶颈时）

**图集分页**：
- 当前单图集方案对 MorrowUI 的典型使用场景（仪表盘：固定少量文本）**完全够用**
- 仅在以下场景需要考虑分页：中文本地化（数千字形）、运行时动态字号切换、多字体同时使用
- 若实施：引入 `FontAtlasPage` 类，图集满时创建新页而非扩容旧页

**布局缓存**：
- 当前双层脏标记已覆盖主要优化路径
- 独立布局缓存仅在"大量相同文本内容的 Label"场景下有价值（如列表视图）
- 建议在出现此类场景时再引入 `TextLayoutCache` 单例

### 4.3 建议更新 ARCHITECTURE.md

将 §15.13 中已实现的两条改为标注完成状态：

```markdown
### 15.13 文本与字体

优化方向：

- 字形 Atlas 分页和回收；
- 文本布局结果缓存；
- ~~未变化文本避免重新生成几何~~ ✅ 已实现（MRLabel 双层脏标记 + Mesh revision）
- ~~相同字体和 Atlas 的文本连续合批~~ ✅ 已实现（BatchCompatibilityKey）
- 降低多语言和动态字号导致的 Atlas 抖动；
- 记录字形上传、Atlas 命中和文本重建统计。
```
