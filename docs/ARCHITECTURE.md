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

### 7.7 Underlay（底层渲染项）

部分效果（如阴影）必须在所属对象之前绘制，才能被主对象正确覆盖。`BatchManager` 通过 underlay 机制保证这类底层渲染项的绘制顺序：

- 组件通过 `addRenderable(material, meshFilter, transform, /*underlay=*/true)` 提交底层渲染项；
- `BatchManager` 将底层项与普通项分容器收集（`m_underlayRenderables` / `m_renderables`），合批时**先构建并先绘制底层项批次**，再处理普通项；
- 底层项仍参与合批（相同 shader/状态的底层项可合并）、增量合批比对与批次统计，不脱离通用渲染管线。

典型应用：`Shadow` 组件复用原始对象的 `MeshFilter` 与 `MeshRenderer`，以 `shadow` shader 和阴影材质提交一个 underlay 渲染项；几何偏移在 `shadow.vert` 内通过 `u_shadowOffset` 完成（对象空间平移），无需 CPU 修改世界矩阵。因此阴影：

- 不会绕过 `BatchManager` 直接绘制，draw call 与批次受统一管理；
- 保证先于原对象绘制，半透明阴影被主对象正确覆盖；
- 与原对象使用不同 shader，合批 key 自动隔离，不会与普通 UI 混批。

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

状态标记：

- ✅：已完成或当前方案已收敛；
- ⏸️：保留为需求驱动的候选项，不属于短期计划；
- 🔜：结合当前代码热路径确认值得近期推进。

原有 Tier 仅表示问题类别，不再表示当前实施优先级。除已完成项外，15.1～15.18
中原有的剩余方向统一降为非短期候选；近期建议以 15.19 的代码审查结果为准。

---

#### Tier 1 — 工程质量基础

### 15.1 渲染回归测试 ⏸️

> 当前状态：非短期。出现明确渲染兼容性风险或跨平台回归后再扩展。

当前已有 `BatchBuilderTests.cpp` 和 `BatchCompatibilityKeyCacheTests.cpp` 两组纯 CPU
单元测试，但无渲染级回归覆盖。

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

### 15.2 压力测试与 CI ⏸️

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

### 15.3 性能时间线 ⏸️

> 当前状态：非短期。短期优化先使用固定场景、现有 Batch 统计和局部计时验证。

已有：`DebugPlane` 显示 FPS、Batch 统计（cache hit/miss、break reason 计数、Draw Call 计数）。缺失各阶段耗时分离。

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

#### Tier 2 — CPU 关键优化

### 15.4 Widget Dirty 体系完善 ✅

已有：`Transform` 通过局部数据版本、局部矩阵版本、世界矩阵使用的局部版本及父节点世界版本进行缓存失效检测；`Widget` 不可见子树跳过 update；`MRLabel` 双层文本脏标记；`Mesh::m_revision` 驱动 VBO 跳过上传；`Material::m_batchCompatibilityRevision` 驱动合批 key 缓存失效，`Material::m_uniformRevision` 独立记录普通 uniform 数据变化。当前不建议强行引入跨模块统一 Dirty 枚举，主要问题应转为明确各级缓存的失效边界，避免“数据变化”与“结构变化”混用。

优化方向：

1. **不再推进跨模块统一 Dirty 枚举**：**方案调整，不作为当前优化目标**。`Mesh::m_revision` 已经负责判断对应 `VertexArray` 是否需要重新上传 VBO；`BatchCompatibilityKey` 已经负责判断 shader / texture / blend / topology / vertex layout / clip 等批次结构是否兼容；`Transform` 版本号负责局部矩阵和世界矩阵缓存；`MRLabel` 的文本排版脏标记只服务于文本布局阶段。它们的缓存粒度、生命周期和消费者不同，统一为一个 `DirtyFlag` 反而会引入跨层依赖，并不能减少实际判断。保留各模块独立的 dirty/revision 机制，仅要求命名、注释和失效契约清晰即可。**已完成（方案收敛）**。
2. **世界矩阵独立 dirty 判定**：**已完成**。`Transform` 已移除 `m_matrixDirty` 与 `m_worldMatrixDirty`，改用版本号判断局部矩阵和世界矩阵是否失效；`getWorldMatrix()` 会比较父节点身份及父节点世界版本，未失效时直接返回缓存矩阵。父节点版本读取前会先刷新父世界矩阵，以确保祖先节点变化可以正确向下传播。
3. **批次结构变化与资源上传变化解耦**：**已完成**。`RenderItem::isSameRenderableAs()` 不再比较 `materialRevision`、`geometryRevision` 等资源内容版本，而是比较渲染项身份、`displayLayer` 和 `BatchCompatibilityKey`；Mesh 内容变化继续由 `Mesh::m_revision` / `VertexArray::needsMeshUpload()` 负责 VBO 上传，材质参数变化继续由 `Material::apply()` 负责 uniform 提交。只有影响批次兼容性的 shader、纹理集合、blend / cull、primitive topology、vertex layout、clip / stencil 等结构变化才会触发批次重建；对应 BatchBuilder 测试已覆盖资源内容变化不使批次结构失效、兼容性 key 变化能够使缓存失效。

> 说明：引擎面向全屏 GUI 渲染，Widget 总是在视口内绘制，不存在屏幕外可见区域剔除（视口裁剪）需求，该方向不作为优化目标。

### 15.5 GPU 状态缓存 ✅

已实现：`GLRenderDevice` 内建 `GLStateCache` 结构，缓存当前 Shader、Texture（每单元）、Blend、Viewport、DepthTest、DepthWrite、CullFace 状态。每次状态设置前先比较缓存值，相同则跳过 GL 调用。`drawVBO` 和 `bindRenderTarget`/`unbindRenderTarget` 自动同步缓存。

---

#### Tier 3 — 框架补全

### 15.6 合批 Key 缓存 ✅

已完成：`Material` 和 `Mesh` 分别缓存自身的 `BatchCompatibilityKey` hash 部分。Material 使用 `m_batchCompatibilityRevision` 和 `m_uniformRevision` 分离合批兼容状态与普通 uniform 数据变化；当前合批缓存只消费前者，后者预留给后续 uniform dirty/UBO 上传刷新机制。Mesh 因资源 revision 还承担 VBO 上传判断，额外使用 batch compatibility revision。对应合批 revision 未变化时，`createBatchKey()` 直接复用缓存结果，不再重复遍历 Material 纹理集合或重算 Mesh layout hash。

失效边界：

- Material 的 texture、shader、blend 和 double-sided 状态变化时失效；普通 uniform 参数变化不影响缓存；
- Mesh 的 primitive topology 或 vertex layout 变化时失效；顶点、索引和 attribute 内容变化不影响缓存；
- `shaderName` 保持现有 string 表示，本阶段不改为整数 ID；
- 保持现有绘制顺序和合批策略，本阶段不引入分层排序。

对应 CPU 测试覆盖缓存复用，以及 blend、topology、vertex layout 变化时的正确失效。该项按当前方案全部完成。

### 15.7 RenderItem 热路径紧凑化 ⏸️

> 当前状态：非短期。SoA、整数 ID 和所有权调整仅在 profiling 证明 RenderItem
> 遍历或引用计数为主要瓶颈后推进。

已有：`BatchCompatibilityKey` 已大量使用整数/位域；`RenderItem` 列表通过 swap 复用容量；`RenderBatchPool` 按 shader 键池化。缺失 SoA 布局和指针消除。

优化方向：

- 收集阶段减少 `shared_ptr` 拷贝（`addRenderable` 当前按值拷贝增加引用计数）；
- 将稳定 Shader、Texture、Material 和 Layout 信息缓存为整数 ID；
- 将高频字段组织为连续紧凑结构；
- 评估 SoA 布局对遍历、排序和合批的收益。

### 15.8 Clip 与 Stencil ⏸️

> 当前状态：非短期。属于功能补全，不作为当前性能优化目标。

已有：`BatchCompatibilityKey` 包含 `clipStateId`/`stencilStateId`；`BatchBreakReason::ClipState` 产生断批；`DebugPlane` 显示断批次数。缺失底层 GL 状态实现。

优化方向：

- 将 Clip Rect 和 Stencil 状态纳入实际 GL 提交（`glScissor` / `glStencilFunc` / `glStencilOp`）；
- 明确嵌套裁剪的 push/pop 生命周期；
- 优先使用 Scissor 处理矩形裁剪；
- 复杂路径裁剪使用 Stencil；
- 避免裁剪状态泄漏到后续批次。

---

#### Tier 4 — GPU 优化

### 15.9 3D GUI 性能 ⏸️

> 当前状态：原列表整体非短期。近期只推进 15.19 中范围更明确的
> Transform3D 缓存和 SceneView 按变化重绘。

已有：IBL 资源可通过 `Scene3DPassContext` 跨视图共享；Offscreen 分辨率可动态调整。缺失视锥裁剪和 Dirty 体系。

优化方向：

- 3D SceneView 视锥裁剪；
- 静态 Mesh 和 Material 状态复用；
- GLTF 场景节点 Dirty 更新（当前每帧全量 update）；
- 3D 视图按实际变化决定是否重绘；
- 大量相同 Mesh 使用实例化绘制。

### 15.10 纹理和 Buffer 上传 ⏸️

> 当前状态：非短期。除非目标设备 profiling 明确显示上传阻塞或带宽峰值问题。

已有：`PixelDataRecyclePool` 上传缓冲池；`shared_ptr` owner 传递避免 memcpy；`glTexSubImage2D` 局部更新；`VBODataRecyclePool` Fence 回收。缺失统计和高级 buffer 策略。

优化方向：

- 记录每帧上传字节数和上传耗时；
- 大纹理上传控制单帧 CPU 拷贝峰值；
- 动态 VBO 使用 orphaning、ring buffer 或 persistent mapping（当前使用 `glBufferData + GL_STATIC_DRAW`）；
- 支持局部纹理更新（`glTexSubImage2D` 已支持）。

---

#### Tier 5 — 架构演进（需需求驱动）

### 15.11 安全重排 ⏸️

> 当前状态：非短期。当前继续严格保持 painter's order。

`BatchBuilder` 当前 `preservePainterOrder = true` 硬编码。仅在 painter's order 严重限制批次规模时考虑。

需要引入：

- RenderItem 提供屏幕空间 bounds；
- 标记 opaque、reorderable 和 depth-independent；
- 判断元素是否重叠；
- 不跨越 Clip、Stencil、RenderTarget 和透明边界；
- 为重排前后结果建立图像回归测试。

### 15.12 非 SSBO 路径减少 Draw Call ⏸️

> 当前状态：非短期。SSBO 仍是主要路径，标准路径只承担正确性回退。

已有：SSBO 路径（实例化绘制 + per-object SSBO 数据）；标准路径（逐对象 `material->apply()` + `drawVBO`）；SSBO fallback。SSBO 已是主要优化路径，标准路径作为回退足够。

仅在需要支持无 SSBO 能力的设备时考虑：

- 使用实例化顶点属性传递 per-object 数据；
- 按设备能力选择 UBO array、instance attribute 或逐项 Uniform；
- 合并兼容静态几何；
- 保留逐对象绘制作为正确性回退路径。

### 15.13 2D/3D Pass 编排 ⏸️

> 当前状态：非短期。不提前引入 RenderGraph 或统一 transient resource 系统。

已有：`Engine::render()` 固定顺序（begin→2D update→lateUpdate→commit）；`MR3DSceneView` 自行管理 FBO 子 Pass 并恢复 GL 状态。缺失统一管理和统计。

优化方向：

- 明确 UI、3D、Offscreen 和 Debug 的执行顺序；
- 统一 RenderTarget、viewport、clear 和状态恢复；
- 减少不必要的 FBO 切换；
- 复用尺寸相同的 Offscreen RenderTarget；
- 记录每种 Pass 的 CPU/GPU 时间；
- 仅在出现跨 Pass 依赖和 transient resource 复用需求后评估 RenderGraph。

### 15.14 FrameState 与全局依赖 ⏸️

> 当前状态：非短期。仅在相关模块修改时局部收敛依赖，不进行全局重构。

已有：`FrameState` 携带相机、BatchManager、SSBOManager 等服务引用。缺失依赖注入和可测试性。

优化方向：

- 区分只读帧参数和可写统计；
- 减少 Component 从 FrameState 获取无关服务；
- 将高频服务通过明确上下文传递，减少 `GlobalObject::getInstance()` 全局查找（当前 16 处直接调用）；
- 收敛 `GlobalObject` 的使用范围；
- 明确 Engine、Platform、Window 和 RenderingThread 的销毁顺序；
- 提升模块可测试性。

### 15.15 Window 与 UI 根节点解耦 ⏸️

> 当前状态：非短期。等待多窗口、嵌入式 Surface 或离屏 UI 的明确需求。

当前 `Window` 直接继承 `UIWidget`，同时承担平台窗口和 UI 根节点职责。仅在多窗口、嵌入式 Surface 或离屏 UI 需求出现时评估：

- Window 组合独立 UI root；
- 平台窗口只负责 surface、context、size 和 Present；
- UI root 负责 Widget 树和 BatchManager；
- 一个 UI root 可绑定不同输出目标；
- 避免在需求出现前提前进行大规模重构。

---

#### Tier 6 — 基础扎实 / 已完成

### 15.16 GPU 资源异步销毁 ✅

已有：删除操作通过 `Cmd_DeleteVBO/Cmd_DeleteTexture2D/Cmd_DeleteGPUProgram/Cmd_DeleteRenderTarget` 编码到 CommandBuffer；`PendingFrame` 携带 Fence，仅在 GPU 完成后回池；像素缓冲立即回收；`~RenderDeviceProxyBase()` 设置 quit 信号并 join 渲染线程。

剩余工作（⏸️ 非短期）：

- 明确各 GPU wrapper 的析构线程文档；
- 为反复创建和销毁 Texture、Buffer、Shader、RenderTarget 增加压力测试。

### 15.17 CommandBuffer 与环形帧槽 ✅

已有：3 槽环形缓冲（`kRingSize=3`），信号量控制主线程等待；`CommandBuffer` 固定 16MB，`push<T>()` 零分配编码；命令字节数可通过 `size()` 获取。

剩余工作（⏸️ 非短期）：

- 记录每帧命令字节数峰值、环形缓冲等待次数和等待时长；
- 为 CommandBuffer 溢出提供明确错误（当前依赖 debug assert）；
- 在编码端增加冗余状态过滤；
- 根据目标设备调整 frame slot 数量和容量。

### 15.18 文本与字体 ⏸️

> 当前状态：非短期。现有文本 dirty 和连续合批满足当前目标，Atlas 分页等能力
> 在多语言大字符集场景出现明确压力后推进。

已实现：~~未变化文本避免重新生成几何~~（MRLabel 双层脏标记 + Mesh revision 跳过链）、~~相同字体和 Atlas 的文本连续合批~~（BatchCompatibilityKey 包含 Shader + Texture 集合）。

尚未实现：

- 字形 Atlas 分页和回收（当前单图集 + 扩容全量重建，O(已有字形数)）；
- 文本布局结果缓存（跨实例复用）；
- 降低多语言和动态字号导致的 Atlas 抖动（扩容丢弃旧纹理）；
- 记录字形上传、Atlas 命中和文本重建统计。

---

### 15.19 近期高价值优化建议 🔜

以下方向来自 2026-08-04 对当前代码热路径的审查，目标是优先减少静态 GUI 的
持续消耗、3D 子场景重复计算和确定性的资源滞留。排序依据是收益、实现范围和
对现有架构的影响，不要求一次全部实施。

#### P0：Material 纹理所有权收敛 ✅

已完成：`Material::setTexture()` 只维护 `m_textureMap`，已删除没有读取点的
`m_textures` 和对应线性查找。替换同名 sampler 的纹理后，旧纹理不再被 Material
额外持有。

对应测试覆盖当前纹理由 Material 持有，以及替换后旧纹理在无其他 owner 时可以
正常析构。`m_batchCompatibilityRevision` 的失效行为保持不变。

#### P0：按需渲染链路闭环

当前 `EngineOptions::enableRequestRender` 只保存到 `Engine::m_requestRenderEnabled`；
`RenderingThread::m_needRender` 虽然支持 set/reset，但渲染循环没有读取该状态。
Windows 和 QNX 平台仍会每轮执行 Widget update、Batch、Present。对大部分时间
静止的车载 GUI，这是最直接的 CPU/GPU 和功耗浪费。

建议：

- 由 Engine 在帧入口统一判断输入、resize、`REQUESTRENDER` 和持续动画状态；
- 无变化时不执行 Widget update、3D Pass、Batch、Present，并使用平台等待机制
  避免空轮询；
- 将 render request 改为线程安全标志，并保证异步加载、视频帧和输入可以唤醒；
- Tween、GLTF animation 和流式纹理显式提供“需要持续帧”状态；
- `maxFrames` 统计实际渲染帧，而不是空闲循环次数。

验收：静态场景完成首帧后不再产生 Draw Call/Present；输入和属性变化能在一个
目标帧周期内唤醒；Tween、GLTF 动画和视频流不中断。

#### P1：Transform3D 世界矩阵版本缓存

当前 `Transform3D::getWorldTransformMatrix()` 每次调用都会通过组件表查找父
`Transform3D`，递归获取父矩阵并重新执行矩阵乘法，即使整个 3D 层级没有变化。
这与 2D `Transform` 已有的版本缓存能力不一致。

建议：

- 为 Transform3D 增加 local/world revision；
- 缓存父 Transform 身份和上次使用的父 world revision；
- local revision 和父 world revision 均未变化时直接返回世界矩阵；
- GLTF 动画只使实际修改节点及其依赖子树失效。

验收：静态 3D 层级预热后世界矩阵重算次数为 0；修改一个节点时只重算该节点
及其后代，渲染结果与现有实现一致。

#### P1：MR3DSceneView 按变化重绘

当前 `MR3DSceneView::update()` 每帧都会绑定并清空 FBO、更新 frame UBO、遍历
完整 SceneNode 树，还会为局部 3D FrameState 创建新的 `shared_ptr`。静态模型
即使画面没有变化也会完整重绘。

建议：

- 建立 scene、camera、lighting、IBL、animation 和 FBO size revision；
- 仅在任一 revision 变化时执行 3D FBO Pass；
- 未变化时直接复用上次 FBO color texture，只执行必要的 2D composite；
- 复用局部 FrameState，消除每帧 `make_shared<FrameState>`；
- 与按需渲染联动：活跃 GLTF 动画保持 3D Pass 连续更新，暂停后自动静止。

验收：静态 3D SceneView 首帧后不再产生 3D Draw Call 和 frame UBO 更新；相机、
灯光、模型或 FBO 尺寸变化会准确触发一次重绘；动画期间保持连续更新。

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

当前 `SSBOLayout::filler` 在每个实例上通过 `getFloat("...")`、
`getVector("...")` 查询 Material 的字符串 map，并通过 `std::function` 间接调用。
大量 Widget 时，这部分可能成为 CPU 热点，但应先使用固定场景确认占比。

建议：

- 为内建 shader 使用类型化 instance payload 或稳定字段句柄；
- Material revision 未变化时复用已解析的 per-instance 常量；
- 保留 Transform matrix 等真正逐帧变化的数据直接写入；
- 评估 `ShaderStorageBuffer` 数据容量复用，避免每批每帧重复获取和 resize DTO。

验收：仅在 profiling 显示 SSBO 填充占用显著 CPU 时间后实施；优化后比较相同
RenderItem 数量下的 collection/pack 时间、命令字节数和内存峰值。

近期不建议推进 SoA、RenderGraph、安全重排、Window/UI root 解耦、非 SSBO
复杂实例化和字体 Atlas 分页。这些方向保留在前述非短期清单中，由真实需求和
profiling 数据触发。

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
