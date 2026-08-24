# MorrowEditor Dock 与文件系统演进方案

## 1. 目标

当前 MorrowEditor 已具备场景树、视口、Inspector、输出区域和基础资源扫描能力。下一阶段需要向 Godot 一类集成编辑器布局演进，重点包括：

- 增加项目文件系统面板。
- 各编辑器模块能够通过鼠标调整大小。
- 面板能够拖动、吸附、停靠和组合为标签页。
- 编辑器布局能够持久化，并在窗口尺寸及 DPI 变化后正确恢复。
- 文件、资源、场景节点和属性之间能够使用统一拖放机制。

本方案的核心原则是：

- 引擎提供通用 UI、输入、窗口和渲染基础能力。
- 编辑器实现 Dock 工作区、文件系统模型、资源浏览器等业务逻辑。
- 不把 Godot 风格的编辑器业务直接耦合进引擎核心。

## 2. 当前实现评估

### 2.1 已具备的基础能力

引擎当前已经具备：

- 顶层交互控件命中测试。
- 鼠标按下后的 Pointer Capture。
- 鼠标进入和离开事件。
- 键盘焦点管理。
- `MRTree`、`MRScrollContainer` 等基础控件。
- 窗口尺寸和 framebuffer 尺寸的 GLFW 回调。

Pointer Capture 已足以支撑分割条和面板拖动：鼠标按下后，即使离开原控件，移动和释放事件仍会发送给捕获目标。

### 2.2 当前 Dock 的限制

当前 `DockLayout` 使用绝对矩形描述面板：

```cpp
struct DockPanelState {
    std::string id;
    float x;
    float y;
    float width;
    float height;
    bool visible;
    int order;
};
```

这种数据模型只能记录独立矩形，无法表达：

- 左右或上下分割关系。
- 分割比例。
- Tab 面板组。
- 面板最小尺寸。
- 面板吸附关系。
- 窗口变化后的响应式布局。

当前 Dock 拖动通过右键按住标题区域后直接修改面板 `x/y`，本质是移动浮动矩形，还不是真正的 Dock 系统。

### 2.3 当前资源浏览能力的限制

现有 `AssetDatabase` 主要面向可导入资源：

- 只记录受支持扩展名的文件。
- 数据结构是扁平资源数组。
- 主要职责是 `asset_id`、Import Metadata、导入状态和资源路径映射。

当前 Assets 操作只是把扫描结果写入 Output，没有真正的目录树、文件列表、搜索、重命名、拖放和增量刷新能力。

`AssetDatabase` 不应直接扩展成完整文件系统模型。资源数据库和项目文件系统应保持职责分离。

## 3. 引擎层需要新增的能力

## 3.1 `MRSplitContainer`

这是第一优先级组件，用于两个区域之间的可拖动分割。

建议接口：

```cpp
enum class SplitOrientation {
    Horizontal,
    Vertical
};

class MRSplitContainer : public UIWidget {
public:
    void setOrientation(SplitOrientation orientation);
    void setSplitRatio(float ratio);
    void setFirstMinSize(float size);
    void setSecondMinSize(float size);
    void setHandleWidth(float width);

    float getSplitRatio() const;

    Observable<float> onSplitRatioChanged;
};
```

组件负责：

- 根据父容器尺寸布局两个子节点。
- 渲染和命中中间分割条。
- 鼠标拖动时更新分割比例。
- 限制两侧区域的最小尺寸。
- 窗口变化后保持比例。
- 拖动时切换正确的 resize 光标。

编辑器初始布局可以表达为：

```text
VerticalSplit
├── MainArea
│   └── HorizontalSplit
│       ├── LeftDock
│       └── HorizontalSplit
│           ├── Viewport
│           └── Inspector
└── BottomDock
```

采用嵌套 Split 后，不再由 `EditorShell` 手工计算每个面板的绝对坐标。

## 3.2 父级裁剪和 Scissor Stack

文件树、Inspector、Output 和文件列表都需要严格限制内容渲染范围。

建议在 `UIWidget` 增加：

```cpp
void setClipChildren(bool clip);
bool getClipChildren() const;
```

渲染阶段维护嵌套裁剪栈：

```text
effective clip = parent clip ∩ local clip ∩ viewport
```

OpenGL 后端可以使用 `glScissor` 实现。

裁剪能力必须同时应用到：

- GPU 渲染。
- 控件命中测试。
- Hover 目标解析。
- 滚动容器子元素交互。

当前命中测试会递归检查所有可见子节点。如果子节点超出父容器但仍为 visible，它仍可能截获鼠标。引入 `clipChildren` 后，命中测试必须携带父级有效裁剪区域。

## 3.3 `MRTabContainer`

编辑器多个模块通常共享同一 Dock 区域，例如：

- Scene / Import。
- Inspector / Node / History。
- Output / Debugger / Audio / Build。
- FileSystem / Favorites。

建议提供通用 Tab 容器：

```cpp
class MRTabContainer : public UIWidget {
public:
    void addTab(
        std::string id,
        std::wstring title,
        std::shared_ptr<UIWidget> content);

    void removeTab(const std::string& id);
    void selectTab(const std::string& id);
    void moveTab(size_t from, size_t to);

    Observable<std::string> onCurrentTabChanged;
};
```

Tab 容器负责：

- Tab 标题栏布局。
- 当前活动页切换。
- Tab 顺序调整。
- Tab 关闭和可见状态。
- 标题过多时滚动或折叠菜单。

Dock 系统只管理 Tab Group，不直接管理具体业务控件。

## 3.4 通用 `DragDropManager`

现有 Pointer Capture 可以处理简单拖动，但 Dock、文件和资源需要语义化拖放。

建议增加：

```cpp
struct DragPayload {
    std::string type;
    std::any data;
};

class DragDropManager {
public:
    void beginDrag(
        const std::shared_ptr<Widget>& source,
        DragPayload payload,
        const std::shared_ptr<Widget>& preview);

    void cancelDrag();
};
```

目标控件需要接收：

```text
DragEnter
DragMove
DragLeave
Drop
DragCancelled
```

建议使用类型化 Payload：

```text
editor/dock-panel
editor/asset-list
editor/filesystem-entry
editor/scene-node
```

统一拖放机制可支持：

- 面板拖到不同停靠区域。
- Tab 在不同组之间移动。
- 文件拖入场景生成节点。
- 纹理拖到 Inspector 的资源属性。
- 场景节点拖动调整父子关系。
- 外部文件拖入项目 Assets 目录。

不应继续在 `EditorShell::handleInput()` 中叠加不同类型的全局拖动状态。

## 3.5 窗口尺寸和 DPI 变化事件

窗口实现已经接收 GLFW 的窗口和 framebuffer 回调，但编辑器需要订阅这些变化。

建议增加：

```cpp
struct WindowEvents {
    Observable<Vector2> onWindowSizeChanged;
    Observable<Vector2> onFramebufferSizeChanged;
    Observable<float> onContentScaleChanged;
};
```

需要明确区分：

- Window logical size。
- Framebuffer pixel size。
- 鼠标输入坐标。
- DPI/content scale。

编辑器布局根节点应在尺寸变化后重新执行布局，而不是只在 `buildLayout()` 中读取一次窗口尺寸。

布局持久化中也应记录保存时的主窗口尺寸和 DPI，以便恢复时进行合理归一化。

## 3.6 鼠标光标 API

分割条和拖放操作需要平台无关的光标接口：

```cpp
enum class CursorShape {
    Arrow,
    IBeam,
    Hand,
    ResizeHorizontal,
    ResizeVertical,
    ResizeAll,
    Forbidden
};

class Window {
public:
    virtual void setCursorShape(CursorShape shape);
};
```

典型行为：

- 水平 Split 使用水平 resize 光标。
- 垂直 Split 使用垂直 resize 光标。
- 浮动面板标题栏使用移动光标。
- 无效 Drop Zone 使用禁止光标。

## 3.7 虚拟化列表

项目文件较多时，不能为所有文件永久创建一个 `MRButton`。

建议增加通用 `MRVirtualList`，或者为 `MRTree` 和文件列表增加：

- 可见行计算。
- 行控件复用。
- 数据模型和视图分离。
- 固定行高快速定位。
- 选中项自动滚动到可见区域。

虚拟化不是第一阶段阻塞项，但应在文件系统进入大规模使用前完成。

## 4. 编辑器层需要新增的组件

## 4.1 `ProjectFileSystemModel`

文件系统模型负责显示项目目录中的所有合法文件，而不是只显示可导入资源。

建议数据结构：

```cpp
enum class AssetImportState {
    NotApplicable,
    Ready,
    NeedsImport,
    Importing,
    Failed
};

struct FileSystemEntry {
    std::filesystem::path relativePath;
    std::string name;
    bool directory = false;
    uintmax_t size = 0;
    std::filesystem::file_time_type modifiedTime;
    std::string assetId;
    std::string assetType;
    AssetImportState importState = AssetImportState::NotApplicable;
    std::vector<FileSystemEntry> children;
};
```

建议接口：

```cpp
class ProjectFileSystemModel {
public:
    bool scan(
        const std::filesystem::path& projectRoot,
        std::string& error);

    bool refreshDirectory(
        const std::filesystem::path& directory,
        std::string& error);

    bool createFolder(...);
    bool rename(...);
    bool remove(...);

    Observable<FileSystemChange> onChanged;
};
```

职责划分：

`ProjectFileSystemModel` 负责：

- 文件夹层级。
- 所有可显示文件。
- 文件创建、删除、重命名和移动。
- 搜索和过滤。
- 文件变化通知。

`AssetDatabase` 继续负责：

- `asset_id`。
- Import Metadata。
- 导入状态。
- Asset 到源文件的映射。
- Import Queue 的数据输入。

文件系统模型可以查询 `AssetDatabase`，但不替代它。

## 4.2 `FileSystemPanel`

建议面板结构：

```text
FileSystemPanel
├── Toolbar
│   ├── Back
│   ├── Forward
│   ├── Refresh
│   ├── Breadcrumb
│   └── Search
├── ContentSplit
│   ├── DirectoryTree
│   └── FileListOrGrid
└── StatusBar
```

第一阶段应至少支持：

- 文件夹树展开和折叠。
- 当前目录文件列表。
- 单选和双击打开。
- 搜索过滤。
- 手动刷新。
- 文件类型图标。
- 导入状态展示。
- 文件拖放 Payload。
- 右键菜单。
- 新建目录和重命名。

后续支持：

- 网格和列表视图切换。
- 多选。
- 收藏夹。
- 最近访问目录。
- 缩略图。
- 文件排序。
- 外部文件导入。

## 4.3 文件变化监听器

第一阶段可以在编辑器层定时检查目录更新时间。

正式版本建议提供平台抽象：

```cpp
class FileSystemWatcher {
public:
    void watch(const std::filesystem::path& root);
    void stop();

    Observable<std::vector<FileChange>> onFilesChanged;
};
```

Windows 后端可以使用 `ReadDirectoryChangesW`。

收到变化后应：

1. 合并短时间内重复事件。
2. 增量更新受影响目录。
3. 通知 `ProjectFileSystemModel`。
4. 更新对应 `AssetDatabase` 记录。
5. 将需要重新导入的资源加入 Import Queue。

不应在每帧递归扫描整个项目目录。

## 4.4 缩略图服务

文件网格最终需要纹理、字体、场景和模型缩略图。

建议增加编辑器服务：

```cpp
class ThumbnailService {
public:
    ThumbnailHandle request(
        const std::filesystem::path& path,
        const Vector2& size);
};
```

缩略图服务负责：

- 后台读取和解码。
- 渲染线程纹理上传。
- 请求去重。
- LRU 缓存。
- 文件更新时间变化后的缓存失效。
- 默认类型图标和失败占位图。

该服务属于编辑器资源服务，不属于核心 UI 组件。

## 5. Dock 数据模型重构

当前绝对矩形数组应升级为 Dock Tree。

建议数据结构：

```cpp
using DockNodeId = uint64_t;

struct DockSplitNode {
    SplitOrientation orientation;
    float ratio = 0.5f;
    DockNodeId first = 0;
    DockNodeId second = 0;
};

struct DockTabNode {
    std::vector<std::string> panelIds;
    std::string activePanelId;
};

struct DockFloatingNode {
    Rect bounds;
    DockNodeId content = 0;
};

using DockNode = std::variant<
    DockSplitNode,
    DockTabNode,
    DockFloatingNode>;
```

编辑器层建议增加：

```text
DockWorkspace
DockPanel
DockTabGroup
DockDropOverlay
DockLayoutSerializer
DockCommand
```

### 5.1 Dock Drop Zone

拖动面板时，目标区域显示五个 Drop Zone：

```text
        Top
Left   Center   Right
       Bottom
```

释放后的操作：

- `Center`：加入目标 Tab Group。
- `Left` / `Right`：创建水平 Split Node。
- `Top` / `Bottom`：创建垂直 Split Node。
- 工作区之外：创建 Floating Node。

释放后修改 Dock Tree，而不是直接修改面板绝对坐标。

### 5.2 面板注册

面板实例和布局状态应分离：

```cpp
struct EditorPanelDescriptor {
    std::string id;
    std::wstring title;
    Vector2 minSize;
    std::function<std::shared_ptr<UIWidget>()> factory;
};

class EditorPanelRegistry {
public:
    void registerPanel(EditorPanelDescriptor descriptor);
    std::shared_ptr<UIWidget> createPanel(const std::string& id);
};
```

这样可以逐步注册：

```text
scene_tree
filesystem
viewport_2d
viewport_3d
inspector
output
build
import
history
```

Dock Layout 只保存面板 ID，不保存控件对象。

## 6. 布局持久化

当前空格分隔的矩形文本建议替换成带版本号的 JSON 或 TOML。

应保存：

- Layout format version。
- Dock Tree。
- Split orientation 和 ratio。
- Tab 顺序。
- 当前活动 Tab。
- 面板 visible 状态。
- Floating window bounds。
- 主窗口尺寸。
- DPI/content scale。
- 可选的面板业务状态。

示例：

```json
{
  "version": 2,
  "window": {
    "width": 1920,
    "height": 1080,
    "content_scale": 1.0
  },
  "root": {
    "type": "split",
    "orientation": "vertical",
    "ratio": 0.82,
    "first": {
      "type": "split",
      "orientation": "horizontal",
      "ratio": 0.16,
      "first": {
        "type": "tabs",
        "active": "scene_tree",
        "panels": ["scene_tree", "filesystem"]
      },
      "second": {
        "type": "tabs",
        "active": "viewport_2d",
        "panels": ["viewport_2d"]
      }
    },
    "second": {
      "type": "tabs",
      "active": "output",
      "panels": ["output", "build"]
    }
  }
}
```

加载布局时需要：

- 校验版本。
- 忽略已经不存在的面板。
- 为新面板应用默认位置。
- 限制非法 Split Ratio。
- 修复空 Tab Group。
- 当布局无法恢复时回退到默认布局。

## 7. `EditorShell` 重构建议

当前 `EditorShell` 同时负责：

- 布局创建。
- 场景树。
- Inspector。
- 资源显示。
- Dock 拖动。
- 视口交互。
- Build 和运行。
- 输入路由。

随着文件系统和 Dock 加入，该类会迅速膨胀。

建议逐步拆分：

```text
EditorShell
├── EditorWorkspace
│   └── DockWorkspace
├── SceneTreePanel
├── FileSystemPanel
├── InspectorPanel
├── ViewportPanel
├── OutputPanel
├── BuildPanel
├── EditorCommandRouter
└── EditorServices
    ├── AssetDatabase
    ├── ProjectFileSystemModel
    ├── ImportQueue
    ├── BuildQueue
    ├── ThumbnailService
    └── FileSystemWatcher
```

`EditorShell` 最终只负责：

- 初始化服务。
- 注册编辑器面板。
- 创建主工作区。
- 连接全局事件。
- 保存和恢复编辑器状态。

具体面板自行管理内部控件和输入。

## 8. 推荐实施阶段

### Phase 1：可调整大小的固定布局

引擎：

- 实现 `clipChildren` 和 Scissor Stack。
- 让命中测试遵守父级裁剪。
- 实现 `MRSplitContainer`。
- 增加 resize cursor。
- 暴露窗口尺寸和 DPI 事件。

编辑器：

- 使用嵌套 Split 替换固定矩形布局。
- Scene、Viewport、Inspector、Output 可以拖动分割条调整尺寸。
- 保存和恢复 Split Ratio。

这个阶段不实现任意面板 Dock，但可以立即解决当前固定布局无法调整的问题。

### Phase 2：项目文件系统

- 实现 `ProjectFileSystemModel`。
- 实现 `FileSystemPanel`。
- 复用或增强 `MRTree` 和 `MRScrollContainer`。
- 文件系统作为左侧固定面板或 Scene 的 Tab。
- 支持目录导航、搜索、刷新和资源状态显示。

### Phase 3：Tab 工作区

- 实现 `MRTabContainer`。
- Scene / FileSystem 共享左侧区域。
- Output / Build / Import 共享底部区域。
- 支持 Tab 排序、关闭和状态持久化。

### Phase 4：通用拖放和完整 Dock

- 实现 `DragDropManager`。
- 实现 Dock Drop Overlay。
- `DockLayout` 升级为 Split/Tab Tree。
- 支持面板在不同区域之间移动。
- 支持 Floating Panel。
- 增加 Dock 操作的撤销或布局重置。

### Phase 5：文件系统增强

- 平台文件变化监听。
- 缩略图服务。
- 虚拟化文件列表。
- 多选和批量操作。
- 外部文件拖入。
- 文件拖到场景和 Inspector。

## 9. 最小新增组件清单

为了完成第一版 Godot 风格编辑器布局，最低限度需要：

### 引擎层

- `MRSplitContainer`
- `MRTabContainer`
- `clipChildren` / Scissor Stack
- 裁剪感知的 Hit Test
- `DragDropManager`
- Window resize / framebuffer resize / DPI 事件
- Cursor Shape API

### 编辑器层

- `DockWorkspace`
- Dock Split/Tab Tree 数据模型
- `DockDropOverlay`
- `DockLayoutSerializer`
- `EditorPanelRegistry`
- `ProjectFileSystemModel`
- `FileSystemPanel`

### 后续增强

- `FileSystemWatcher`
- `ThumbnailService`
- `MRVirtualList`
- 原生文件选择器和外部文件 Drop

## 10. 结论

MorrowUI 当前已有输入捕获、基础 Tree、ScrollContainer 和 AssetDatabase，可以继续复用。真正缺少的是编辑器级工作区所依赖的通用容器和裁剪能力。

推荐优先实现：

1. 裁剪和裁剪感知命中测试。
2. `MRSplitContainer`。
3. 窗口变化事件和 resize cursor。
4. 使用嵌套 Split 重构当前固定布局。
5. `ProjectFileSystemModel` 和 `FileSystemPanel`。
6. `MRTabContainer`。
7. 通用 DragDrop 和完整 Dock Tree。

其中裁剪、Split、Tab、拖放、窗口事件和光标属于引擎通用能力；文件系统、Dock Tree、面板注册和布局持久化应放在 `editor/`。
