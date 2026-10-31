# MorrowUI 编辑器与 Godot 2D 编辑器的功能差距分析

> 状态:规划文档,不改动任何代码。结论基于 2026-10-31 的 `dev` 分支代码核查(文件:行号均以当日工作区为准)。
>
> **状态更新(2026-10-31,同日实施)**:阶段一(编辑手感)已全部实施完成并通过 MSYS2 clang64 构建,详见第 6 节的阶段一标记与各小节"已实施"注记。5.4 中颜色选择器/滑块/枚举下拉三项控件尚未做(属性描述符表已就位,为它们铺平了道路),仍计入待办。
>
> 关联文档:
> - `docs/road_map/GODOT_2D_COMPONENT_GAP.md` —— 运行时**组件**层面对照 Godot 2D 的缺口(本文不重复其内容);
> - `docs/road_map/EDITOR_DESIGN_PROPOSAL.md` —— 编辑器整体架构设计提案(本文在其之上做功能级差距细化);
> - `docs/ARCHITECTURE.md` —— 引擎与编辑器架构上下文。

## 1. 文档目的

回答两个问题:

1. MorrowEditor 目前**已经有什么**(以代码为准,不凭印象);
2. 相比 **Godot 编辑器的 2D 部分**(2D 视口 + Control/UI 编辑 + 相关编辑器基础设施),还缺什么、按什么优先级补、落点在哪些文件。

每个差距项给出:Godot 的行为 → MorrowUI 现状(代码证据)→ 实现建议(落点文件)→ 优先级(P0/P1/P2)与粗估工作量。

## 2. 对比范围与前提

- 对比对象是 **Godot 4.x 的 2D 编辑器**:2D 视口编辑、Control/CanvasItem 编辑、Inspector、场景树、动画编辑器、主题系统、资源导入、多分辨率预览等;
- MorrowUI 是 HMI/UI 引擎,因此 Godot 的 **TileMap、2D 物理、Skeleton2D/Bone2D、Navigation2D** 等游戏向能力不纳入差距(见第 7 节);
- 脚本系统遵循 `EDITOR_DESIGN_PROPOSAL.md` 的前提:逻辑语言是 C++,走"改代码 → 重新编译 → 重启预览"工作流,第一阶段不做运行时脚本,Godot 的脚本编辑器/调试器只作为远期参考;
- 工作量为**粗估人日**(单人,含自测,不含跨平台适配)。

## 3. 现状总览:MorrowEditor 已具备的能力

编辑器约 9,900 行,无第三方 UI 框架,全部用本库控件自绘。面板布局为:顶部工具栏 + 左 Scene 树 + 中 Viewport + 右 Inspector + 底部 FileSystem/Output/Build。

| 能力域 | 已实现内容 | 证据 |
|---|---|---|
| 视口点选 | 左键命中检测(逆序取最上层),Ctrl 加选 | `editor/panels/ViewportPanel.cpp:164-179,308` |
| 选中反馈 | 橙色边框(随缩放补偿线宽)+ 8 个圆形手柄 | `ViewportPanel.cpp:57-104` |
| 8 向缩放 | `ResizeHandle` 8 方向,命中半径检测,拖拽改 position+size(最小 1px) | `ViewportPanel.cpp:200-219,335-373` |
| 拖拽移动 | 位移除以 zoom;多选整体移动 | `ViewportPanel.cpp:374-384`;`editor/scene/EditorSession.cpp:104-120` |
| 视口导航 | 滚轮缩放(0.2~5.0,光标锚定),中键平移 | `ViewportPanel.cpp:264-287` |
| 属性面板 | 固定集(position/rotation/scale/size/visible 等)+ 节点自带属性;bool→复选框,Vector/Color→分量输入,数值校验;资产引用选择器;Material 资产可编辑 shader 与属性;多选 `<mixed>` 占位 + 批量写入;资产拖放指派 | `editor/panels/InspectorPanel.cpp:281-452,524-531`;`editor/EditorShell.cpp:445-537` |
| 场景格式 | 类 INI 文本格式:`[node]` + `property = 值`,支持 Vector/Color/Rect/Enum/`resource()`/`sub_resource()` 引用 | `editor/scene/SceneDocument.cpp:173-342` |
| 场景树 | 行内重命名、拖拽改父级(环检测)、右键菜单(Add/Rename/Delete) | `editor/panels/SceneTreePanel.cpp:151-216`;`EditorShell.cpp:246-258` |
| 新建节点 | 搜索 + 分类树 + 描述对话框,45+ 类型 6 分类 | `editor/ui/CreateNodeDialog.cpp`;`editor/scene/NodeTypeCatalog.cpp:44-104` |
| 撤销/重做 | 命令模式,7 种命令,拖拽/连改自动合并 | `editor/commands/CommandHistory.cpp:213-282` |
| 资源管理 | 文件树/网格切换、过滤、排序、缩略图(LRU)、监视器轮询、`.import` 元数据、右键新建 Material/Shader/文件夹 | `editor/panels/FileSystemPanel.cpp`;`editor/assets/AssetDatabase.cpp:49-67,226-288` |
| 构建/运行 | Windows CreateProcess + cmake 配置/构建/运行,Job Object 强杀,诊断正则解析,BuildPanel 状态机,运行前自动存场景 | `editor/build/BuildQueue.cpp:77-270`;`editor/panels/BuildPanel.cpp:26-69` |
| 界面布局 | Dock 比例 + Tab 顺序持久化(`.morrow/editor.layout` v3),Tab 跨区拖放 | `editor/layout/EditorLayoutController.cpp:31-153` |

一句话评价:**"面板框架 + 单场景静态编辑"已成型,"编辑手感、布局表达、动画、主题、真实导入"是五大空洞**。

## 4. 差距总览表

| # | 领域 | 与 Godot 2D 的差距 | 优先级 | 粗估 | 状态 |
|---|---|---|---|---|---|
| 1 | 视口编辑手感 | 旋转 gizmo、方向键微调、框选、锁定、F 聚焦、点击穿透选择 | **P0** | 5~7 人日 | ✅ 已实施(除 Alt 穿透选择) |
| 2 | 吸附与参考线 | 网格吸附、智能对齐线、标尺、可拖参考线(钩子已有死代码) | **P0** | 6~8 人日 | ✅ 已实施 |
| 3 | 复制粘贴 | Ctrl+C/V/D、右键菜单(副本命令已写好未接 UI) | **P0** | 2~3 人日 | ✅ 已实施(含 Ctrl+X) |
| 4 | Inspector 控件 | 颜色选择器、滑块、枚举下拉、重置默认、属性搜索、折叠记忆 | **P0** | 8~10 人日 | ◐ 部分(重置/搜索/折叠+schema 已做;颜色选择器/滑块/枚举待做) |
| 5 | 锚点与自适应布局 | anchors/margins/pivot/grow 全套缺失(最大结构差距) | **P0(分阶段)** | 20~30 人日 | 未开始 |
| 6 | 节点属性覆盖面 | 45+ 控件只有 3 个能在编辑器改自有属性 | **P1** | 10~15 人日 | 未开始(schema 表已就位,扩表即扩覆盖) |
| 7 | 多选对齐/分布 | 对齐左右上下/居中、等距分布工具栏 | P1 | 2~3 人日 | 未开始 |
| 8 | 多场景/实例化 | 多文档 Tab、场景嵌套实例化(instance 子场景) | P1 | 8~12 人日 | 未开始 |
| 9 | 主题与样式 | Theme/StyleBox/九宫格/覆盖体系缺失 | P1 | 15~20 人日 | 未开始 |
| 10 | 动画时间线 | 无关键帧/时间线/dope sheet(库已有 Tween) | P1 | 20~30 人日 | 未开始 |
| 11 | 分辨率/安全区预览 | 设备预设、安全区叠加、横竖屏(车机刚需) | P1 | 4~6 人日 | 未开始 |
| 12 | 真实资源导入 | 目前导入=逐字节拷贝;Basis/字体/图集转换未做 | P1 | 10~15 人日 | 未开始 |
| 13 | 日志与诊断 | Output 仅 6 行上限、无过滤、无跳转 | P1 | 2~3 人日 | 未开始 |
| 14 | 远程调试 | 运行中场景树/属性检视(Godot Remote) | P2 | 8~12 人日 | 未开始 |
| 15 | 编辑器基础设施 | 命令面板、快捷键自定义、编辑器设置、UTF-8、i18n | P2 | 6~10 人日 | 未开始 |

## 5. 分领域差距与建议

### 5.1 视口编辑手感(P0)

| Godot 行为 | MorrowUI 现状 | 差距 |
|---|---|---|
| 工具栏 Select/Move/Rotate(/Scale) 模式切换(Q/W/E/R) | 无模式概念,移动与 8 向缩放常驻 | 缺**旋转 gizmo**;rotation 只能 Inspector 数值改(`SceneInstantiator.cpp:296-303`) |
| 方向键微调,Shift+方向键大步进(= 网格步长) | 键盘只处理 Esc/F3/Delete/Enter/Ctrl+S/Z/Y(`EditorShell.cpp:627-703`) | 缺方向键微调 |
| 拖框橡皮筋多选(rubber band) | 只有点选 + Ctrl 加选 | 缺框选 |
| Lock/Unlock(锁定后在视口不可选中,场景树仍可选)、Group | 无 | 缺锁定/分组 |
| F / Shift+F 聚焦选中节点,使视口居中缩放 | 无 | 缺聚焦 |
| Alt+右键(或点击重叠时)弹出该位置节点列表选择 | 无,逆序取最上层(`ViewportPanel.cpp:164-179`) | 缺穿透选择 |
| Ctrl+拖拽临时禁用吸附 | 无吸附,自然无 | 随 5.2 一起做 |

**实现落点:**

- 旋转 gizmo:在 `ViewportPanel` 的手柄体系上新增第 9 个手柄(控件上方圆形,拖拽绕 pivot 计算角度),写入 `SetNodeRectCommand` 或新增 `rotation` 合并命令;多选时对各自 pivot 旋转。世界矩阵已含旋转(`ViewportPanel.cpp:107-158`),渲染侧无障碍;
- 方向键微调:扩展 `EditorShell::handleKeyEvent`,方向键对选中集发 `SetNodeRectCommand`(可合并),Shift 步进取网格步长(依赖 5.2);
- 框选:`ViewportPanel` 左键点空白进入 rubber band 绘制(用 previewCanvas 画半透明矩形),抬键时与各节点世界矩形求交;
- 锁定/分组:节点文档加 `locked`/`grouped` 标志位(场景格式新增 property 即可,`SceneDocument` 读写天然支持),命中检测跳过 locked;场景树行画锁图标;
- F 聚焦:把视口 pan/zoom 目标设为选中节点并集矩形(现有滚轮缩放已支持任意 zoom 值,复用即可)。

### 5.2 网格、吸附、标尺与参考线(P0)

现状:**完全缺失,且留有死代码**——`ViewportPanel::refreshGuides()` 只移除 previewCanvas/previewGrid 从不创建(`ViewportPanel.cpp:221-233`),`gizmoSnap` 字段声明后从未使用(`editor/scene/SceneEditorModel.h:30`)。说明原设计预留了位置但未实施。

| Godot 行为 | 建议的 MorrowUI 对应 |
|---|---|
| 网格吸附(Snap to Grid)+ 可配置步长 | 视口工具栏:吸附开关 + 设置弹出(grid step 默认 10px);拖拽/缩放/微调时对 position/size 取模吸附 |
| 智能对齐线(Smart Snap):吸附到其他节点边缘/中线/顶点,显示紫色对齐线 | 拖拽时收集同父与相邻节点的边缘/中线候选,差值 < 阈值(约 6px/zoom)时吸附并画线(previewCanvas) |
| 标尺(Rulers):视口上/左边缘像素刻度 | 视口绘制层画刻度,数值随 pan/zoom 换算 |
| 参考线(Guides):从标尺拖出,橙色线,随场景保存;Clear Guides | `refreshGuides()` 补上创建逻辑;参考线存到 `.scene` 文件新增 `[guides]` 段(格式向后兼容:无段不报错) |
| Snap Relative / 旋转步进 / 缩放步进 | 二期再加,先做位移吸附 |

**建议顺序:**网格吸附(最小可用)→ 参考线 → 标尺 → 智能对齐线(工作量最大,体验收益也最大)。

### 5.3 锚点与自适应布局(P0,最大结构差距)

这是与 Godot Control 体系的**根本性差距**,不只是编辑器问题:

- 现状:布局表达只有 `position + size`,世界矩形 = 父级 position 累加(`SceneEditorModel.cpp:161-189`);HBox/VBox/Margin 等容器虽存在于库中(`include/morrow/layout/`),但**编辑器侧没有锚点/边距/枢轴概念**,控件不随画布尺寸自适应;
- Godot:Control 有 anchors(0~1 四边锚点)+ offsets(margins)+ pivot_offset + grow 方向 + size_flags(expand/fill/shrink),编辑器提供锚点预设下拉(Full Rect / Center / Top Left…)、视口内锚点手柄拖拽、容器选中时的间距手柄;配合容器实现任意分辨率自适应;
- 车机场景多分辨率/横竖屏是刚需,没有这套系统,"预览不同分辨率"也无从谈起(见 5.11)。

**分阶段建议:**

1. **阶段 A(运行时,先行)**:Widget/Transform 增加 anchors + offsets 计算:父尺寸变化时按锚点重算自身 rect;先支持非容器父级下的锚点布局(容器内仍由容器管);
2. **阶段 B(编辑器)**:
   - Inspector 新增 Layout 分组:锚点预设下拉(枚举控件,依赖 5.4)+ 四边 offset 数值;
   - 视口选中节点画锚点指示(父矩形内绿色锚点标记,Godot 风格),锚点预设按钮放视口工具栏;
   - `SceneDocument` 序列化 anchors/offsets(纯新增 property,格式兼容);
3. **阶段 C**:容器属性(间距、对齐、margin)进 Inspector,选中容器时视口显示间距 gizmo(Godot 选中 HBox 会显示间距手柄)。

**注意**:`GridContainer` 目前是空壳类(`include/morrow/layout/GridContainer.h:9-14`),若要做布局系统增强,建议库侧先补齐该容器。

### 5.4 Inspector 属性编辑(P0)

现状痛点:Color 是 4 个文本框(`InspectorPanel.cpp:373-419`),无数值滑块,无枚举下拉(只有资产选择菜单)。

| Godot 行为 | MorrowUI 现状 | 建议 |
|---|---|---|
| 颜色选择器弹出(HSV/色板/十六进制/吸管) | 4 个 RGBA 文本框 | 新建 `MRColorPicker` 弹窗(复用 `MRPopup` 体系):SV 方块 + 色相条 + RGBA/HEX 输入 + 最近使用色板;库侧新增控件,编辑器与业务都能用 |
| Range 属性显示滑块 + 数值(可设 min/max/step) | 纯文本输入 + 数值校验 | 属性 schema 增加 min/max/step 元数据,渲染 `MRSlider`+`MRLineEdit` 组合 |
| 枚举属性下拉(int with enum hints) | 无 | 复用 `MROptionButton` 做属性下拉;`SceneDocument` 的 Enum 值格式已支持(`SceneDocument.cpp:68-79`) |
| 属性值"重置为默认"按钮 | 无 | 属性 schema 记录默认值,偏离时显示 ↺ 按钮,点击发 SetNodePropertyCommand |
| 属性搜索过滤 / 分类折叠(记忆状态) | 固定 4 组分组,无折叠 | 折叠状态存 `.morrow/editor.layout` 或独立 UI 状态文件 |
| 属性右键:复制/粘贴属性值 | 无 | `applyValue` 已支持批量写,补剪贴板序列化即可 |
| Pin Inspector(切换选中不换面板) | 无 | 二期 |
| 属性旁钥匙按钮(插关键帧,配合 AnimationPlayer) | 无动画系统 | 随 5.8 一起 |

**属性 schema 是核心改造点**:当前 schema 由 `SceneEditorModel.cpp:27-45` 硬编码返回,建议演化为"节点类型注册属性描述符表"(名称/类型/范围/枚举值/默认值/分类),Inspector 与 `SceneInstantiator` 共用同一张表,一次性解决 5.4 与 5.11 两个问题。

### 5.5 场景树与多文档编辑(P1)

| Godot 行为 | MorrowUI 现状 | 差距与建议 |
|---|---|---|
| 多场景多 Tab 编辑、最近文件 | 单场景单文档,`--scene` 参数指定 | 视口区支持多 Tab:EditorSession 持有文档集合,切换即换 session;构建运行用当前激活场景 |
| 场景实例化:instance 子场景为节点,引用而非拷贝 | 无(只有平铺节点树) | `.scene` 已有 `external_resource`,可扩展 `instance_node path="xxx.scene"`;实例节点只读、可"打散"(detach 为副本);编辑器打开子场景新 Tab。这是做大型 UI(多页面复用)的必要能力 |
| 远程树(Debugger→Remote,运行时检视) | 无 | P2,见 5.14 |
| 场景树过滤、批量折叠/展开 | 无 | 小改进,顺手做 |
| Save Branch as Scene | 无 | 依赖实例化做完后自然获得 |

### 5.6 复制/粘贴/副本(P0,成本最低收益最快)

`DuplicateNodeCommand` 和 `EditorSession::duplicateNode` **已经写好但没有任何调用点**(grep 全编辑器无引用,`CommandHistory.h:126-138`、`EditorSession.cpp:144-149`)。建议:

- Ctrl+C/X/V + Ctrl+D 快捷键(`EditorShell::handleKeyEvent`);
- 场景树右键菜单加 Cut/Copy/Paste/Duplicate 项;
- 跨场景粘贴:剪贴板存节点子树序列化文本(直接复用 `SceneDocument` 的 writer 输出),粘贴时重新分配 id;
- 粘贴位置偏移(8,8)避免完全重叠。

粗估 2~3 人日,是全表性价比最高的一项。

### 5.7 主题与样式系统(P1)

现状:**无 Theme/StyleBox 概念**(全库 grep 无匹配);控件外观由各控件 setter 硬编码;场景格式里虽有 `[sub_resource type="Style"]` 但只对 MRButton 生效且仅 `background_color`/`corner_radius` 两个键(`SceneInstantiator.cpp:428-453`),Inspector 无 Style 编辑 UI。

Godot 对照与建议:

| Godot | 建议的 MorrowUI 路线 |
|---|---|
| Theme 资源(项目级默认主题 + 类型变体) | 新增 `.theme` 资产:类型→属性表(颜色/常量/字体/字号/圆角/九宫格图),项目 `.morrow` 文件引用默认主题 |
| StyleBox(Flat/Texture 九宫格/Line) | Style 对象两种:FlatStyle(色+圆角+边框)+ NinePatchStyle(纹理 + 九宫格 inset);**九宫格是 UI 皮肤刚需**,对应 `GODOT_2D_COMPONENT_GAP.md` 里列的缺口 |
| 控件级 theme_override | 场景 property 直接覆盖主题键(现有 property 机制可表达) |
| Theme Editor 面板 | 新 Dock 面板:类型列表 + 属性表 + 实时预览控件农场;一期可先做"主题资产在 Inspector 可编辑 + 视口实时生效",面板二期 |

主题系统与 5.11(属性覆盖面)有耦合:建议先做属性描述符表,再做主题。

### 5.8 动画系统(P1,工作量最大的单项)

现状:编辑器是**纯静态编辑器**,无时间线、无关键帧面板。库侧已有:Tween 补间(`include/morrow/helpers/Tween.h`)、帧动画 `MRFrameAnimation`、GLTF 关键帧控制器(3D 侧)。特效组件(FlowingLight/Bounce/Gears 系)可摆进场景但参数不可调。

Godot 的 AnimationPlayer 是"属性关键帧 + dope sheet + 曲线 + 方法调用轨"的完整体系。建议按最小可用集分两期:

- **一期(最小时间线)**:
  - 新增 `MRAnimationPlayer` 节点类型 + `.scene` 内动画数据(`[sub_resource type="Animation"]`:轨道 = 节点属性路径 + 时间/值采样点,线性/缓动插值);
  - 编辑器底部新增 Animation Dock:时间轴、播放/暂停/停止、选中节点属性关键帧化(K 按钮)、dope sheet 网格(轨道×时间)、关键帧拖动;
  - 运行时播放:复用 Tween 插值引擎采样属性写入;
  - 播放预览:编辑器内实时循环播放(不进 Build 流程);
- **二期**:曲线编辑(贝塞尔)、动画事件轨(回调帧)、多动画切换/混合。

**切入口建议**:属性描述符表(5.4)先行,因为"哪些属性可关键帧化"由它定义。

### 5.9 资源导入与资产管理(P1)

现状:导入是 **Phase 1 占位**——artifact 只是源文件逐字节拷贝到 `.morrow/imported/<assetId>/<platform>/`,无任何转换(`editor/assets/ImportQueue.cpp:76-95` 注释明示)。资产识别支持 png/jpg/basis/otf/gltf/glsl/mat/wav 等(`AssetDatabase.cpp:49-67`)。

| Godot 行为 | 建议的 MorrowUI 路线 |
|---|---|
| 纹理导入:VRAM 压缩、mipmap、preset | 对接工具链:PNG/JPG → **Basis/KTX2 转码**(库已支持 basis/ktx2 魔数嗅探,见 ARCHITECTURE.md 装配章节),`.import` 里记录 preset;离线调用 basisu 工具进程 |
| 字体导入:烘焙参数 | SDF 字形图集的字体预烘焙参数化(点阵尺寸/spread/字符集);字符集配置对车机(中文大字表)尤其重要 |
| 图集:AtlasTexture 引用 | 库已有 TextureAtlas + 图集生成脚本(tools/),编辑器缺 UI:新建图集资产 → 拖入纹理 → 生成 .atlas + 预览;Inspector 引用图集 region |
| 音频导入 | 格式转码暂缓,先保持拷贝 |
| 重导入/批量重导入 | `.import` 已有元数据与哈希,补"右键→Reimport" |

优先级:**纹理转码 > 图集 UI > 字体烘焙**。

### 5.10 多分辨率与安全区预览(P1,车机刚需)

现状:视口随面板尺寸自适应(`ViewportPanel.cpp:235-248`),无设备预设、无安全区、无安全区叠加;分辨率只影响运行器窗口(`SceneRuntime.cpp:107-108`)。

建议:

- 视口工具栏新增预览下拉:项目基准分辨率 + 常见车机/手机预设 + 自定义;选中后视口内画**固定分辨率画布框**(居中,画布外压暗),实时演示锚点布局效果(依赖 5.3);
- 横竖屏切换按钮(宽高对调);
- **安全区叠加**:按预设(刘海/挖孔/圆角仪表造型)画半红叠加层,并可勾选"将安全区作为锚点参考";`ProjectSettings` 已有 platform 字段可挂安全区配置;
- 依赖关系:无锚点系统时此功能只能"看",有锚点后才能"验",故排在 5.3 之后。

### 5.11 节点属性覆盖面(P1,质量型差距)

现状对比刺眼:可创建 45+ 种节点(`NodeTypeCatalog.cpp:44-104`),但编辑器真正能编辑自有属性的只有 **MRButton/MRLabel(text/font_size)和 MRImage(texture_asset)**,其余控件属性改动会报 "unsupported property"(`SceneInstantiator.cpp:379-492`)。也就是说 Slider 不能调范围、ProgressBar 不能调值、HBox 不能调间距、特效组件参数全部不可调。

建议:

1. 建立"节点类型→可编辑属性"注册表(属性描述符:名称/类型/范围/默认值),Inspector 渲染、场景序列化、SceneInstantiator 应用三处共用;
2. 按高频排序补齐:容器类(间距/对齐/margin)→ 文本类(对齐/自动换行/富文本)→ 滑块/进度/SpinBox(范围/步进/当前值)→ 滚动(滚动条策略)→ 特效组件参数;
3. 未知属性告警降级:未注册属性不再报错,原样保留序列化(向前兼容手工编辑场景文件)。

### 5.12 日志、诊断与调试(P1~P2)

现状:Output 面板只读且**仅保留最近 6 行**(`OutputPanel.cpp:16-22`),Build 与 Output 共用行缓冲;诊断解析已有(gcc 风格正则,`BuildQueue.cpp:218-270`)。

建议(P1,2~3 人日):行数上限提升到千行级环形缓冲、按级别着色(Error/Warning/Info)、文本过滤框、Clear/复制按钮、诊断行点击定位(文件:行已有解析结果,补 UI 跳转——跳内部资源编辑器或外部编辑器)。

远期(P2):运行时日志通过 SceneRuntime 回传编辑器(进程间管道已存在,stdout 轮询即用)。

### 5.13 远程调试(P2)

Godot 运行时可在 Debugger 检视远程场景树并修改属性。MorrowUI 建议:SceneRuntime 加本地 IPC(命名管道/localhost socket),编辑器连接后:远程树浏览(只读)→ 节点属性实时读 → 可写属性下发(调试期热改属性,不热改代码,与 C++ 前提不冲突)。粗估 8~12 人日。

### 5.14 编辑器基础设施(P2)

| Godot 行为 | MorrowUI 现状 | 建议 |
|---|---|---|
| 命令面板(Ctrl+P,4.2+) | 无 | 二期;命令注册表复用快捷键映射 |
| 快捷键自定义(Editor Settings) | 快捷键硬编码(`EditorShell.cpp:627-703`) | 先做"快捷键清单展示",自定义后置 |
| 编辑器设置 UI | 只有 Configure(项目设置) | 字体大小/主题色/外部编辑器路径等常改项 |
| 多语言编辑器界面 | 英文硬编码;`wide()` 逐字符扩展非 UTF-8(`InspectorPanel.cpp:60-67`) | **先修 UTF-8/中文渲染正确性(P1.5,车机中文必经)**,再谈界面翻译 |
| 翻译/伪本地化预览 | 无 | 场景文本抽表 → CSV/po;伪本地化(加长+重音)测布局溢出,车机多语言刚需 |

### 5.15 未尽小项清单(顺手即可做)

- 场景树行显示可见性眼睛图标(点击切换 visible,场景文件已支持);
- `NodeTypeCatalog` 补漏:`MRSegmentedButton` 等库有但目录没列的类型;同时明确 GridContainer 空壳待补;
- FileSystem 面板双击 `.scene` 直接在当前 Tab 打开(配合 5.5 多 Tab);
- 视口右下角显示当前 zoom 百分比与点击坐标状态栏;
- 新建节点对话框记住上次分类。

## 6. 优先级路线图

按"手感 → 布局 → 质量 → 资产/主题 → 动画 → 调试/生态"排序:

| 阶段 | 内容 | 条目 | 粗估合计 | 状态 |
|---|---|---|---|---|
| **一:编辑手感** | 让日常摆放控件达到 Godot 基线 | 5.6 复制粘贴(2~3d)→ 5.1 视口手感(5~7d)→ 5.2 吸附参考线(6~8d)→ 5.4 Inspector 控件升级 + 属性描述符表(8~10d) | **21~28 人日** | ✅ 已实施(2026-10-31;5.4 剩颜色选择器/滑块/枚举下拉) |
| **二:布局表达** | 多分辨率自适应的根本 | 5.3 锚点系统 A/B/C(20~30d)→ 5.10 分辨率/安全区预览(4~6d)→ 5.7 多选对齐分布(2~3d) | **26~39 人日** | 未开始 |
| **三:内容质量** | 属性与序列化覆盖面 | 5.11 属性覆盖面(10~15d)→ 5.12 日志诊断(2~3d)→ UTF-8 修复 | **12~18 人日** | 未开始 |
| **四:资产与主题** | 皮肤与管线 | 5.7 主题/StyleBox/九宫格(15~20d)→ 5.9 真实导入(10~15d)→ 5.5 多场景/实例化(8~12d) | **33~47 人日** | 未开始 |
| **五:动画** | 让 UI 动起来 | 5.8 动画一期(20~30d) | **20~30 人日** | 未开始 |
| **六:调试与生态** | 提效与全球化 | 5.13 远程调试(8~12d)→ 5.14 基础设施(6~10d) | **14~22 人日** | 未开始 |

### 阶段一实施记录(2026-10-31,dev 分支,已通过 MSYS2 clang64 构建、未提交)

- **5.6 复制/剪切/粘贴/副本**:`DuplicateNodeCommand` 升级为整棵子树克隆;新增 `AddNodesCommand`(批量添加)、`MacroCommand`(组合单步撤销)、`MoveSelectionCommand`(整组平移,多选拖拽/方向键共用,连续合并);`EditorSession` 增加 `copySelection/cutSelection/pasteClipboard/duplicateSelection`(粘贴根偏移 8,8、id 全量重映射);Ctrl+C/X/V/D 快捷键(新增 `TOUCH_KEY_V/X/D/F` 键码与 WGL 映射)与场景树右键菜单接入。
- **5.1 视口手感**:旋转 gizmo(第 9 手柄,绕选中中心,Shift 15° 步进);方向键微调(步长=吸附步长,Shift ×10,长按重复合并为单条撤销);F 聚焦(选中并集适配居中);橡皮筋框选(含 Ctrl 累加,空白点击清选);锁定/解锁(`locked` 属性 + 右键 Toggle Lock + 场景树 `[L]` 标记 + 视口命中跳过)。
- **5.2 吸附系统**:网格吸附(Snap 开关按钮 + Step 循环 1/2/5/10/20/50 + Ctrl 拖拽临时禁用 + 自适应网格显示);参考线(顶/左标尺拖出、拖动重定位、拖回标尺删除、`[guides]` 段随场景保存);标尺刻度(随 pan/zoom 重建);智能对齐线(与未选中/未锁定节点的边/中线及参考线匹配,6px 阈值,品红辅助线)。原 `refreshGuides()` 死代码与 `gizmoSnap` 未用字段被真正启用/取代。
- **5.4 schema + Inspector**:新增 `editor/scene/NodePropertySchema`(common/meshRenderer/per-type 三层描述符,Inspector 展示与默认值的单一事实源);Inspector 增加属性搜索过滤框(复用 legacy 编辑通道,Enter 生效)、分组折叠(可点击表头,会话内记忆)、偏离默认值行的 R 重置按钮;`locked` 进入属性面板。
- **附带修复**:`SceneInstantiator` 对 `locked` 元数据属性静默跳过;`EditorSession::clearGuides/markDirty`。
- **实施中发现并规避的坑**:`ViewportPanel::handlePointer` 内局部变量 `x/y` 遮蔽同名成员会诱发 clang 对成员表达式 `panel->getWorldSpaceAABB()` 的名称查找失败(报"no member"),已改名 `canvasX/canvasY` 并留注释。

依赖链:属性描述符表(阶段一)→ 属性覆盖面(阶段三)与关键帧化(阶段五)都靠它;锚点(阶段二)→ 分辨率预览(阶段二)与主题覆盖(阶段四)都受它约束;实例化(阶段四)建议在主题之前或并行,大型项目早受益。

## 7. 明确不建议做的部分

| Godot 能力 | 不做/缓做的理由 |
|---|---|
| TileMap/2D 物理/骨骼/导航 | 游戏 HMI 引擎无此需求,`GODOT_2D_COMPONENT_GAP.md` 同口径 |
| 内置脚本编辑器 + 断点调试器 | 与 `EDITOR_DESIGN_PROPOSAL.md` 的 C++ 工作流前提冲突;外部编辑器 + 构建面板已覆盖 |
| GDScript 插件生态 | 无脚本运行时,不做编辑器插件系统;扩展点用 C++ 编译期注册(节点/属性/资产类型目录已经是这个模式) |
| 导出/发布预设(publish) | 车机交付走客户构建链,MorrowEditor 只负责场景/资产产物 |
| 可视化着色器编辑器 | 已有 .glsl 资产 + 材质 Inspector,文本工作流够用 |

## 8. 与现有文档的关系

- `GODOT_2D_COMPONENT_GAP.md`:组件(Widget)层缺口 → 本文 5.11 的属性覆盖面与它互补(它是"缺哪些控件",本文是"已有控件在编辑器里调不了");
- `EDITOR_DESIGN_PROPOSAL.md`:其阶段划分中的属性编辑/资产引用/预览运行已基本落地,本文可视作其后续版差距验收单;
- `ARCHITECTURE.md`:编辑器豁免使用 `src/` 私有头的约定(155、174 行)在新增 ColorPicker/Timeline 等编辑器控件时继续适用——新控件应沉淀到 `include/morrow` 公共层。
