# MorrowUI Editor 设计建议

## 1. 文档目的

本文针对当前 MorrowUI 引擎补齐 Editor 短板，提出一套可以逐阶段落地的编辑器设计。

本文的核心前提是：

- 默认脚本/逻辑语言采用 C++；
- 接受 C++ 修改后需要重新编译、重新启动预览的工作流；
- 不以“保存即生效”或运行时热替换作为第一阶段目标；
- 编辑器复用现有运行时对象模型，不维护第二套平行的 Widget 或渲染世界；
- 优先解决场景搭建、属性编辑、资源引用、预览运行和问题定位。

本文是设计提案，不代表所有接口已经存在。带有“建议新增”的内容需要后续拆分为实现任务。

## 2. 结论摘要

建议把 Editor 做成一个独立的桌面工具进程，内部托管一个编辑器运行时实例：

```text
MorrowEditor.exe
  ├─ ProjectService       项目、配置、构建目录
  ├─ AssetService         资源扫描、导入、缩略图、依赖
  ├─ SceneService         场景文档、实例化、保存、加载
  ├─ ReflectionService    C++ 类型/属性注册表
  ├─ CommandService       编辑命令、撤销/重做
  ├─ BuildService         CMake 配置、编译、日志、产物
  ├─ Editor UI            层级、视口、检查器、资源浏览器、日志
  └─ Preview Host         编辑器内预览窗口或独立预览进程
```

推荐的第一版工作流：

```text
打开项目
  -> 加载 *.scene
  -> 根据类型注册表创建 Widget/Component
  -> 在 Inspector 修改属性
  -> 命令系统记录修改，视口立即刷新
  -> Ctrl+S 保存场景文档
  -> 修改 C++ 后点击 Build
  -> 编译成功后重新启动 Preview
```

这里的“立即刷新”仅指编辑器内对已支持的场景属性修改立即刷新；C++ 源码修改不承诺热编译，也不承诺保存源码后自动影响正在运行的进程。

## 3. 为什么采用这条路线

### 3.1 与当前引擎模型匹配

当前引擎是保留式对象模型。`Widget` 持有子节点和组件，`Component` 具有生命周期回调，`Transform`、`MeshRenderer`、`Material` 等对象在运行期间持续存在。编辑器最重要的工作不是重新发明一套 ECS 或 Render World，而是：

1. 用稳定的文档数据描述场景；
2. 将文档数据实例化为现有的 Widget/Component 对象；
3. 将用户操作转换为可撤销的对象属性命令；
4. 让现有 Engine 帧循环和渲染路径负责预览。

这样可以保持 `docs/ARCHITECTURE.md` 中的短渲染路径和保留式对象原则，不给运行时增加一套编辑器专用渲染链。

### 3.2 C++ 作为默认逻辑语言是可行的，但必须承认编译边界

C++ 适合当前项目的原因：

- 引擎本身以 C++/CMake 为中心；
- 类型安全、性能和调试工具链成熟；
- 可以直接访问现有 Widget、Component、Material 和渲染接口；
- 不需要为了 Editor 强行设计一套新的脚本 VM；
- 引擎和目标应用的部署逻辑可以继续由 CMake/toolchain 管理，Editor 本身只构建和运行在 Windows。

代价也很明确：

- 修改逻辑通常需要重新编译；
- 首次编译时间可能较长；
- C++ ABI、链接、头文件依赖和编译错误会成为用户体验的一部分；
- 不应在早期为了追求热更新引入不稳定的 DLL ABI 或复杂的动态反射系统。

因此 Editor 应把“编译和重新启动预览”做成一等工作流，而不是把它当成异常情况。

### 3.3 第一阶段不做源码级热重载

不建议第一阶段做以下方案：

- 每个用户项目都编译成可卸载 DLL；
- 运行时替换带有虚函数、`std::shared_ptr`、STL 容器和跨模块对象所有权的 C++ 类型；
- 在 Widget 树中直接替换 Component 的动态类型；
- 通过源码解析器自动生成完整 C++ 反射并依赖其保持 ABI 稳定；
- 用脚本解释器绕过 C++ 构建流程。

这些方案会把当前的对象所有权、线程、GPU 资源生命周期和平台 ABI 问题同时暴露出来。当前引擎已有 `RenderDeviceProxy`、渲染线程、GPU 资源延迟回收和多平台 toolchain，热替换边界必须等运行时模块边界稳定后再单独设计。

## 4. 产品边界

### 4.1 第一版必须解决的问题

- 创建、删除、复制、移动场景节点；
- 层级树查看和选择；
- Transform 属性编辑；
- 常用 Widget/Component 的添加和移除；
- 资源引用和路径校验；
- 场景保存、加载和格式版本；
- 撤销/重做；
- 编辑器视口预览；
- CMake 配置、构建、运行；
- 编译错误、运行时日志和对象 snapshot 查看；
- 项目内多个场景切换。

### 4.2 第一版明确不解决的问题

- 保存 C++ 源码后自动生效；
- 蓝图或节点式逻辑系统；
- 通用 3D 建模器；
- 复杂动画曲线编辑器；
- 多人实时协作；
- 通用插件市场；
- 任意 C++ 类型的无配置自动反射；
- 在 QNX 设备上直接提供完整桌面 Editor。

Editor 明确只支持 Windows。QNX 仍然可以作为引擎和目标应用平台，但其交叉编译、部署和日志回传不属于 Editor 当前设计与验收范围。

## 5. 总体架构

### 5.1 Editor 与 Runtime 的关系

建议将 Editor 分成两种运行状态：

```text
编辑模式：
Editor UI + In-Process Preview Runtime
  - 读取场景文档
  - 创建编辑对象
  - 属性修改后立即刷新
  - 允许选取和查看调试信息

运行模式：
Editor UI + Child Preview Process
  - 使用项目实际编译产物
  - 与编辑器内对象隔离
  - 支持启动参数、日志、退出码和崩溃诊断
```

第一阶段可以只实现进程内预览，第二阶段加入独立预览进程。长期建议保留独立预览进程，因为它能避免用户项目逻辑崩溃拖垮 Editor，也能更接近最终应用行为。

编辑器对象和预览对象不应共享所有权。编辑器保存的是文档，预览运行时是文档的一个实例化结果。

### 5.2 推荐目录结构

建议逐步形成以下目录：

```text
editor/
  Editor.cpp
  EditorApplication.h/.cpp
  EditorServices.h/.cpp
  ui/
    EditorShell.h/.cpp
    SceneTreePanel.h/.cpp
    InspectorPanel.h/.cpp
    AssetBrowserPanel.h/.cpp
    ViewportPanel.h/.cpp
    ConsolePanel.h/.cpp
  scene/
    SceneDocument.h/.cpp
    SceneSerializer.h/.cpp
    SceneInstantiator.h/.cpp
  reflection/
    TypeRegistry.h/.cpp
    PropertyDescriptor.h/.cpp
  commands/
    Command.h
    CommandHistory.h/.cpp
    SceneCommands.h/.cpp
  build/
    BuildService.h/.cpp
  preview/
    PreviewHost.h/.cpp
```

其中 `editor/` 只依赖公开的 `src/` API 和编辑器服务，不应把编辑器逻辑塞进 `src/core/Engine.cpp`。

### 5.3 Editor UI 是否使用 MorrowUI

建议分两个阶段处理：

1. **最小可用阶段**：允许 Editor 使用现有 MorrowUI 控件，快速验证场景树、属性面板和命令系统。
2. **稳定阶段**：如果当前控件缺少停靠布局、文本编辑、滚动、键盘快捷键、树控件和多窗口能力，再为 Editor 引入桌面 UI 适配层，但仍复用 MorrowUI 的场景/渲染核心。

不要为了证明 Editor 能运行而把编辑器 UI 与引擎运行时强耦合。`EditorApplication` 应负责服务组合，面板只依赖服务接口。

## 6. 场景文档模型

### 6.1 场景文档是持久化真相

场景直接采用 MorrowUI 自有的 UTF-8 文本格式，结构参考 Godot 的 `.tscn`，但不复用 Godot 语法和类型系统。推荐扩展名为：

```text
*.scene
```

不经过 JSON 过渡。原因是场景文件是开发者长期阅读、审查、版本控制和合并的核心文件，需要从第一版就具备清晰的资源、子资源、节点和属性边界。

推荐的第一版语法：

```text
[morrow_scene format=1]

[external_resource id="asset_001" type="Texture" path="assets/images/button.png"]

[sub_resource type="Style" id="style_001"]
property background_color = Color(0.12, 0.18, 0.28, 1.0)
property corner_radius = 6.0

[node id="node_0001" type="SceneNode" name="Main"]

[node id="node_0002" type="MRButton" parent="node_0001" name="StartButton"]
property text = "Start"
property position = Vector3(100.0, 100.0, 0.0)
property size = Vector2(100.0, 48.0)
property texture = resource("asset_001")
property style = sub_resource("style_001")
```

第一版格式至少需要支持以下结构：

```text
scene header
external resources
sub resources
nodes
node properties
node connections
```

格式设计原则：

- 每个节点有稳定的文档 ID；
- 类型名使用 `TypeRegistry` 中的稳定名字，不使用 C++ `typeid().name()`；
- 父子关系通过 `parent` 表达，避免依赖文本出现顺序推断；
- 外部资源通过 `resource("asset_id")` 引用；
- 内部子资源通过 `sub_resource("sub_resource_id")` 引用；
- 属性值使用明确的类型字面量，例如 `Vector2(...)`、`Color(...)`、`Enum(...)`；
- 文件带格式版本，加载器提供迁移器；
- 未知 section、属性和资源引用默认保留或给出明确警告，不能静默丢数据；
- 序列化器应尽量保持稳定排序和局部格式，避免每次保存产生无意义的大范围 diff；
- 文件允许保留注释，但 Editor 自动写回时不承诺保留所有注释位置；
- 文档解析和运行时对象实例化必须分离，不能让 parser 直接依赖 GPU 或窗口状态。

建议把格式解析器拆成三层：

```text
Lexer / Parser
  -> SceneDocument AST
  -> SceneInstantiator
  -> Widget / Component RuntimeInstance
```

`nlohmann/json.hpp` 仍可继续用于 Editor 布局、用户设置和部分导入器配置，但不再作为工程文件或场景文件的默认格式。

### 6.2 文档对象与运行时对象分离

建议引入两个明确概念：

```text
SceneDocument
  - 可序列化
  - 可比较
  - 可撤销
  - 不持有 GPU 资源和平台窗口

RuntimeInstance
  - Widget/Component 对象树
  - 参与 Engine 更新和渲染
  - 可以从 SceneDocument 重建
  - 不直接作为场景文件写入源
```

Inspector 修改时，可以先修改文档，再将变化应用到 RuntimeInstance；或者在第一阶段直接修改运行时对象，同时由命令对象记录旧值和新值。推荐前者作为长期方向，原因是保存、撤销、重载和预览重启都更容易保持一致。

第一阶段允许采用“运行时对象为主、命令记录文档变化”的过渡实现，但必须把序列化接口独立出来，避免以后只能从任意 C++ 对象状态反向猜测场景结构。

### 6.3 运行时生成对象的处理

C++ 代码可能在 `start()`、`update()` 或事件回调中动态创建 Widget。此类对象分为两种：

- **文档对象**：由场景文件创建，显示在层级树中，可以保存；
- **运行时对象**：由逻辑创建，不写回场景文件，默认标记为 Runtime Only。

对象注册表应提供来源标记：

```text
Document
Runtime
EditorPreview
```

这样 ObjectDB snapshot 和 Editor 层级树不会把临时对象误认为用户场景数据。

## 7. 类型与属性反射

### 7.1 不追求全自动 C++ 反射

C++ 本身不提供足够可靠的标准运行时反射。第一版建议采用显式注册：

```cpp
TypeRegistry::registerType<MRButton>("MRButton")
    .base<UIWidget>("UIWidget")
    .property("text", PropertyType::String,
              &MRButton::getText,
              &MRButton::setText)
    .property("enabled", PropertyType::Bool,
              &MRButton::isEnabled,
              &MRButton::setEnabled);
```

注册表只服务于 Editor、序列化和 Inspector，不应成为渲染热路径的依赖。

### 7.2 属性描述必须包含

- 稳定属性名；
- 显示名和分组；
- 属性类型；
- getter/setter 或专用读写器；
- 默认值；
- 可选范围、步长、单位；
- 是否可编辑；
- 是否影响布局；
- 是否需要重新创建资源；
- 是否支持多选编辑；
- 序列化迁移名。

建议第一版只支持以下类型：

```text
bool
int32
float
string
Vector2
Vector3
Vector4 / Color
Quaternion
Rect
enum
asset reference
node reference
```

不要在第一版把任意 STL 容器、回调、线程对象和 GPU 句柄暴露给 Inspector。

### 7.3 组件注册顺序

建议按以下顺序接入：

1. `Widget` / `UIWidget` / `SceneNode`；
2. `Transform` / `Transform3D`；
3. `MeshFilter` / `MeshRenderer` / `MeshRenderer3D`；
4. `MRImage` / `MRLabel` / `MRButton`；
5. 布局组件；
6. 3D 场景和动画组件；
7. 资源和材质高级属性。

每接入一类，配套一个可保存、加载、撤销和预览的最小测试场景。

## 8. 命令、撤销和修改语义

### 8.1 所有用户修改都走命令

Inspector、层级树和视口 Gizmo 都不能直接散落地修改对象。所有修改统一进入 `CommandHistory`：

```text
SetPropertyCommand
AddNodeCommand
RemoveNodeCommand
ReparentNodeCommand
DuplicateNodeCommand
AddComponentCommand
RemoveComponentCommand
AssignAssetCommand
```

每个命令至少包含：

- 目标节点 ID；
- 属性或关系；
- 修改前值；
- 修改后值；
- 执行和反执行逻辑；
- 合并键和时间戳。

连续拖动 Transform 时，应把同一属性的多个中间值合并成一个命令，否则撤销一次只退回一个像素，体验会很差。

### 8.2 保存状态

编辑器需要同时显示两种状态：

- `Document Dirty`：场景文档有未保存修改；
- `Build Dirty`：C++ 或构建配置有未编译修改。

保存场景不等于构建成功，构建成功也不等于当前预览已经重启。状态栏应明确显示：

```text
Scene: Modified / Saved
Code: Modified / Built
Preview: Stopped / Running / Outdated / Failed
```

这正是接受“没有保存即生效”之后，仍然可以保持清晰反馈的关键。

## 9. C++ 工作流设计

### 9.1 项目模型

项目根目录建议包含：

```text
MorrowUI.morrow
  scenes/
    main.scene
  assets/
  src/
  build/
  generated/
```

`MorrowUI.morrow` 是工程入口文件，建议采用 UTF-8、可版本控制的结构化文本格式，职责类似 Godot 的 `project.godot`。它至少包含：

- 项目名称；
- 默认场景；
- CMake 源目录；
- 构建目录；
- 构建配置；
- 预览目标；
- 资源根目录；
- 目标平台；
- 引擎版本/场景格式版本。

示例：

```text
[morrow_project format=1]

property name = "MorrowUI"
property default_scene = "scenes/main.scene"
property source_root = "src"
property asset_root = "assets"
property build_root = "build"
property preview_target = "MorrowPreview"
property platform = "windows"
property engine_version = "0.1"
```

也可以直接兼容当前仓库布局，让现有 `assets/`、`samples/` 和 CMake 工程先作为一种项目形态。

当前仓库的 Phase 0 测试工程位于：

```text
editor/morrow.gui/
  MorrowUI.morrow
  scenes/main.scene
```

这是一个独立的测试项目目录。测试工程使用自己的 `assets/` 和 `.import` 文件，不读取或写入仓库根目录的 `assets/`。根目录 `assets/` 只属于引擎仓库资源。

```text
editor/morrow.gui/assets/
  models/box/wood.png
  models/box/wood.png.import
```

Editor 的资源扫描根目录必须从 `MorrowUI.morrow` 的 `asset_root` 属性读取。禁止为了测试工程资源而在仓库根 `assets/` 下生成 `.import`、派生资源或其他 Editor 状态文件。当前实现已经按此规则扫描，不再硬编码仓库根 `assets/`。

### 9.2 BuildService

`BuildService` 建议只负责流程编排，不负责解析 C++：

```text
Configure
  -> cmake -S <source> -B <build>
Build
  -> cmake --build <build> --target <target>
Run
  -> 启动 preview executable
Stop
  -> 请求退出，超时后终止子进程
```

必须记录：

- 命令行和工作目录；
- stdout/stderr；
- 开始时间、结束时间和退出码；
- 编译诊断；
- 产物路径；
- 使用的配置和 toolchain。

编译错误应尽可能解析为文件、行、列和消息，并允许双击跳转到 IDE 或内置代码查看器。第一版可以只显示原始日志，但接口不要绑定某一种编译器格式。

### 9.3 预览启动策略

每次 Build 成功后，Preview 标记为 `Outdated`，用户可点击 `Restart Preview`。不建议默认在每次文件保存后自动构建，因为：

- C++ 保存频率高，容易触发大量无意义构建；
- 构建错误会打断场景编辑；
- 大型项目的增量构建也可能很慢；
- 用户需要明确知道何时使用了新代码。

推荐三个操作：

- `Build`：只编译，不启动；
- `Build & Run`：编译成功后重启预览；
- `Run Last Successful`：使用最近一次成功产物启动。

### 9.4 代码与场景的连接

C++ 逻辑应通过稳定的场景节点名、文档 ID 或显式节点路径查找对象。不要把编辑器生成的内存地址或容器下标写进 C++。

推荐提供：

```cpp
auto button = scene->findById("node_0002");
auto startButton = scene->findByPath("Main/StartButton");
```

其中文档 ID 适合稳定引用，节点路径适合人读和调试。重命名节点不应改变 ID；复制节点必须生成新 ID，并处理引用复制策略。

## 10. Editor 主要界面

### 10.1 默认布局

推荐采用常规工作台布局：

```text
┌──────────────┬─────────────────────────────┬──────────────┐
│ Scene Tree   │ Viewport                    │ Inspector    │
│              │                             │              │
├──────────────┴─────────────────────────────┴──────────────┤
│ Asset Browser / Console / Build Output / Profiler           │
└─────────────────────────────────────────────────────────────┘
```

第一版不需要复杂可编程停靠系统，但面板边界和服务接口应稳定，后续再增加布局保存。

### 10.2 Scene Tree

必须支持：

- 搜索和过滤；
- 单选、多选；
- 重命名；
- 拖拽重父级；
- 复制、删除；
- 可见性和锁定；
- 文档对象/运行时对象标识；
- 错误节点标识。

### 10.3 Inspector

必须支持：

- 按组件分组；
- 多选共同属性；
- 数字输入和拖动；
- 资源选择；
- Reset to Default；
- 属性错误提示；
- 修改预览；
- 撤销/重做。

属性面板不能直接依赖具体控件类型。它应消费 `PropertyDescriptor` 和通用 `PropertyValue`。

### 10.4 Viewport

第一版只需要：

- 2D 视口；
- 可选 3D 视口；
- 选择命中；
- 平移、缩放；
- Transform Gizmo；
- 网格和安全区域；
- 编辑对象边界框；
- Play/Pause/Stop；
- Preview 重启。

不要第一版就实现完整 3D 编辑器。当前引擎的核心优势是 UI 和 2D/3D 混合展示，先让视口可靠地编辑 `Transform`、图片、文本和按钮。

### 10.5 Console 与诊断

复用现有 Log 和 ObjectRegistry 能力，增加：

- 日志级别和来源过滤；
- 编译日志；
- 运行时错误；
- 当前选中对象的 DebugObjectId；
- snapshot 触发和查看；
- 当前帧 draw call、batch、命令字节数和 FPS。

编辑器应把“看见问题”作为核心功能，而不只是提供一个漂亮的场景树。

## 11. 资源系统建议

### 11.1 源资源和导入资源分离

场景资源系统应采用类似 Godot 的 sidecar import 文件。需要转换、压缩、预处理或生成派生数据的源资源，都在源文件旁边生成 `.import` 文件：

```text
assets/
  images/
    button.png
    button.png.import
  fonts/
    MorrowSansCN1.1-Regular.otf
    MorrowSansCN1.1-Regular.otf.import
  models/
    dashboard.glb
    dashboard.glb.import

.morrow/
  imported/
    asset_001/
    asset_002/
```

源文件是用户维护的内容，`.import` 是引擎生成和维护的导入元数据，`.morrow/imported/` 是可删除、可重建的派生资源缓存。三者的职责不能混在一个文件中。

`.import` 推荐使用容易审查的文本格式。第一版可以使用 INI 风格：

```ini
[import]
format = 1
asset_id = "asset_001"
importer = "morrow.texture"
source_hash = "..."
importer_version = 3

[options]
srgb = true
generate_mipmaps = false
compression = "basis"
max_size = 2048

[platform.windows]
artifact = ".morrow/imported/asset_001/windows/texture.bin"

[platform.qnx]
artifact = ".morrow/imported/asset_001/qnx/texture.bin"
```

如果导入设置需要嵌套结构，可以使用 JSON 作为 `.import` 文件内容；这不影响场景使用自有文本格式。关键是 `.import` 的语义，而不是强制所有元数据采用同一种语法。

资源服务负责：

- 扫描；
- 类型识别；
- 文件存在性检查；
- 修改时间和哈希；
- 创建和更新稳定 Asset ID；
- 读取和校验 `.import`；
- 调度导入器；
- 记录导入器版本和导入选项；
- 生成平台相关的派生资源；
- 缩略图；
- 资源依赖列表；
- 导入错误。

第一阶段不需要把所有资源打包成专有二进制格式，但应预留派生资源目录和导入器接口。

### 11.2 AssetId 和资源引用

Asset ID 应从第一版资源系统就存在，不要等资源重命名成为问题后再补：

```text
AssetId -> source path -> imported artifact
```

场景文件中使用 Asset ID 引用：

```text
property texture = resource("asset_001")
```

`.import` 中同时保留源路径、Asset ID、源文件哈希和派生资源路径。这样：

- 资源移动或重命名时，Asset ID 不变；
- 场景不需要全工程替换绝对路径；
- 资源依赖关系可以被扫描；
- 多平台派生资源可以共用一个源资源；
- 导入选项变化时可以判断是否需要重新导入。

资源导入器应至少区分：

```text
source file
import metadata
derived artifact
```

纹理、字体、GLTF/GLB、shader、音频和材质源文件通常需要导入；场景文件、C++ 源码、普通 JSON 配置和 Editor 布局可以直接读取，不强制生成 `.import`。

## 12. 运行时需要补的最小接口

为了支撑 Editor，建议优先补齐以下接口，而不是立刻改造整个引擎：

### 12.1 节点与组件操作

```cpp
WidgetId getStableEditorId() const;
void setEditorName(std::string name);
std::string getEditorName() const;

std::vector<WidgetSharedPtr> getChildren() const;
bool removeChild(const WidgetSharedPtr& child);
bool removeComponent(const ComponentSharedPtr& component);
```

如果当前 `Widget` 已有类似 API，优先适配现有命名，不要平行增加重复接口。

### 12.2 修改通知

属性 setter 修改后，Editor 需要知道哪些内容失效：

```text
Transform changed
Layout dirty
Geometry dirty
Material dirty
Resource reload requested
Continuous update requested
```

建议先采用明确的 `markDirty()`/Observable 机制，不要让 Editor 每次修改后全场景强制重建。

### 12.3 编辑器上下文

建议增加轻量的 `EditorContext`，只向编辑器创建的对象提供：

- 当前文档 ID；
- 预览模式；
- 编辑器选择状态；
- 诊断 sink；
- 资源解析器。

不要把 EditorContext 放进所有运行时热路径，运行时应用应可以完全不链接编辑器模块。

## 13. 分阶段实施路线

### Phase 0：整理入口和构建目标（已实现）

目标：让 Editor 从硬编码 demo 变成正式 target，并建立项目/场景入口约定。场景解析和文档实例化不属于本阶段。

已完成：

- 将 `editor/Editor.cpp` 纳入 CMake；
- 明确 `MorrowEditor` target；
- 增加 `EditorApplication`，统一 Engine、资源路径和窗口配置；
- 支持 `--project <MorrowUI.morrow>` 和 `--scene <path/to/main.scene>` 参数；
- 增加测试工程目录 `editor/morrow.gui/`；
- 增加测试工程文件 `editor/morrow.gui/MorrowUI.morrow`；
- 增加最小场景文件 `editor/morrow.gui/scenes/main.scene`；
- 默认按当前目录、可执行文件目录向上路径和内置测试工程路径查找 `MorrowUI.morrow`；
- 场景文件存在时进行扩展名检查，缺失时使用内置 bootstrap preview；
- `MorrowEditor` target 只在 Windows 平台生成；
- 保留现有 samples 和 tests 的 CMake 组织方式，不改动其入口。

当前验收状态：

- [x] CMake 配置阶段识别 `MorrowEditor` target；
- [x] 可以传入指定工程和场景路径；
- [x] 工程文件使用 `.morrow`、场景文件使用 `.scene`；
- [x] 测试工程位于 `editor/morrow.gui/`；
- [x] 从构建输出目录启动时可以通过默认测试工程路径定位项目；
- [x] 场景缺失时给出明确提示并进入 bootstrap preview；
- [x] SceneDocument 和运行时实例化模块可在 Windows LLVM 环境编译；
- [x] 预览窗口由场景文档实例化按钮；
- [ ] 当前命令行环境下完整 `MorrowEditor.exe` 链接仍需使用与仓库 `libglfw3.a` ABI 匹配的 MinGW 工具链。

### Phase 1：场景编辑最小闭环（核心服务已完成，UI 面板待接入）

已完成：

- `SceneDocument` 文本格式读写；
- 场景格式 Lexer/Parser 和基础 AST；
- `external_resource` 声明读取；
- `sub_resource` 声明和属性读取；
- `.import` sidecar 扫描；
- Asset ID、源路径、导入器、导入选项和平台 artifact 解析；
- Asset ID 到源资源路径解析；
- 节点稳定 ID、类型、名称和父子关系读取；
- 基础属性读取：`visible`、`display_layer`、`position`、`size`、`text`、`font_size`、`background_color`；
- `SceneNode`、`MRButton`、`MRImage`、`MRLabel` 的基础实例化；
- 纹理 Asset ID 到 `Texture::setImageUrl()` 的运行时加载路径；
- `Style` 子资源到 `MRButton` 圆角和背景色的应用；
- `MorrowEditor` 从 `editor/morrow.gui/scenes/main.scene` 创建预览对象；
- 解析错误包含场景文件行号和具体原因；
- 场景文档稳定序列化和保存；
- `findNode`、属性修改、属性删除和重父级文档 API；
- 重父级循环检测；
- `CommandHistory` 的 execute/undo/redo；
- `SetNodePropertyCommand`；
- `ReparentNodeCommand`；
- `AddNodeCommand`、`DeleteNodeCommand`、`DuplicateNodeCommand`；
- 连续属性命令的合并；
- `EditorSession` 统一 Scene Tree、Inspector、Gizmo、命令和保存入口；
- `EditorInputRouter` 支持 Ctrl+S、Ctrl+Z、Ctrl+Shift+Z 和 Ctrl+Y；
- 2D 矩形命中选择；
- 2D Transform position/size Gizmo 数据入口；
- Scene Tree 数据模型和 Inspector 属性模型；
- 文档层与运行时实例化层拆分，纯文档测试不依赖 OpenGL/GLFW；
- `SceneDocumentTests` 覆盖加载、资源声明、Asset ID、Scene Tree、Inspector、2D 选择、Gizmo、增删复制、命令合并、Ctrl+S、保存和重载。

核心服务和 Editor Shell 已接入，整体布局参照 Godot 的工作台结构：

```text
┌─────────────────────────────────────────────────────────────┐
│ Toolbar: Save / Undo / Redo / Project status                │
├──────────────┬──────────────────────────────┬───────────────┤
│ Scene Tree   │ 2D Viewport                  │ Inspector     │
│              │ selection / gizmo            │ properties     │
├──────────────┴──────────────────────────────┴───────────────┤
│ Status / Import / Build output                              │
└─────────────────────────────────────────────────────────────┘
```

已接入：

- `EditorShell` 固定工作台布局；
- Scene Tree 面板绘制和节点选择；
- Inspector 属性列表绘制；
- 保存、撤销、重做工具栏按钮；
- Windows GLFW 键盘回调；
- 现有 `InputEventsManager` 鼠标事件订阅；
- 2D 矩形命中和鼠标拖动位置 Gizmo；
- 文档命令成功后的 RuntimeInstance 清空和重建；
- `ImportQueue`：按 AssetDatabase 生成 `.morrow/imported/<asset_id>/<platform>/` 派生 artifact；
- `BuildQueue`：Configure/Build 任务接口。

当前仍属于 UI/平台完善工作：

- 将固定面板升级为可保存/可拖拽的真正 Dock 布局；
- Inspector 从“点击回写当前值”升级为文本/数字/颜色等专用编辑控件；
- 完善鼠标坐标缩放、视口平移、缩放、尺寸 Gizmo 和多选；
- 将 BuildQueue 的 stdout/stderr、取消、进程状态和 Run 接入底部输出面板；
- 将 ImportQueue 的增量哈希、导入器版本检查和失败重试接入资源面板；
- 完整 `MorrowEditor.exe` 链接需要与仓库 GLFW 静态库匹配的 MinGW Windows 工具链。

验收：

- 不写 C++ 代码也能搭建一个简单 UI；
- 关闭并重新打开 Editor 后场景一致；
- 任意一次属性修改都可以撤销和重做；
- 未知资源和未知属性不会静默丢失。

### Phase 2：资源与 C++ 构建工作流

- Asset Browser；
- 资源路径校验；
- CMake Configure/Build/Run；
- 编译日志和错误定位；
- `Build`、`Build & Run`、`Run Last Successful`；
- Preview 状态管理；
- 运行时日志接入 Console。

验收：

- 修改一个 C++ Component 后可以通过 `Build & Run` 看到新行为；
- 编译失败时旧预览仍可运行；
- Editor 不会因为预览进程崩溃而退出；
- 可以区分“场景未保存”和“代码未构建”。

### Phase 3：调试和生产力

- 独立 Preview 进程；
- 运行时对象与文档对象标识；
- Object snapshot 面板；
- 选择对象与运行时 DebugObjectId 对应；
- 帧统计；
- 场景引用检查；
- 布局保存；
- 多场景和启动场景；
- Windows 预览进程隔离和崩溃恢复。

### Phase 4：再评估高级能力

只有在实际使用中确认需求后，再评估：

- C++ 增量热重载；
- 插件模块；
- 资源导入缓存；
- 动画时间轴；
- 更完整的 3D 操作；
- Editor 脚本化；
- 外部 IDE 深度集成。

## 14. 风险和应对

| 风险 | 影响 | 建议 |
| --- | --- | --- |
| C++ 编译慢 | 预览反馈变慢 | 增量构建、预编译头、`Run Last Successful`、编译和编辑解耦 |
| C++ 编译失败 | 用户无法运行新逻辑 | 保留上一次成功产物，明确显示 Preview Outdated |
| 运行时崩溃 | Editor 丢失工作 | 尽早采用独立 Preview 进程，场景命令实时保留 |
| 类型属性缺少反射 | Inspector 覆盖不足 | 显式注册，按组件逐步扩展 |
| 文档与代码模型不一致 | 加载/保存错误 | 版本化格式、迁移器、未知字段警告 |
| 动态对象混入场景 | 保存污染 | 来源标记，Runtime Only 默认不序列化 |
| 资源绝对路径失效 | 跨机器不可用 | 项目相对路径、Asset ID 和 `.import` sidecar |
| 导入设置和派生资源失配 | 运行时加载错误或重复导入 | 源哈希、导入器版本、平台 artifact 和导入状态 |
| 非 Windows 环境 | Editor 无法运行 | Editor target 只在 Windows 生成，其他平台只构建引擎和目标应用 |
| 使用全局注册表过度 | 测试和生命周期复杂 | ObjectRegistry 主要用于诊断，文档状态由 SceneDocument 管理 |
| 过早热重载 | ABI、线程、GPU 资源风险 | 延后到模块边界稳定并有真实需求后评估 |

## 15. 建议的验收指标

第一版不应只用“窗口能打开”验收，至少应有以下场景：

```text
场景打开时间
场景保存时间
100/500/1000 个节点下的层级树操作响应
Inspector 单属性修改到视口刷新的延迟
撤销/重做正确性
资源缺失提示
CMake 增量构建耗时
编译失败后旧预览可用性
预览崩溃后 Editor 可恢复性
场景格式向后兼容性
```

运行时性能指标继续沿用现有架构中的：

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

Editor 自身还应记录：

```text
Scene load/save time
Command history size
Asset scan time
Build configure/build time
Preview restart time
Diagnostic message count
```

## 16. 需要审查时重点决定的问题

建议先确认以下决策，再开始实现 Phase 0/1：

1. 是否接受 `*.scene` 作为场景文件扩展名，工程入口固定使用 `MorrowUI.morrow`？
2. Editor 只支持 Windows；其他平台不生成 `MorrowEditor` target。
3. Editor UI 是否先使用 MorrowUI 自身控件？
4. 第一版是否接受显式 C++ 类型注册，而不是自动反射？
5. 场景文档是否允许保留未知 section、属性和资源引用，以支持未来版本和第三方扩展？
6. 预览第一阶段采用进程内实例，还是直接从第一天采用独立进程？
7. C++ 项目入口是复用现有 samples/CMake，还是建立独立的 `MorrowUI.morrow` 项目模板？
8. `.import` 文件是否统一使用 INI 风格，还是允许复杂导入器使用 JSON？
9. 运行时动态创建的对象是否全部默认为 Runtime Only？

## 17. 最终建议

建议批准以下方向：

- Editor 是独立工具，不是 `Engine` 中的特殊分支；
- SceneDocument 是可持久化、可撤销的编辑真相，场景文件直接采用 MorrowUI 自有的 Godot 风格文本格式；
- RuntimeInstance 由文档实例化，不反向承担文档格式；
- 属性反射采用显式注册，先覆盖核心组件；
- 所有编辑操作统一走命令系统；
- C++ 是默认逻辑语言，Build/Run 是明确的用户操作；
- 外部资源使用 `.import` sidecar 管理导入器、源哈希、Asset ID、导入选项和平台派生资源；
- 第一阶段接受无“保存即生效”，用清晰的 `Scene Dirty`、`Code Dirty`、`Preview Outdated` 状态弥补反馈差异；
- 先做 2D/UI 场景编辑闭环，再做资源、独立预览和调试；
- 暂不做 C++ 热重载、蓝图和复杂 3D 编辑器。

这条路线的关键不是让 Editor 一开始功能很多，而是让“场景文档 -> 运行时对象 -> 渲染预览 -> C++ 构建 -> 诊断”形成稳定闭环。闭环稳定后，后续增加组件、资源类型和高级调试能力都可以沿用同一套文档、反射和命令基础设施。
