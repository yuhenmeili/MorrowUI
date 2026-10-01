# MorrowUI 性能优化方向与决策标准

> 自 ARCHITECTURE.md §15/§16 迁出（2026-09-30）：架构文档只保留架构说明，
> 后续优化方向、实施状态与优化决策标准集中到本目录。条目编号沿用原序号
> （原 15.N 对应本文件第 N 条）。专项路线见同目录
> [PERFORMANCE_OPTIMIZATION_ROADMAP.md](PERFORMANCE_OPTIMIZATION_ROADMAP.md)。

状态标记：

- ✅：已完成或当前方案已收敛；
- ⏸️：保留为需求驱动的候选项，不属于短期计划；
- 🔜：结合当前代码热路径确认值得近期推进。

原有 Tier 仅表示问题类别，不再表示当前实施优先级。除已完成项外，1～18
中原有的剩余方向统一降为非短期候选；近期建议以 19 的代码审查结果为准。

---

## Tier 1 — 工程质量基础

### 1. 渲染回归测试 ⏸️

> 当前状态：非短期。出现明确渲染兼容性风险或跨平台回归后再扩展。

仓库当前不保留测试目录（tests/ 与临时验证设施已按评审移除）；渲染验证以
samples 目检和 `DebugDemo --report-json` 指标（renderItems/batches/drawCalls）
为基线，无渲染级回归覆盖。

需要覆盖：

- 半透明元素重叠；
- display layer 和 insertion order；
- Material、Texture 和 Blend 动态变化；
- SSBO 与标准路径一致性；
- Clip 和 Stencil 嵌套；
- 2D 与 3D 混合；
- Offscreen RenderTarget；
- Window resize；
- 单线程和多线程一致性；
- golden image 或 perceptual diff。

### 2. 压力测试与 CI ⏸️

> 当前状态：非短期。随发布流程和目标平台交付要求推进。

无 CI 配置，无压力测试。

需要覆盖：

- 大量 Widget 更新；
- 大量 RenderItem 和 Batch；
- CommandBuffer 满载；
- 环形 frame slot 阻塞；
- 大纹理上传；
- 动态 VBO 高频更新；
- GPU 资源反复创建和销毁；
- 多窗口创建、缩放和关闭；
- 单线程与多线程退出；
- Windows / MinGW 构建；
- QNX / EGL 交叉编译；
- 单元测试、固定帧示例和渲染回归自动执行。

### 3. 性能时间线 ⏸️

> 当前状态：非短期。短期优化先使用固定场景、现有 Batch 统计和局部计时验证。

已有：`DebugPlane` 显示 FPS、Batch 统计（cache hit/miss、break reason 计数、
Draw Call 计数），默认隐藏且关闭时不更新文字。Windows runtime 和 MorrowEditor
可按 `F3` 切换；也可通过 `EngineOptions::debugOverlayVisible` 设置初始状态。
`MORROW_ENABLE_DEBUG_OVERLAY=OFF` 会在 CMake 配置阶段移除 `DebugPlane.cpp` 并
裁掉 Engine 快捷键处理。当前仍缺失各阶段耗时分离。

需要建立统一、低开销的性能统计：

- Input / Animation / Widget update / lateUpdate 各阶段耗时；
- RenderItem collection / Batch build / Command encode 耗时；
- Render thread execute / Present 耗时；
- Texture / Buffer upload 字节数和耗时；
- 各 3D/Offscreen Pass GPU 时间；
- 环形缓冲等待时间和次数；
- Triangle 计数；
- 统计结果应支持固定场景导出和版本间对比。

---

## Tier 2 — CPU 关键优化

### 4. Widget Dirty 体系完善 ✅

已有：`Transform` 通过局部数据版本、局部矩阵版本、世界矩阵使用的局部版本及父节点世界版本进行缓存失效检测；`Widget` 不可见子树跳过 update；`MRLabel` 双层文本脏标记；`Mesh::m_revision` 驱动 VBO 跳过上传；`Material::m_batchCompatibilityRevision` 驱动合批 key 缓存失效，`Material::m_uniformRevision` 独立记录普通 uniform 数据变化。当前不建议强行引入跨模块统一 Dirty 枚举，主要问题应转为明确各级缓存的失效边界，避免“数据变化”与“结构变化”混用。

优化方向：

1. **不再推进跨模块统一 Dirty 枚举**：**方案调整，不作为当前优化目标**。`Mesh::m_revision` 已经负责判断对应 `VertexArray` 是否需要重新上传 VBO；`BatchCompatibilityKey` 已经负责判断 shader / texture / blend / topology / vertex layout / clip 等批次结构是否兼容；`Transform` 版本号负责局部矩阵和世界矩阵缓存；`MRLabel` 的文本排版脏标记只服务于文本布局阶段。它们的缓存粒度、生命周期和消费者不同，统一为一个 `DirtyFlag` 反而会引入跨层依赖，并不能减少实际判断。保留各模块独立的 dirty/revision 机制，仅要求命名、注释和失效契约清晰即可。**已完成（方案收敛）**。
2. **世界矩阵独立 dirty 判定**：**已完成**。`Transform` 已移除 `m_matrixDirty` 与 `m_worldMatrixDirty`，改用版本号判断局部矩阵和世界矩阵是否失效；`getWorldMatrix()` 会比较父节点身份及父节点世界版本，未失效时直接返回缓存矩阵。父节点版本读取前会先刷新父世界矩阵，以确保祖先节点变化可以正确向下传播。
3. **批次结构变化与资源上传变化解耦**：**已完成**。`RenderItem::isSameRenderableAs()` 不再比较 `materialRevision`、`geometryRevision` 等资源内容版本，而是比较渲染项身份、`displayLayer` 和 `BatchCompatibilityKey`；Mesh 内容变化继续由 `Mesh::m_revision` / `VertexArray::needsMeshUpload()` 负责 VBO 上传，材质参数变化继续由 `Material::apply()` 负责 uniform 提交。只有影响批次兼容性的 shader、纹理集合、blend / cull、primitive topology、vertex layout、clip / stencil 等结构变化才会触发批次重建。

> 说明：引擎面向全屏 GUI 渲染，Widget 总是在视口内绘制，不存在屏幕外可见区域剔除（视口裁剪）需求，该方向不作为优化目标。

### 5. GPU 状态缓存 ✅

已实现：`GLRenderDevice` 内建 `GLStateCache` 结构，缓存当前 Shader、Texture（每单元）、Blend、Viewport、DepthTest、DepthWrite、CullFace 状态。每次状态设置前先比较缓存值，相同则跳过 GL 调用。`drawVBO` 和 `bindRenderTarget`/`unbindRenderTarget` 自动同步缓存。

---

## Tier 3 — 框架补全

### 6. 合批 Key 缓存 ✅

已完成：`Material` 和 `Mesh` 分别缓存自身的 `BatchCompatibilityKey` hash 部分。Material 使用 `m_batchCompatibilityRevision` 和 `m_uniformRevision` 分离合批兼容状态与普通 uniform 数据变化；当前合批缓存只消费前者，后者预留给后续 uniform dirty/UBO 上传刷新机制。Mesh 因资源 revision 还承担 VBO 上传判断，额外使用 batch compatibility revision。对应合批 revision 未变化时，`createBatchKey()` 直接复用缓存结果，不再重复遍历 Material 纹理集合或重算 Mesh layout hash。

失效边界：

- Material 的 texture、shader、blend 和 double-sided 状态变化时失效；普通 uniform 参数变化不影响缓存；
- Mesh 的 primitive topology 或 vertex layout 变化时失效；顶点、索引和 attribute 内容变化不影响缓存；
- `shaderName` 保持现有 string 表示，本阶段不改为整数 ID；
- 保持现有绘制顺序和合批策略，本阶段不引入分层排序。

该项按当前方案全部完成。

### 7. RenderItem 热路径紧凑化 ⏸️

> 当前状态：非短期。SoA、整数 ID 和所有权调整仅在 profiling 证明 RenderItem
> 遍历或引用计数为主要瓶颈后推进。

已有：`BatchCompatibilityKey` 已大量使用整数/位域；`RenderItem` 列表通过 swap 复用容量；`RenderBatchPool` 按 shader 键池化。缺失 SoA 布局和指针消除。

优化方向：

- 收集阶段减少 `shared_ptr` 拷贝（`addRenderable` 当前按值拷贝增加引用计数）；
- 将稳定 Shader、Texture、Material 和 Layout 信息缓存为整数 ID；
- 将高频字段组织为连续紧凑结构；
- 评估 SoA 布局对遍历、排序和合批的收益。

### 8. Clip 与 Stencil ⏸️

> 当前状态：非短期。属于功能补全，不作为当前性能优化目标。

已有：`BatchCompatibilityKey` 包含 `clipStateId`/`stencilStateId`；`BatchBreakReason::ClipState` 产生断批；`DebugPlane` 显示断批次数。缺失底层 GL 状态实现。

优化方向：

- 将 Clip Rect 和 Stencil 状态纳入实际 GL 提交（`glScissor` / `glStencilFunc` / `glStencilOp`）；
- 明确嵌套裁剪的 push/pop 生命周期；
- 优先使用 Scissor 处理矩形裁剪；
- 复杂路径裁剪使用 Stencil；
- 避免裁剪状态泄漏到后续批次。

---

## Tier 4 — GPU 优化

### 9. 3D GUI 性能 ⏸️

> 当前状态：原列表整体非短期。近期只推进 19 中范围更明确的
> Transform3D 缓存和 SceneView 按变化重绘。

已有：IBL 资源可通过 `Scene3DPassContext` 跨视图共享；Offscreen 分辨率可动态调整；
`MR3DSceneView` 已按场景签名和持续更新状态决定是否重绘。仍缺失视锥裁剪和
SceneNode3D 级局部 Dirty 更新。

优化方向：

- 3D SceneView 视锥裁剪；
- 静态 Mesh 和 Material 状态复用；
- GLTF 场景节点 Dirty 更新（当前每帧全量 update）；
- 3D 视图按实际变化决定是否重绘；
- 大量相同 Mesh 使用实例化绘制。

### 10. 纹理和 Buffer 上传 ⏸️

> 当前状态：非短期。除非目标设备 profiling 明确显示上传阻塞或带宽峰值问题。

已有：`PixelDataRecyclePool` 上传缓冲池；`shared_ptr` owner 传递避免 memcpy；`glTexSubImage2D` 局部更新；`VBODataRecyclePool` Fence 回收。缺失统计和高级 buffer 策略。

优化方向：

- 记录每帧上传字节数和上传耗时；
- 大纹理上传控制单帧 CPU 拷贝峰值；
- 动态 VBO 使用 orphaning、ring buffer 或 persistent mapping（当前使用 `glBufferData + GL_STATIC_DRAW`）；
- 支持局部纹理更新（`glTexSubImage2D` 已支持）。

---

## Tier 5 — 架构演进（需需求驱动）

### 11. 安全重排 ⏸️

> 当前状态：非短期。当前继续严格保持 painter's order。

`BatchBuilder` 当前 `preservePainterOrder = true` 硬编码。仅在 painter's order 严重限制批次规模时考虑。

需要引入：

- RenderItem 提供屏幕空间 bounds；
- 标记 opaque、reorderable 和 depth-independent；
- 判断元素是否重叠；
- 不跨越 Clip、Stencil、RenderTarget 和透明边界；
- 为重排前后结果建立图像回归测试。

### 12. 非 SSBO 路径减少 Draw Call ⏸️

> 当前状态：非短期。SSBO 仍是主要路径，标准路径只承担正确性回退。

已有：SSBO 路径（实例化绘制 + per-object SSBO 数据）；标准路径（逐对象 `material->apply()` + `drawVBO`）；SSBO fallback。SSBO 已是主要优化路径，标准路径作为回退足够。

仅在需要支持无 SSBO 能力的设备时考虑：

- 使用实例化顶点属性传递 per-object 数据；
- 按设备能力选择 UBO array、instance attribute 或逐项 Uniform；
- 合并兼容静态几何；
- 保留逐对象绘制作为正确性回退路径。

### 13. 2D/3D Pass 编排 ⏸️

> 当前状态：非短期。不提前引入 RenderGraph 或统一 transient resource 系统。

已有：`Engine::render()` 固定顺序（begin→2D update→lateUpdate→commit）；`MR3DSceneView` 自行管理 FBO 子 Pass 并恢复 GL 状态。缺失统一管理和统计。

优化方向：

- 明确 UI、3D、Offscreen 和 Debug 的执行顺序；
- 统一 RenderTarget、viewport、clear 和状态恢复；
- 减少不必要的 FBO 切换；
- 复用尺寸相同的 Offscreen RenderTarget；
- 记录每种 Pass 的 CPU/GPU 时间；
- 仅在出现跨 Pass 依赖和 transient resource 复用需求后评估 RenderGraph。

### 14. FrameState 与全局依赖 ⏸️

> 当前状态：非短期。仅在相关模块修改时局部收敛依赖，不进行全局重构。

已有：`FrameState` 携带相机、BatchManager、SSBOManager 等服务引用。缺失依赖注入和可测试性。

优化方向：

- 区分只读帧参数和可写统计；
- 减少 Component 从 FrameState 获取无关服务；
- 将高频服务通过明确上下文传递，减少 `GlobalObject::getInstance()` 全局查找（当前 23 处直接调用）；
- 收敛 `GlobalObject` 的使用范围；
- 明确 Engine、Platform、Window 和 RenderingThread 的销毁顺序；
- 提升模块可测试性。

### 15. Window 与 UI 根节点解耦 ✅

已完成：Window 不再继承 UIWidget，持有独立 `Root2D`（公共 UIWidget 轻量子类）
作为 2D UI 树根；平台窗口职责（surface、context、size、Present）与 UI 树职责
分离；`Engine::getRootWidget()` 是对外挂载入口。Window 本体随公共面收敛内部化
（公共面无 Window 类型，单窗口假设，见 ARCHITECTURE.md §11.2）。

剩余方向（⏸️ 需求驱动）：

- 一个 UI root 绑定不同输出目标（多窗口、嵌入式 Surface、离屏 UI）；
- 需求出现前不进行进一步重构。

---

## Tier 6 — 基础扎实 / 已完成

### 16. GPU 资源异步销毁 ✅

已有：删除操作通过 `Cmd_DeleteVBO/Cmd_DeleteTexture2D/Cmd_DeleteGPUProgram/Cmd_DeleteRenderTarget` 编码到 CommandBuffer；`PendingFrame` 携带 Fence，仅在 GPU 完成后回池；像素缓冲立即回收；`~RenderDeviceProxyBase()` 设置 quit 信号并 join 渲染线程。

剩余工作（⏸️ 非短期）：

- 明确各 GPU wrapper 的析构线程文档；
- 为反复创建和销毁 Texture、Buffer、Shader、RenderTarget 增加压力测试。

### 17. CommandBuffer 与环形帧槽 ✅

已有：3 槽环形缓冲（`kRingSize=3`），信号量控制主线程等待；`CommandBuffer` 固定 16MB，`push<T>()` 零分配编码；命令字节数可通过 `size()` 获取。

剩余工作（⏸️ 非短期）：

- 记录每帧命令字节数峰值、环形缓冲等待次数和等待时长；
- 为 CommandBuffer 溢出提供明确错误（当前依赖 debug assert）；
- 在编码端增加冗余状态过滤；
- 根据目标设备调整 frame slot 数量和容量。

### 18. 文本与字体 ⏸️

> 当前状态：非短期。现有文本 dirty 和连续合批满足当前目标，Atlas 分页等能力
> 在多语言大字符集场景出现明确压力后推进。

已实现：~~未变化文本避免重新生成几何~~（MRLabel 双层脏标记 + Mesh revision 跳过链）、~~相同字体和 Atlas 的文本连续合批~~（BatchCompatibilityKey 包含 Shader + Texture 集合）。

尚未实现：

- 字形 Atlas 分页和回收（当前单图集 + 扩容全量重建，O(已有字形数)）；
- 文本布局结果缓存（跨实例复用）；
- 降低多语言和动态字号导致的 Atlas 抖动（扩容丢弃旧纹理）；
- 记录字形上传、Atlas 命中和文本重建统计。

---

### 19. 近期高价值优化建议 🔜

以下方向来自 2026-08-04 对当前代码热路径的审查，目标是优先减少静态 GUI 的
持续消耗、3D 子场景重复计算和确定性的资源滞留。排序依据是收益、实现范围和
对现有架构的影响，不要求一次全部实施。

#### P0：Material 纹理所有权收敛 ✅

已完成：`Material::setTexture()` 只维护 `m_textureMap`，已删除没有读取点的
`m_textures` 和对应线性查找。替换同名 sampler 的纹理后，旧纹理不再被 Material
额外持有；`m_batchCompatibilityRevision` 的失效行为保持不变。

#### P0：按需渲染链路闭环 ✅

已完成：

- `RenderingThread` 使用原子 render request，并通过 consume 语义保证每个请求
  至少触发一帧；
- `Engine` 在输入派发后消费请求；无请求时跳过动画、Widget update、3D Pass、
  Batch、Present 和 rendered frame 计数；
- Windows 使用 `glfwWaitEventsTimeout()` 空闲等待，并可通过
  `glfwPostEmptyEvent()` 跨线程唤醒；其他平台使用短超时等待保证输入轮询；
- Window resize、异步资源更新和已有 Widget `requestRender()` 可以唤醒引擎；
- Tween 和 GLTF animation 在活跃期间持续请求下一帧，完成或暂停后停止请求；
- MR3DSceneView 的场景、相机适配、灯光、IBL 和 FBO 变化会请求重绘；
- `enableRequestRender=false` 时保持原有连续渲染行为。

`maxFrames` 继续只统计实际完成的渲染帧。

#### P1：Transform3D 世界矩阵版本缓存 ✅

已完成：

- `Transform3D` 使用 local revision 缓存 TRS 组合后的局部矩阵；
- 世界矩阵记录使用的 local matrix revision、父 `Transform3D` 身份和父 world
  revision，三者均未变化时直接复用；
- world revision 只在世界矩阵实际重建时递增；查询子节点前先刷新父节点，使祖先
  变化可以穿过尚未单独查询的中间节点惰性传播；
- 重挂接通过父对象身份变化失效，不需要 Widget 层级额外递归标脏；
- 显式 local matrix 与 TRS 模式共用 revision 机制，保持 GLTF 节点矩阵语义。

静态层级预热后不再执行局部矩阵组合或世界矩阵乘法；
节点变化后，仅在该节点及相关后代下次查询时重建。

#### P1：MR3DSceneView 按变化重绘 ✅

已完成：

- `MR3DSceneView` 缓存场景渲染签名，覆盖场景层级、可见性、Transform3D、
  MeshRenderer3D、Material、Texture、相机、灯光、IBL 和 FBO 尺寸；
- 仅在显式失效、签名变化或存在持续更新组件时执行 3D FBO Pass；
- 比较 3D Pass 前后签名；若懒创建 Shader、绑定 IBL 或准备 GPU 资源导致状态在
  Pass 内变化，则自动请求一个收敛帧，签名稳定后才复用 FBO；
- 静态帧直接复用上次 FBO color texture，只执行 2D composite；
- 局部 3D `FrameState` 改为成员复用，不再逐帧 `make_shared<FrameState>`；
- `OrbitCamera` 通过变化回调使 SceneView 失效，外部相机控制和 OrbitController
  均可准确触发重绘；
- `Component::requiresContinuousUpdate()` 统一描述持续更新需求，GLTF animation
  等持续更新组件活跃时保持连续 3D Pass，暂停、禁用或速度为零后自动静止。

静态 3D SceneView 首帧后不再清空 FBO、更新 frame UBO 或产生 3D Draw Call；
相机、灯光、模型、纹理或 FBO 尺寸变化会重新生成离屏内容。

#### P2：Material Uniform/UBO 提交收敛

当前 `Material::apply()` 每次绘制都会遍历多个 `unordered_map`，并以 uniform
名称字符串编码命令。现有 `m_uniformRevision` 已经把普通 uniform 与合批状态
分离，可作为后续刷新机制的失效依据。

建议分阶段推进，现阶段不立即修改上传模式：

- 先以 `m_uniformRevision` 缓存已打包的 Material uniform payload；
- Shader/Material 常量迁移到 Material UBO；
- model 等逐对象数据使用 per-draw UBO ring 或现有 SSBO；
- texture/sampler 状态继续独立管理，不与普通 uniform dirty 混用；
- 保留逐项 uniform 路径作为兼容性回退。

验收：Material uniform 未变化时不重复构建字符串命令或上传常量数据；动态
model 数据仍能逐帧更新；SSBO 与标准路径画面一致。

#### P2：SSBO 实例数据打包去字符串查找

> 说明：本项编号属于 19 的性能优化清单，与 [SSBO_LAYOUT_AUTOMATION.md](SSBO_LAYOUT_AUTOMATION.md) 的
> 分阶段编号（P0~P4）是两套不同体系，不冲突。

SSBO 打包已按 [SSBO_LAYOUT_AUTOMATION.md](SSBO_LAYOUT_AUTOMATION.md) 的 P0~P3 重构（2026-08-05）：注册表 +
声明式字段绑定（`SSBOFieldBinding`）+ 类型安全的 Material 读取接口
（`tryGetFloat` / `getVector4Or` / `tryGetVectorComponent`）+ 首次使用时的 shader
reflection 布局校验。但每个实例仍通过 `materialProperty` 字符串查询 Material 的
map，并经由 `std::function` 间接调用 `writeSSBOField`。大量 Widget 时，这部分
可能成为 CPU 热点，但应先使用固定场景确认占比。

建议（当前状态）：

- 为内建 shader 使用类型化 instance payload 或稳定字段句柄；—— ⏸️ 未实施，
  现有 `SSBOFieldBinding` 为声明式描述，运行时仍按字符串查找；
- Material revision 未变化时复用已解析的 per-instance 常量；—— ⏸️ 未实施；
- 保留 Transform matrix 等真正逐帧变化的数据直接写入；—— ✅ 已随字段绑定落地
  （`makeWorldMatrixField` 直接写入世界矩阵）；
- 评估 `ShaderStorageBuffer` 数据容量复用，避免每批每帧重复获取和 resize DTO；——
  ⏸️ 未实施。

验收：仅在 profiling 显示 SSBO 填充占用显著 CPU 时间后实施；优化后比较相同
RenderItem 数量下的 collection/pack 时间、命令字节数和内存峰值。

近期不建议推进 SoA、RenderGraph、安全重排、Window/UI root 解耦、非 SSBO
复杂实例化和字体 Atlas 分页。这些方向保留在前述非短期清单中，由真实需求和
profiling 数据触发。

---

## 优化决策标准

任何新增系统或抽象应至少满足以下条件之一：

1. 解决已经出现的正确性问题；
2. 显著降低 CPU 帧时间；
3. 显著降低 Draw Call 或 GPU 时间；
4. 显著降低内存峰值或分配次数；
5. 解决明确的平台兼容问题；
6. 提高可测试性且不会增加热路径负担。

优化前应先建立基线，优化后应使用相同场景验证：

```text
CPU frame time
GPU frame time
Draw Call
Batch count
Command bytes
Upload bytes
Memory peak
Visual correctness
```
如果复杂方案不能在真实目标场景中产生可测量收益，应优先保留简单实现。
