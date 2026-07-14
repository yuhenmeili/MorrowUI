# MorrowUI 架构说明

## 1. 文档范围

本文描述 MorrowUI 的目标定位、当前核心架构、运行时数据流、线程边界、资源所有权和后续优化方向。

本文只保留：

- 当前有效的架构说明；
- 必须长期遵守的架构约束；
- 尚未完成的优化方向。

本文不记录历史实施过程、已完成任务、阶段验收结果和已废弃方案。

---

## 2. 项目定位

MorrowUI 是面向嵌入式设备、桌面应用和 3D 人机界面的轻量高性能 GUI 渲染引擎。

项目采用**保留式 UI 模型**，而不是立即模式：

- Widget、Component、Material、Mesh 等对象可跨帧存在；
- 应用通过修改对象状态更新界面；
- 引擎逐帧遍历需要更新和绘制的对象；
- 渲染阶段直接收集可渲染项并编码 GPU 命令。

项目在性能目标上参考 ImGui 的简单和直接，但不复制其立即模式 API。MorrowUI 的核心目标是：

1. 保持架构轻量；
2. 缩短 Widget 状态到 GPU Draw 的路径；
3. 降低每帧分配、复制和间接访问；
4. 支持高效的 2D 与 3D GUI 混合；
5. 支持单线程和渲染线程两种提交模式；
6. 保持 Windows 与 QNX 等平台的可移植性；
7. 避免为当前需求提前引入复杂资源框架、快照系统或通用 RenderGraph。

### 2.1 非目标

当前阶段不以构建通用大型 3D 引擎为目标，不优先引入：

- 通用资源注册与资源状态中心；
- 与资源对象分离的通用上传调度层；
- Widget 和渲染提交之间的完整状态副本；
- ECS Render World 镜像；
- 通用 RenderGraph 编译器；
- 跨 API 的复杂资源驻留和虚拟化系统；
- 面向超大开放世界的场景流式加载框架。

只有当实际功能或性能数据证明现有模型无法满足需求时，才增加新的抽象层。

---

## 3. 架构原则

### 3.1 短渲染路径

核心路径保持为：

```text
Widget / Component 状态
        ↓
组件 update / lateUpdate
        ↓
BatchManager 收集 RenderItem
        ↓
BatchBuilder 构建批次
        ↓
Material / Mesh / Transform 直接提交
        ↓
RenderDeviceProxy 编码命令
        ↓
GLRenderDevice 执行
```

不在 Widget 与渲染提交之间复制完整场景状态，也不维护另一套渲染世界。

### 3.2 保留式对象，逐帧直接收集

Widget 树和 Component 是长期存在的业务对象。

每帧渲染时，组件直接将当前可渲染状态提交给 `BatchManager`。`RenderItem` 保存渲染所需对象引用和轻量状态，不建立额外快照对象。

### 3.3 数据局部性优先

高频路径优先使用：

- 连续数组；
- 紧凑 POD 数据；
- 对象池和复用缓冲；
- 预分配 CommandBuffer；
- 缓存后的整数 ID 和 Hash；
- 一次遍历完成收集与分类。

避免在每个 RenderItem 上重复构造字符串、哈希容器和临时智能指针。

### 3.4 需求驱动抽象

抽象必须解决当前存在的问题。

如果简单的直接对象引用、显式生命周期和命令提交可以解决问题，则不引入全局注册表、通用句柄系统或多阶段状态机。

### 3.5 正确性优先于错误合批

UI 默认保持稳定绘制顺序。透明元素、裁剪、Stencil、RenderTarget 和 3D 深度关系不能因为减少 Draw Call 而改变。

只有在顺序可交换或实例可安全合并时才扩大批次。

---

## 4. 分层架构

```text
Application / Samples
        │
        ▼
UI
  Widget / UIWidget / Elements / Layout / HMI
  Component / ComponentManager
  Transform / MeshFilter / MeshRenderer
        │
        ▼
Core
  Engine / FrameState
  BatchManager / BatchBuilder
  Camera / Managers
        │
        ▼
Renderer
  Material / Mesh / Texture / Shader
  VertexArray / UBO / SSBO
  RenderDeviceProxy / CommandBuffer / RenderingThread
  GLRenderDevice
        │
        ▼
Platform
  Platform / Window / InputProvider
  WGL / EGL
        │
        ▼
OpenGL / OpenGL ES / Window System
```

### 4.1 目录职责

| 目录 | 职责 |
|---|---|
| `src/core/` | Engine 主循环、相机、合批、公共管理器和运行时服务 |
| `src/ui/` | Widget 树、Component、控件、布局、交互和 3D UI 元素 |
| `src/renderer/` | GPU 抽象、材质、纹理、Shader、Buffer、命令编码和渲染线程 |
| `src/platform/` | 窗口、图形上下文、输入、Present 和平台生命周期 |
| `src/gltf/` | GLTF/GLB 加载、场景构建和动画 |
| `src/fonts/` | 字体、文本布局和字形资源 |
| `src/math/` | 向量、矩阵、四元数和几何运算 |
| `src/debug/` | 运行时调试显示和性能统计 |
| `samples/` | 集成示例和功能验证入口 |
| `tests/` | CPU 单元测试、架构回归和渲染验证 |

### 4.2 依赖方向

主要依赖方向为：

```text
Application
    → UI / Core
    → Renderer abstraction
    → RenderDeviceProxy
    → GLRenderDevice
    → Platform graphics context
```

基本约束：

- UI 不直接调用 OpenGL；
- Platform 不承载 Widget 业务和合批规则；
- `BatchBuilder` 不依赖 GPU Context；
- OpenGL 调用只出现在拥有图形上下文的线程；
- 上层通过 `RenderDevice` 接口提交 GPU 操作。

---

## 5. Engine 与帧循环

`Engine` 是运行时入口，负责：

- 平台初始化；
- FrameState 更新；
- 输入、动画和 Widget 更新编排；
- 渲染提交；
- 帧后回调；
- Present 和退出。

主帧循环为：

```text
updateFrameState
    ↓
Platform::beginFrame
    ↓
Platform::dispatchEvents
    ↓
Engine::preRender
    ↓
TweenManager::update
    ↓
Platform::beginRenderPass
    ↓
Platform::updateWidgets
    ↓
Platform::lateUpdateWidgets
    ↓
Platform::commitRenderPass
    ↓
Debug / afterRender
    ↓
Platform::endFrame
```

### 5.1 输入阶段

输入处理发生在 Widget 更新之前：

```text
InputProvider
    → InputEventsManager
    → Platform::resolveInputTargets
    → Widget::dispatchTouchEvent
    → Interaction Component
```

事件导致的状态修改可以在同一帧进入 update、lateUpdate 和渲染提交。

### 5.2 更新阶段

更新分为两个阶段：

```text
update
    → 本地状态、动画、布局和普通组件逻辑

lateUpdate
    → 依赖其他组件更新结果的变换、几何和渲染收集
```

`MeshRenderer` 等渲染组件在合适阶段向当前窗口的 `BatchManager` 提交可渲染对象。

### 5.3 提交阶段

`Window::commitRenderPass()` 调用 `BatchManager::renderBatches()`，完成：

1. 检查当前可渲染列表；
2. 构建或复用批次结构；
3. 选择 SSBO 或标准绘制路径；
4. 应用 Material、Mesh 和 Transform；
5. 向 `RenderDeviceProxy` 编码 GPU 命令。

### 5.4 FrameState

`FrameState` 是当前帧的轻量运行上下文，包含：

- frame number 和 delta time；
- 2D/3D Camera；
- InputEventsManager；
- 当前 BatchManager；
- SSBOManager；
- 2D/3D 渲染上下文；
- draw call、batch 和 FPS 统计；
- 帧末回调。

`FrameState` 不拥有 Widget、Material、Mesh 或 GPU 资源的生命周期。

---

## 6. UI 模型

### 6.1 Widget 树

`Widget` 是保留式 UI 树节点，负责：

- parent / children；
- 可见性；
- display layer；
- Component 集合；
- 更新遍历；
- 输入命中和事件派发。

应用通过创建 Widget、挂载 Component 和修改对象属性构建界面。Widget 不需要每帧由业务代码重新声明。

### 6.2 Component

Widget 的功能通过 Component 组合，主要包括：

- `Transform`：局部变换和世界变换；
- `MeshFilter`：几何数据；
- `MeshRenderer`：材质、绘制状态和渲染收集；
- `MeshRenderer3D`：3D 几何提交；
- `Interaction`：输入交互；
- 布局、文本、图片和控件专用组件。

Component 之间通过明确的数据依赖协作，避免把所有逻辑集中到 Widget 基类。

### 6.3 状态更新

Widget、Component、Material、Mesh 和 Texture 是可变的保留式对象。

状态修改应满足：

- 业务修改发生在主线程；
- 同一帧内先完成状态更新，再编码 GPU 命令；
- 已写入 CommandBuffer 的数据不能依赖短生命周期临时内存；
- 多线程渲染时，命令 payload 必须拥有执行前所需的数据。

### 6.4 可见性与 Dirty

保留式 UI 需要通过 Dirty 标记避免无意义工作。

Dirty 类型应围绕实际成本划分，例如：

```text
LayoutDirty
TransformDirty
GeometryDirty
MaterialDirty
VisibilityDirty
ChildrenDirty
```

Dirty 用于跳过不必要的布局、矩阵计算、几何更新和 GPU 数据上传，但不引入完整渲染快照。

---

## 7. 渲染收集与合批

### 7.1 RenderItem

`RenderItem` 是当前帧合批输入，直接引用：

- Material；
- MeshFilter；
- Transform；
- BatchCompatibilityKey；
- display layer；
- insertion index；
- 用于判断变化的 revision。

其目标是作为轻量的帧内绘制描述，不承担独立资源所有权模型。

### 7.2 BatchManager

`BatchManager` 负责：

- 接收组件提交的 RenderItem；
- 保存当前帧可渲染列表；
- 判断批次结构是否需要重建；
- 调用 `BatchBuilder`；
- 选择渲染路径；
- 执行批次绘制；
- 记录批次统计；
- 帧末清理并复用容量。

每个 Window 持有自己的 BatchManager，避免不同窗口之间隐式共享绘制状态。

### 7.3 BatchBuilder

`BatchBuilder` 是纯 CPU 批次构建器，不创建 GPU 资源，也不依赖 Window、FrameState 或 RenderingThread。

输入：

```text
连续 RenderItem 列表
```

输出：

```text
BatchGroup 列表
+ 断批原因
```

这种边界便于：

- 单元测试；
- 独立优化批次算法；
- 对比不同排序和合并策略；
- 保持 GPU 提交代码简单。

### 7.4 BatchCompatibilityKey

批次兼容条件包括：

- Shader 和 Shader Variant；
- Texture 集合；
- Blend 状态；
- Double-sided / Cull 状态；
- Material 状态；
- Primitive Topology；
- Vertex Layout；
- RenderTarget；
- Clip / Stencil；
- SSBO Layout。

Key 相同只表示 GPU 状态兼容，不表示元素一定可以越过其他元素重排。

### 7.5 绘制顺序

2D UI 默认保持 painter's order：

```text
A → B → A
```

不能仅因为两个 A 使用相同材质就跨越 B 合并。

`displayLayer` 和 `insertionIndex` 共同形成稳定顺序。透明、Clip、Stencil、RenderTarget 和具有重叠关系的元素默认不可重排。

### 7.6 SSBO 与标准路径

渲染提交分为：

- SSBO 路径：批量提交实例数据，减少状态切换和 Draw Call；
- 标准路径：按普通 Uniform、VBO 和纹理接口提交；
- 安全回退路径：无法正确实例化的对象保持顺序逐项绘制。

所有路径必须产生一致的视觉结果。性能优化不能改变材质、变换、透明度和绘制顺序语义。

---

## 8. GPU 资源模型

### 8.1 直接所有权

GPU 资源由对应的高层对象或专用管理器直接持有：

| 资源 | 主要所有者 |
|---|---|
| Texture2D | Texture |
| GPUProgram | Material / Shader |
| VBO / VertexArray | Mesh / VertexArray |
| UBO | UniformBuffer 或对应渲染上下文 |
| SSBO | SSBOManager / Batch 路径 |
| RenderTarget | OffscreenRenderTarget |

对象通过 `RenderDevice` 接口创建、更新、绑定和删除 GPU 资源。

资源生命周期保持显式，不通过通用 Registry 建立第二套资源身份和状态。

### 8.2 Texture

Texture 负责：

- CPU 图像描述和像素数据；
- 文件或 Basis 数据加载；
- Texture2D 创建；
- 上传状态；
- 纹理绑定；
- GPU 纹理销毁。

纹理更新直接编码到 RenderDeviceProxy。

在多线程模式下，上传命令执行前所需的像素数据必须满足以下条件之一：

- 由 `shared_ptr` 保持所有权；
- 复制到可复用上传缓冲；
- 由 CommandBuffer payload 明确持有。

### 8.3 Material 与 Shader

Material 负责：

- Shader 选择和创建；
- Texture 绑定；
- Blend、Cull 等状态；
- Vector、Float、Int、Matrix 和数组参数；
- 2D 与 3D 材质参数；
- 用于批次兼容判断的 revision 和 hash。

Material 在绘制时直接应用当前状态，不生成额外的材质状态副本。

### 8.4 Mesh 与 Buffer

Mesh 和 VertexArray 负责几何数据与 GPU Buffer。

CPU 几何数据变化时，通过 revision 或 Dirty 状态触发 VBO 更新。未变化的几何不应重复上传。

### 8.5 资源销毁

单线程模式下，GPU 对象可以在图形上下文线程直接销毁。

多线程模式下，删除操作必须编码为命令，由渲染线程按命令顺序执行。资源对象析构后，已提交命令仍需具备安全执行条件。

不使用资源注册表并不意味着忽略异步生命周期；生命周期安全由以下机制保证：

```text
明确对象所有权
+ CommandBuffer payload 所有权
+ 命令 FIFO 顺序
+ Fence 和回收队列
```

---

## 9. RenderDevice 与命令系统

### 9.1 RenderDevice

`RenderDevice` 是 GPU 操作抽象，由以下接口组成：

- `GPUBufferDevice`；
- `GPUTextureDevice`；
- `GPUShaderDevice`；
- `GPURenderPassDevice`。

主要能力包括：

- VBO、UBO、SSBO 创建、更新、绑定和销毁；
- Texture 创建、上传、绑定和销毁；
- Shader 创建、使用、Uniform 设置和销毁；
- RenderTarget 创建、绑定和销毁；
- viewport、clear、blend、depth 和 cull；
- Fence 和同步。

### 9.2 RenderDeviceProxy

上层渲染代码调用 `RenderDeviceProxy`。

在单线程模式下，Proxy 可以在当前线程直接执行或使用相同接口完成提交。

在多线程模式下，Proxy 将调用编码到固定容量的 CommandBuffer：

```text
[Command Header | Aligned Payload]
```

渲染线程读取 CommandBuffer，并调用 `GLRenderDevice` 执行实际 OpenGL/OpenGL ES 操作。

### 9.3 CommandBuffer

CommandBuffer 应满足：

1. 固定或可控容量；
2. 对齐存储；
3. 尽量避免逐命令堆分配；
4. 非平凡 payload 正确构造和析构；
5. payload 不引用提前失效的栈内存；
6. 满载和溢出具有明确诊断；
7. 清空时正确释放命令持有的数据。

### 9.4 多线程模型

```text
主线程
  ├─ 输入与动画
  ├─ Widget / Component 更新
  ├─ 收集 RenderItem
  ├─ 构建 Batch
  ├─ 编码 CommandBuffer
  └─ 提交 frame slot
               │
               ▼
渲染线程
  ├─ 获取图形上下文
  ├─ 顺序执行 GPU 命令
  ├─ 插入 Fence
  ├─ 回收上传和 Buffer 数据
  └─ Present
```

CommandBuffer 使用环形 frame slot。主线程只有在写入速度超过渲染线程且环形缓冲区满时才等待。

### 9.5 数据回收

VBO、UBO、SSBO 和像素上传数据使用可复用缓冲或对象池。

回收时机按数据用途区分：

- CPU 数据仅被同步 OpenGL 调用读取时，可在命令执行后回收；
- 数据可能被 GPU 异步读取时，需要等待对应 Fence；
- 资源删除命令必须在最后一次使用命令之后执行。

---

## 10. 2D 与 3D GUI

### 10.1 2D UI

2D UI 使用正交相机，主要渲染数据包括：

- Widget 世界变换；
- Mesh；
- Material；
- Texture；
- display layer；
- Blend / Clip / Stencil 状态。

2D 路径优先保证：

- 稳定绘制顺序；
- 低 Draw Call；
- 低 CPU 提交成本；
- 文本和图片高效渲染。

### 10.2 3D UI

3D GUI 使用透视相机和 `Scene3DPassContext`，包含：

- PerspectiveCamera；
- 3D RenderTarget；
- viewport；
- depth 和 cull；
- lighting；
- IBL；
- frame UBO；
- GLTF 场景和 3D Mesh。

`MR3DSceneView` 是 Widget 树中的 3D 视图入口，可将 3D 场景渲染到 Offscreen RenderTarget，再作为 UI 内容参与组合。

### 10.3 Offscreen 渲染

`OffscreenRenderTarget` 直接持有 RenderTarget 和颜色纹理。

典型流程：

```text
绑定 Offscreen RenderTarget
    → 设置 viewport / depth / cull
    → 绘制 3D 场景
    → 恢复主 RenderTarget
    → 在 2D UI 中使用颜色纹理
```

当前 Pass 模型保持显式调用和固定顺序，不引入通用 RenderGraph。

---

## 11. Platform、Window 与输入

### 11.1 Platform

Platform 定义跨平台生命周期：

```text
initialize
    → beginFrame
    → dispatchEvents
    → beginRenderPass
    → updateWidgets
    → lateUpdateWidgets
    → commitRenderPass
    → endFrame
    → terminate
```

平台实现包括：

| 平台 | 实现 |
|---|---|
| Windows | GLFW / WGL |
| QNX | EGL |

Platform 负责窗口系统、图形上下文和输入接入，不实现 UI 合批策略。

### 11.2 Window

Window 当前同时承担：

- 平台窗口对应的 UI 根节点；
- Widget 树更新入口；
- 当前窗口 BatchManager 所有者；
- framebuffer、viewport、clear 和 Present 协调。

多窗口场景下，每个 Window 应保持独立的：

- Widget 根；
- BatchManager；
- viewport；
- RenderTarget 状态；
- 输入目标范围。

### 11.3 输入捕获

Platform 根据当前 Widget 树解析命中目标，并维护 pointer capture。

输入系统与渲染系统共享 Widget 的可见性和几何状态，但不直接依赖 GPU 对象。

---

## 12. 所有权与线程边界

### 12.1 主要对象所有权

| 对象 | 所有者 / 生命周期 |
|---|---|
| Engine | Application |
| Platform / Window | Engine / Platform |
| Widget / Component | Widget 树 |
| Material / Mesh / Texture | UI 对象或资源管理器 |
| BatchManager | Window |
| RenderItem | BatchManager 帧内列表 |
| CommandBuffer | RenderDeviceProxy frame slot |
| GPU wrapper | 对应 Material、Mesh、Texture 或专用对象 |
| native GPU object | 图形上下文线程 |
| 上传 payload | 资源对象、共享 owner 或命令 payload |

### 12.2 主线程可写对象

主线程负责修改：

```text
Widget
Component
Transform
Material
Mesh
Texture CPU 状态
Layout / Animation / Interaction
```

### 12.3 渲染线程可执行对象

渲染线程负责：

```text
CommandBuffer
GLRenderDevice
native GPU object
Fence
Present
```

### 12.4 跨线程规则

1. 主线程不能在渲染线程执行命令时修改命令 payload；
2. 命令不能引用提前失效的栈变量；
3. 原始指针数据若不能保证生命周期，必须复制或附带 owner；
4. GPU wrapper 的删除必须通过渲染命令进入图形上下文线程；
5. Widget 和 Component 不由渲染线程遍历或修改；
6. 多线程模式下不得绕过 RenderDeviceProxy 直接调用 GLRenderDevice。

---

## 13. 运行时服务

当前公共服务包括：

- RenderingThread；
- TextureManager；
- FontManager；
- SSBOManager。

`GlobalObject` 提供现有全局访问入口。

公共服务只应保存真正跨模块共享的长期对象，不应演变为任意状态的全局存储。

高频渲染路径应优先通过明确参数、Window 上下文或 FrameState 获取依赖，减少重复全局查询。

---

## 14. 架构不变量

以下约束应长期保持：

1. 项目采用保留式 Widget 树，不转为每帧重建全部 UI 的立即模式；
2. Widget 到 GPU 提交之间保持短路径，不建立完整场景快照副本；
3. OpenGL/OpenGL ES 只能在拥有图形上下文的线程执行；
4. 多线程模式下所有 GPU 操作通过 RenderDeviceProxy 编码；
5. 已编码命令不能引用可能提前失效的数据；
6. CPU 几何和纹理数据未变化时不得重复上传；
7. UI 默认保持 painter's order；
8. 批次兼容不等于允许跨元素重排；
9. 透明、Clip、Stencil、Depth 和 RenderTarget 语义不能因合批改变；
10. 高频路径避免无界堆分配、字符串比较和全局锁；
11. GPU 资源生命周期由明确对象所有权和命令顺序管理；
12. 新增抽象必须由可测量的问题驱动；
13. 固定容量缓冲区必须提供峰值、溢出和失败诊断；
14. 单线程与多线程路径应保持一致的渲染结果。

---

## 15. 后续优化方向

本章只记录尚未完成或仍需持续推进的工作。

### 15.1 RenderItem 热路径紧凑化

当前 RenderItem 和 BatchCompatibilityKey 仍包含智能指针、字符串和较宽的状态字段。

优化方向：

- 收集阶段减少 `shared_ptr` 拷贝；
- 将稳定 Shader、Texture、Material 和 Layout 信息缓存为整数 ID；
- 避免每帧构造和比较 Shader 名称字符串；
- 将高频字段组织为连续紧凑结构；
- 评估 SoA 布局对遍历、排序和合批的收益；
- 复用 RenderItem 容量，避免帧间重复分配。

### 15.2 Widget 更新与渲染收集裁剪

保留式树的主要价值是跳过未变化工作。

优化方向：

- 完善 Layout、Transform、Geometry、Material 和 Visibility Dirty；
- 不可见子树直接停止 update 和渲染收集；
- 未变化 Transform 避免重复计算世界矩阵；
- 未变化文本和几何避免重建 Mesh；
- 仅在结构或渲染状态变化时更新批次 revision；
- 对大型 UI 树增加轻量可见区域裁剪。

### 15.3 合批 Key 和批次缓存

优化方向：

- 缓存 Material 和 Mesh 的 BatchCompatibilityKey；
- revision 未变化时直接复用 key；
- 用整数和 bit field 代替高频字符串、指针组合比较；
- 减少 `unordered_map` 遍历对纹理集合 hash 的影响；
- 记录每种断批原因和对应 Draw Call 成本；
- 根据真实场景数据决定是否引入分层排序。

### 15.4 非 SSBO 路径减少 Draw Call

标准路径需要在不支持 SSBO 的设备上保持较低 Draw Call。

优化方向：

- 使用实例化顶点属性传递 per-object 数据；
- 按设备能力选择 UBO array、instance attribute 或逐项 Uniform；
- 合并兼容静态几何；
- 减少重复 Material apply 和纹理绑定；
- 保留逐对象绘制作为正确性回退路径。

### 15.5 安全重排

默认 painter's order 会限制批次规模。

仅在具备明确证据时进行重排：

- RenderItem 提供屏幕空间 bounds；
- 标记 opaque、reorderable 和 depth-independent；
- 判断元素是否重叠；
- 不跨越 Clip、Stencil、RenderTarget 和透明边界；
- 为重排前后结果建立图像回归测试。

### 15.6 Clip 与 Stencil

优化方向：

- 将 Clip Rect 和 Stencil 状态纳入实际提交；
- 明确嵌套裁剪的 push/pop 生命周期；
- Clip 或 Stencil 变化产生确定的断批；
- 优先使用 Scissor 处理矩形裁剪；
- 复杂路径裁剪使用 Stencil；
- 避免裁剪状态泄漏到后续批次。

### 15.7 纹理和 Buffer 上传

不引入通用上传队列，继续基于资源对象和 RenderDevice 命令优化。

优化方向：

- 纹理上传数据统一明确 owner；
- 原始指针路径使用上传缓冲池；
- 支持局部纹理更新；
- 大纹理上传控制单帧 CPU 拷贝峰值；
- 动态 VBO 使用 orphaning、ring buffer 或 persistent mapping；
- 静态 Buffer 避免重复更新；
- 记录每帧上传字节数和上传耗时。

### 15.8 GPU 资源异步销毁

优化方向：

- 明确各 GPU wrapper 的析构线程；
- 多线程模式统一编码删除命令；
- 防止对象析构与未执行命令之间出现悬空引用；
- 对需要 GPU 完成后才能释放的数据使用 Fence；
- 统一处理 Engine 退出时的命令排空和上下文销毁顺序；
- 为反复创建和销毁 Texture、Buffer、Shader、RenderTarget 增加压力测试。

### 15.9 CommandBuffer 与环形帧槽

优化方向：

- 记录每帧命令字节数和峰值；
- 记录环形缓冲等待次数和等待时长；
- 为 CommandBuffer 溢出提供明确错误；
- 减少非平凡 command payload；
- 合并细粒度状态命令；
- 在编码端增加冗余状态过滤；
- 根据目标设备调整 frame slot 数量和容量。

### 15.10 GPU 状态缓存

优化方向：

- 缓存当前 Shader、Texture、VBO、RenderTarget；
- 缓存 Blend、Depth、Cull、Viewport 和 Scissor；
- 跳过重复状态设置；
- 在 RenderTarget 切换后明确失效相关缓存；
- Debug 模式提供状态一致性校验。

### 15.11 2D/3D Pass 编排

保持显式 Pass，不提前引入 RenderGraph。

优化方向：

- 明确 UI、3D、Offscreen 和 Debug 的执行顺序；
- 统一 RenderTarget、viewport、clear 和状态恢复；
- 减少不必要的 FBO 切换；
- 复用尺寸相同的 Offscreen RenderTarget；
- 记录每种 Pass 的 CPU/GPU 时间；
- 仅在出现跨 Pass 依赖和 transient resource 复用需求后评估 RenderGraph。

### 15.12 3D GUI 性能

优化方向：

- 3D SceneView 视锥裁剪；
- 静态 Mesh 和 Material 状态复用；
- GLTF 场景节点 Dirty 更新；
- 3D 视图按实际变化决定是否重绘；
- Offscreen 分辨率按显示尺寸和设备能力调整；
- IBL 和光照资源跨视图共享；
- 大量相同 Mesh 使用实例化绘制。

### 15.13 文本与字体

优化方向：

- 字形 Atlas 分页和回收；
- 文本布局结果缓存；
- 未变化文本避免重新生成几何；
- 相同字体和 Atlas 的文本连续合批；
- 降低多语言和动态字号导致的 Atlas 抖动；
- 记录字形上传、Atlas 命中和文本重建统计。

### 15.14 FrameState 与全局依赖

优化方向：

- 区分只读帧参数和可写统计；
- 减少 Component 从 FrameState 获取无关服务；
- 将高频服务通过明确上下文传递；
- 收敛 `GlobalObject` 的使用范围；
- 明确 Engine、Platform、Window 和 RenderingThread 的销毁顺序；
- 提升模块可测试性。

### 15.15 Window 与 UI 根节点解耦

当前 Window 同时是平台窗口和 UIWidget。

后续可在多窗口、嵌入式 Surface 或离屏 UI 需求出现时评估：

- Window 组合独立 UI root；
- 平台窗口只负责 surface、context、size 和 Present；
- UI root 负责 Widget 树和 BatchManager；
- 一个 UI root 可绑定不同输出目标；
- 避免在需求出现前提前进行大规模重构。

### 15.16 性能时间线

需要建立统一、低开销的性能统计：

- Input；
- Animation；
- Widget update；
- lateUpdate；
- RenderItem collection；
- Batch build；
- Command encode；
- Render thread execute；
- Texture / Buffer upload；
- Present；
- 各 3D/Offscreen Pass GPU 时间；
- 环形缓冲等待；
- Draw Call、Triangle 和上传字节数。

统计结果应支持固定场景导出和版本间对比。

### 15.17 渲染回归测试

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

### 15.18 压力测试与 CI

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

---

## 16. 优化决策标准

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
