# MorrowUI 与 Godot 4.8 Canvas 合批策略对比

> 对比基准：Godot 4.8-dev（`E:\WorkSpace\Client\godot`，commit c51ff06e88）2D canvas 渲染链
> 对比对象：MorrowUI 当前合批体系（`ARCHITECTURE.md` 第 7 节 + `src/core/` 实现）
> 性质：结论文档，只保留仍然有效的对比结论与借鉴建议，随代码演进维护。

---

## 1. 结论摘要

1. **方向一致，不要引入全局排序/重排。** 两边都是「严格 painter's order + 相邻兼容合并」：
   顺序 = 树遍历顺序，合批只发生在相邻且状态兼容的元素之间，均无 sort key、无跨项重排。
   Godot 曾尝试文本跨命令重排（PR #116751，按字体图集页预排 glyph），**合并当天即回退**。
   MorrowUI 现有策略经过了 Godot 4.x 的大规模验证，这是本次对比最重要的确认。
2. **最大形态差异在实例化载体。** MorrowUI 是**控件级实例**（144B `UIInstanceData` 进 SSBO）
   + 字形/几何烘焙进合并 VBO（每顶点带 BatchID）；Godot 是 **quad/glyph 级实例**（128B
   `InstanceData` 走 vertex attribute，divisor=1）+ 全场共享一个单位四边形索引，
   **顶点缓冲零重传**，一个矩形/一个字形就是一个实例。
3. **per-instance / per-batch 数据切分是合批体系的灵魂，MorrowUI 已具备。**
   颜色等 uniform 值不进 `materialStateHash`、运行时填进实例槽（Button color0 等），
   与 Godot「modulate 进实例数据、材质/纹理进批状态」完全同构。该准则应固化为不变量（见 B3）。
4. **增量复用是 MorrowUI 的相对优势，保持。** Godot 每帧全量重录批次 + 全量重传实例数据；
   MorrowUI 帧间列表等价时整帧复用批次结构，VBO/SSBO 均有 revision 级跳过。
5. **落地状态**：B2（SSBO 持久映射环形分段 + fence 帧隔离，无回退路径）与
   B3（控件差异化参数必须走实例槽，固化为不变量 22 + 参数绑定护栏）已落地；
   B1（屏幕空间剔除）经项目决策不做（当前无屏幕外场景）；字形级实例流（B5）
   收益可观但需测量后决策。

---

## 2. 体系对照表

| 维度 | MorrowUI | Godot 4.8 canvas |
|---|---|---|
| 顺序模型 | 树 DFS 序；`displayLayer` 是**断批条件**（保序） | z 桶扁平化（z-index 是**排序键**，改序语义）+ ysort；桶间串成单链 |
| 分批算法 | 单遍相邻合并：`displayLayer` / `clipRect` / 双 uint64 hash 全等（`BatchBuilder.cpp:5-9`） | 线性扫描记录：`_new_batch` **拷贝上一批全部状态**，逐属性 diff（`rasterizer_canvas_gles3.cpp:1571`） |
| 兼容键 | `materialStateHash` + `meshStateHash`（revision 缓存） | material RID（item 级）+ texture RID/filter/repeat/blend/command 类型（command 级） |
| 实例粒度 | **控件级**：1 控件 = 1 个 144B SSBO 实例 | **矩形/字形级**：1 rect = 1 个 128B 实例（`gles3.h:200`，static_assert） |
| 顶点来源 | 批内所有 mesh 顶点拼入合并 VBO（每顶点 BatchID 属性），revision 变化才重传 | 共享单位四边形（6 索引），实例数据即顶点输入，**从不重传顶点** |
| draw 形态 | 合并 VBO + SSBO 索引取实例 → 1 次 `glDrawElements`（非硬件 instancing） | `glDrawElementsInstanced(GL_TRIANGLES, 6, ..., instance_count)`（`gles3.cpp:1315`） |
| 每批断批状态 | shader/纹理/blend/cull/clipRect | 同左，另含 filter/repeat、`lights_disabled`、buffer 满强制断批 |
| 颜色/modulate | uniform 值不进 hash，进 SSBO 实例槽（同色异值可合批） | 进 `InstanceData`（同批逐实例异色）；仅 LCD 文本时颜色=blend constant 会断批 |
| 文本 | 整串文本 = 1 RenderItem，字形 CPU 烘焙进单 Mesh；label 间相邻可再合批 | 每字形 1 个 `CommandRect` 实例；同图集页相邻即同批，**一页字体一个 draw** |
| clip | `ClipRect` 值精确相等才合批；批首设 scissor | bake `final_clip_owner`（指针）；批间 diff 切 scissor；`CLIP_IGNORE` 用「制造状态差异」自然 reclip |
| 增量复用 | 帧间列表等价 → 整帧复用批次结构（cache hit），VBO/SSBO 按 revision 跳过 | **无**：每帧全量重录 batch + 全量重传实例数据 |
| 缓冲管理 | 每帧 `glBufferData(GL_STREAM_DRAW)` 全量 orphan | persist-mapped 环形 DataBuffer（16384 实例 ≈2MB）+ GLsync fence 三缓冲；**满即拆批换缓冲** |
| 视口剔除 | 无（仅空 clipRect 子树跳过） | cull 层 rect intersects 剔除（`renderer_canvas_cull.cpp:260`） |
| 诊断 | `BatchStatistics` + 断批原因归因 `[L/M/G/O]`（Godot 无此粒度） | RenderInfo 计数；无断批归因 |

---

## 3. MorrowUI 现状要点（对比相关的事实）

以下为源码核实的精确事实，补充 `ARCHITECTURE.md` 未展开的部分：

- **无排序**：`BatchBuilder::build`（`src/core/BatchBuilder.cpp:27-53`）只做相邻合并，
  顺序 = Widget 树 DFS 序。`BatchBuilder.h:25` 头注释中「按 (displayLayer, materialKey) 排序」
  已过时；`git log -S "std::sort" -- src/core/` 为空，排序从未存在。
- **`insertionIndex` 是预留死字段**：唯一写入点 `BatchManager.cpp:44`，全仓库无读取点。
- **批次重建是整帧粒度**：`isRenderableListUnchanged`（`BatchManager.cpp:63-74`）逐下标比对
  指针同一 + hash 全等；任一控件变化 → 整帧 `buildBatches`（先 `RenderBatchPool::releaseAll`）。
  无局部增量。
- **SSBO 每帧全量重传**：cache hit 帧复用批次结构，但绘制时 `SSBOManager::updateSSBO`
  （`SSBOManager.cpp:30-48`）仍然 `resize` + 逐实例填充 + `glBufferData(GL_STREAM_DRAW)`。
  实例数据来自 transform/material **当前值**（矩阵、颜色），所以列表等价 ≠ 数据等价，
  不能直接跳过——这正是 B2 的出发点。
- **clip 是值语义**：`ClipRect::operator==`（`include/morrow/ClipRect.h:41-45`）为精确 float
  相等；同框异主的控件可合批（比 Godot 的 owner 指针比较更激进，且安全：scissor 相同即可合并）。
- **无视口剔除**：`BatchManager` / `BatchBuilder` 无任何屏幕空间相交测试；屏幕外 widget
  仍完整走收集 → 合批 → 提交链路。
- **underlay 通道保留但无提交者**：阴影重构后走通用管线（`underlay=false`）。

## 4. Godot 4.8 体系要点

完整数据流（`renderer_canvas_cull.cpp` → `drivers/gles3/rasterizer_canvas_gles3.cpp` /
`renderer_rd/renderer_canvas_render_rd.cpp`）：

```text
CanvasItem 树（RCD 命令链，10 种命令）
    ↓ cull 层：递归收集，z 桶尾插（8192 桶），ysort 子树扁平化排序，
      clip 交集下传，rect intersects 剔除 → 一条 Item 单链（cull 层不合批）
    ↓ 后端：链表拷入扁平数组，_render_items 先【记录】后【绘制】
       item 级断批：clip owner 变化 / material RID 变化
       command 级断批：texture RID、filter/repeat、blend_mode、command 类型……
       _new_batch = 拷贝当前批状态 + instance_count=0（断批退化为逐属性 diff）
    ↓ 顺序遍历 batch 数组：切 scissor → 材质 UBO → blend → 1 次 instanced draw/批
```

关键设计：

- **`_new_batch` 拷贝+diff**（`rasterizer_canvas_gles3.cpp:1571-1592`）：新批继承上一批全部
  状态，扫描过程零哈希、零排序；`r_batch_broken` 保证一条命令内多次属性变化只建一个新批。
- **128B 定长 `InstanceData`**（`gles3.h:200-227`）：world 矩阵 2x3、modulation、src/dst rect、
  ninepatch margins、flags、instance_uniforms 偏移、lights[4]。**modulate/颜色/transform/灯光
  索引全是 per-instance**——普通 MIX 下 modulate 变化不断批，文本逐字形异色也能一页一个 draw。
- **共享单位四边形 + 硬件 instancing**：全场顶点只有 6 个索引；实例数据以 vertex attribute
  （location 8..15，stride 128，divisor=1）下发（`_enable_attributes`，gles3.cpp:1594）。
- **缓冲满即拆批 + 帧隔离**：`max_instances_per_buffer = 16384`（≈2MB/缓冲），满则上传本段、
  换下一个 GL buffer、强制新批（`_add_to_batch`，gles3.cpp:1547-1569）；帧首检查 fence，
  3 帧内仍在用就换新，超过 2 帧 `glClientWaitSync`（gles3.cpp:118-142）。
- **文本 = 实例流**：`TextServerAdvanced::_font_draw_glyph` 对每字形发一个
  `CommandRect`（图集页 + uv 子矩形），无 glyph mesh、无字形顶点烘焙；合批完全靠后端扫描聚簇。
- **polygon / mesh / MultiMesh 一律自成一批**（注释："can't be batched"）。
- **读回屏幕的节点（CanvasGroup / backbuffer 材质）是天然强制拆 pass 点**：主循环遇到即先
  flush 累积 items（gles3.cpp:471/526）——与 MorrowUI BackdropBlur 的分段建模互相印证。
- **RD 后端同构**，另有：`PipelineKey`（variant/fb 格式/顶点格式/specialization）哈希查
  pipeline；`TextureState` 打包键查 uniform set LRU；instance uniforms 走全局参数缓冲 +
  偏移索引（与 MorrowUI SSBO 思路一致）；polygon 实例数据走 push constant（最高位标记）。

---

## 5. 逐项差异分析

### 5.1 顺序模型：断批条件 vs 排序键（语义不同，不要混）

- MorrowUI `displayLayer`：不同层交错 `A(L0) B(L1) A(L0)` 按 **A B A** 绘制，层边界断批。
- Godot z-index：同输入绘制为 **A A B**（z 桶重排），z 语义就是「改绘制顺序」。
- MorrowUI 若引入按层分段收集，会在层交错时改变 painter's order，违反不变量 7/8。
  **结论：维持现状。** 层交错导致的批次碎片化由 `Break[L]` 统计监控即可；
  只有当业务明确需要 Godot 式「层内跨越改序」语义时才重新评估。

### 5.2 分批算法：hash 全比较 vs 拷贝+diff

两者渐进复杂度相同（单遍扫描），语义一致。MorrowUI 的 hash 比较还额外服务帧间等价比对
（Godot 没有这层），**无需改动**。差异仅在工程风格：Godot 断批继承上一批状态，天然表达
「状态只在变化处更新」；MorrowUI 每批独立持有 clipRect/材质，绘制时显式设置。现状更直观。

### 5.3 实例粒度与顶点来源（核心形态差异）

| | MorrowUI | Godot |
|---|---|---|
| 实例含义 | 一个控件（widget） | 一个 quad / 一个字形 |
| 实例大小 | 144B（SSBO） | 128B（vertex attribute） |
| 顶点 | 每批合并 VBO，按 mesh revision 跳过重传 | 永久共享单位四边形，零重传 |
| 动态成本 | mesh 变化 → 该批 VBO 重传（该批全部顶点） | 实例数据全量重传（但本来就是流式） |

- MorrowUI 的「BatchID 顶点属性 + SSBO 按索引取实例」与 Godot「实例 attribute 从实例缓冲
  按顶点拉取」本质相同（都是顶点着色器索引化实例数据）；差别只在**顶点从哪来**。
- MorrowUI 已用 VertexArray 级 revision 缓存消除静态 UI 的 VBO 重传，故此差异的主要代价在
  **mesh 烘焙与批次级重传粒度**（批内任一 mesh 变化 → 整批顶点重传），以及 quad 类控件
  的 CPU mesh 生成/管理成本。B4（单位四边形实例化）是针对性的可选优化。

### 5.4 per-instance / per-batch 切分

两边同构：**能进实例数据的绝不进批状态**。Godot 的反面教材是 LCD 文本——颜色必须是
blend constant，只能进批状态，导致逐色断批；普通路径把颜色下沉为实例数据后文本免费合批。
MorrowUI 等价机制已存在（uniform 值不进 `materialStateHash`，经 `SSBOFieldBinding` 填实例槽）。
**风险点在未来扩展**：新增控件 layout 时，把本可实例化的参数（颜色、进度、状态位）误做进
材质状态或 mesh revision，会静默劣化合批——应固化为架构不变量（B3）。

### 5.5 文本

| | MorrowUI | Godot |
|---|---|---|
| 结构 | 整串文本 1 实例 + 字形 CPU 烘焙进单 Mesh | 每字形 1 实例（uv+offset+color），无 mesh |
| 布局变化成本 | 脏时重建整串 mesh + VBO 重传（有 revision 缓存） | 仅实例流追加/重写，无顶点概念 |
| label 间合批 | 相邻同 hash label 合并（字形已并入批 VBO） | 相邻同图集页聚簇（一页一个 draw） |
| 图集扩容 | 版本比对 → 强制重建 UV、断批重建 | uniform set invalidation 回调 |

静态文本两边成本接近；差异集中在**动态文本**（光标移动、逐字输入、滚动、选区变化）：
MorrowUI 每次脏都重建整串 mesh，Godot 只重写实例区间。B5（字形级实例流）即针对此，
但需要引入第二层实例布局（字形 layout），与 7.6 的控件族 layout 体系正交，**需测量后决策**。

### 5.6 clip

MorrowUI 用**值相等**（激进但正确：scissor 相同即可合并）；Godot 用 **owner 指针** +
批间 diff + `CLIP_IGNORE` 的 reclip 技巧（把 `current_clip` 置空制造状态差异，迫使下批重建
从而重新启用 scissor，`gles3.cpp:1257-1290`）。MorrowUI 无 `CLIP_IGNORE` 等价需求，
现状无需改动；reclip 技巧仅在引入「忽略父裁剪」控件时再参考。

### 5.7 增量复用（MorrowUI 优势区）

Godot 无帧间复用：每帧重录全部 batch、全量重传实例数据（persist-mapped 从头写，
注释明言 "Data is always uploaded from scratch"）。MorrowUI 的整帧列表等价比对 +
revision 缓存 + `RenderBatchPool` 池化在「静态为主、局部动画」的仪表/HMI 场景下
省掉整个记录阶段。**保持，不借鉴此方向。** 但注意 B2：等价帧的 SSBO 数据重传
（矩阵/颜色当前值）目前无任何跳过手段，与「整帧复用」承诺不一致，是最值得补齐的一环。

### 5.8 缓冲与同步

Godot：16384 实例 ≈2MB 分块、`glMapBufferRange(UNSYNCHRONIZED)` + fence 三缓冲、
满即拆批换缓冲。MorrowUI：每帧 `glBufferData(GL_STREAM_DRAW)` 重新分配（驱动侧 orphan），
无帧隔离 fence，无满载拆批（依赖批次结构本身有限）。嵌入式（QNX/EGL）驱动上，
反复大块 orphan 分配与隐式同步代价可能显著——B2。

---

## 6. 借鉴建议

### B1 屏幕空间剔除（经项目决策不做）

Godot cull 层对每个 item 做 `p_clip_rect.intersects(p_global_rect, true)`，屏幕外元素
不进入渲染链。MorrowUI 无视口剔除，但当前产品形态（嵌入式仪表/HMI）不存在屏幕外
widget 场景，剔除无收益。若未来出现可滚动超界内容再评估此条。

### B2 SSBO / 上传缓冲帧隔离与 orphan 化（第一阶段已落地）

**已落地**：`GLRenderDevice::updateSSBO` 重写为 glBufferStorage 持久映射环形分段
（3 段轮转、每段独立 fence 写入前等待，稳态零阻塞），每帧 memcpy 取代
`glBufferData` 重分配；`glGetIntegeri_v` 绑定校验降为每 SSBO 生命周期一次；
帧末由 `RenderDeviceProxy` 调 `endSSBOFrame()` 落账 fence；无回退路径
（桌面 GL ≥ 4.4 / QNX ES 3.2 均为核心 API，能力不足时 LOG_E 并跳过上传，
仅防御硬崩溃）。上游 staging 数据流、SSBOData 池、PendingFrame 回收未改动。
诊断（创建/重建/段等待/超时计数）按不变量 13/20 在设备析构汇总输出。

**第二阶段（待做）**：等价帧跳过 SSBO 重传。可用「实例槽来源的 revision 聚合签名」
（参照 BackdropBlur 脏签名）实现；Tween 动画帧自然失效，静态帧直接整段跳过。
跳过帧需绑定最近已写分段，与环形轮转的同段并发需按 5.7 的 identical-content
边界单独设计，建议拿到第一阶段基线数据后评估收益再实施。

### B3 控件各自的参数必须走实例槽，不进共享状态（「per-instance 优先」固化为架构铁律，已落地）

**已落地**：`ARCHITECTURE.md` 第 14 节不变量 22 + `SSBOManager::validateBatchParamBindings`
诊断护栏——合并 SSBO 批填充时逐材质比对参数名与绑定表消费集合
（`collectLayoutConsumedProperties` 纯 CPU 判定，消费集合按 layout 指针缓存，
scratch 复用保证稳态零分配），未登记参数（会被 `applyBatch` 静默丢弃）
按 (layout, 参数) 限流 LOG_W。`Material::collectParamNames` 为公共面只加 API。

为什么是合批体系的判定灵魂：一次 draw 的成本结构是「批状态切换（= 断批，API 往返）」
vs「实例数据（同一次 draw 内摊销）」——把参数从批状态挪进实例数据，语义不变地把
「N 个对象 N 次 draw」变成「1 次 draw + N×字段字节数」。Godot 的教科书案例：
文本 modulate 进实例数据 → 一页图集 1 个 draw；LCD 子像素路径颜色必须是 blend
constant（批状态）→ 逐色断批（`gles3.cpp:915-918`）。MorrowUI 的失效模式是：
新增控件参数做成 material uniform → 标准路径与单素材批正常 → 相邻两个同族控件
合并后参数静默丢失（`applyBatch` 只应用 program + 纹理）——「单控件正确、
多控件错」最难排查。护栏让违规在第一时间可见。

### B4 矩形类几何单位四边形实例化（P1，需测量）

对 quad/9-slice 族控件，用共享单位四边形索引替代逐控件 mesh（实例数据携带 rect/uv/margin），
消除 quad 控件的 CPU mesh 生成与批级 VBO 重传。仅当测量显示：
(a) quad 类 mesh 烘焙占 CPU 帧时间可观；或 (b) 动态 quad 批次 VBO 重传频繁时再实施。
实施时应与 B5 共用「字形/矩形实例 layout」的基础设施。

### B5 文本字形级实例流（P1，收益集中在动态文本）

每字形一个实例（uv offset/size、笔位、颜色/alpha），替代整串 CPU mesh 烘焙：

- 动态文本（输入、光标、滚动、选区）只重写受影响实例区间，免整串重建与 VBO 重传；
- 跨 label 同图集页天然同批（实例流直接拼段）；
- 代价：实例从 1 label/144B 变为 1 glyph/约 48-64B（uv+pos+color 压缩后），
  实例数量级上升；静态长文本的首帧填充成本略增。
- 决策依据：`TextDemo`/输入类 demo 的文本脏重建耗时 profiling；若当前 mesh 重建
  （`MRLabel::createTextMesh`）在目标设备上不可测，维持现状。

### B6 命令/批次内存 bump allocator（P2）

Godot 的 4KB `CommandBlock`（只增不减、clear 重置 usage）针对「大多数 item 只有一条命令、
每帧重绘」场景。MorrowUI 的 swap 双缓冲 vector + `RenderBatchPool` 已覆盖同类需求，
仅在 RenderItem 数量达到数万级、分配开销可测时再考虑。

### B7 状态前置 pipeline key（P2，条件触发）

RD 后端的 `PipelineKey`/`TextureState` LRU 思想（状态切换成本前置、uniform set 缓存）
仅在 MorrowUI 引入 Vulkan/Metal/GLES3 重写或管线状态显著劣化时评估；当前 GL 后端的
惰性状态切换路径已足够。

### B8 z 桶排序语义（P2，需求触发）

仅当业务出现「必须跨 displayLayer 改序」的需求时评估；与不变量 7 冲突，需先修订需求
再动架构。当前 `Break[L]` 统计足以监控层交错的碎片化代价。

---

## 7. 明确不建议照搬

| 方向 | 原因 |
|---|---|
| 全局排序 / sort key / 跨项重排 | 两边共同实践「相邻合并」；Godot 文本 presort（PR #116751）合并当天回退，API 面与正确性风险过高 |
| Godot 式每帧全量重录 | MorrowUI 帧级等价比对在静态为主场景下严格更优 |
| clip owner 指针语义 | MorrowUI 值语义合并更多且正确，无需回退 |
| CanvasGroup 回读 pass 机制 | BackdropBlur 分段建模（不变量 21）已等价覆盖，二者互相印证 |
| 每 item 15 盏灯索引 per-instance 打包 | MorrowUI 无 canvas lighting；若未来引入，按「有光/无光断批 + 索引进实例」的 Godot 形态起步 |

---

## 8. 关键源码索引

MorrowUI：

| 文件 | 内容 |
|---|---|
| `src/core/BatchBuilder.cpp:5-53` | 相邻合并 + 断批归因 + 帧间等价比对 |
| `src/core/BatchBuilder.h:17-46` | `BatchCompatibilityKey`（双 uint64 hash）与 `RenderItem` |
| `src/core/BatchManager.cpp:63-74,129-146,184-190` | 整帧复用判定、路径选择、重建 |
| `src/core/BatchManager.cpp:242-286` | SSBO 批 / 标准批 / 安全回退绘制 |
| `src/renderer/resource/ssbo/SSBOManager.cpp:30-48` | 每帧全量实例填充 |
| `src/renderer/resource/VertexArray.cpp:76-239` | 合并 VBO + BatchID 顶点属性 |
| `src/renderer/resource/Material.cpp:483-515` | `materialStateHash` 组成（不含 uniform 值） |
| `src/ui/elements/MRLabel.cpp:371-414` | 文本 CPU 烘焙单 Mesh |
| `include/morrow/BatchStatistics.h` | 断批原因枚举与统计面 |

Godot 4.8（`E:\WorkSpace\Client\godot`）：

| 文件 | 内容 |
|---|---|
| `servers/rendering/renderer_canvas_cull.cpp:71-109,260,304-493` | z 桶收集、rect 剔除、递归 cull |
| `servers/rendering/renderer_canvas_render.h:168-502` | Item / 10 种 Command / 4KB CommandBlock |
| `drivers/gles3/rasterizer_canvas_gles3.cpp:576-806` | 记录 + 绘制主循环、item 级断批 |
| `drivers/gles3/rasterizer_canvas_gles3.cpp:814-1291` | `_record_item_commands`（command 级断批全表） |
| `drivers/gles3/rasterizer_canvas_gles3.cpp:118-142,1547-1592` | fence 帧隔离、buffer 满拆批、`_new_batch` |
| `drivers/gles3/rasterizer_canvas_gles3.h:200-289` | 128B `InstanceData` / `Batch` / `DataBuffer` |
| `servers/rendering/renderer_rd/renderer_canvas_render_rd.cpp:2187-2338,3247-3289` | RD 后端记录/绘制/`_new_batch` |
| `servers/rendering/renderer_rd/renderer_canvas_render_rd.h:128-147,407-465,516-610` | PipelineKey / TextureState / RD Batch / MultiUmaBuffer |
| `servers/rendering/instance_uniforms.h` | 实例 uniform 全局缓冲 + 偏移索引 |
| `modules/text_server_adv/text_server_adv.cpp:4261-4402` | 每字形一个 rect command |
| git `892381663ba` / `2f335f1698` | 文本 presort PR 及当日回退（跨项重排的反面教材） |
