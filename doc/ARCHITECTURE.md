# MorrowUI 引擎架构文档

## 一、概览

MorrowUI 是一个自研的跨平台 2D/3D 混合 UI 渲染引擎，基于 OpenGL ES / OpenGL，支持 Windows (WGL/GLFW) 与 QNX (EGL) 双平台。引擎采用 **ECS 组件架构** + **树形场景图** + **平台抽象层** 的分层设计，支持多线程异步渲染。

### 核心设计目标

- **跨平台**：一套代码，Windows (WGL) 与 QNX (EGL) 双平台运行
- **2D/3D 混合渲染**：原生支持 UI 控件与 3D 场景（GLTF）混合渲染
- **高性能**：实例化渲染 + 合批 + SSBO + 多线程异步渲染
- **组件化**：ECS 架构，Widget 行为通过组件组合，易扩展

---

## 二、分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Application / Samples                   │
├─────────────────────────────────────────────────────────────┤
│  UI Layer (src/ui/)                                          │
│  ┌──────────┐ ┌──────────────┐ ┌──────────┐ ┌───────────┐  │
│  │ Elements │ │   Layout     │ │  Helpers │ │    HMI    │  │
│  │ MRImage  │ │ HBoxContainer│ │  Tween   │ │SafeSprite │  │
│  │ MRLabel  │ │ VBoxContainer│ │3DLoader  │ │SafeCanvas │  │
│  │ MRButton │ │GridContainer │ │          │ │           │  │
│  └──────────┘ └──────────────┘ └──────────┘ └───────────┘  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Base (ECS + Scene Graph)                │   │
│  │  Widget → UIWidget → [Transform, MeshRenderer, ...]  │   │
│  │  Component / ComponentManager                        │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Renderer Layer (src/renderer/)                              │
│  ┌────────────────┐ ┌──────────────┐ ┌──────────────────┐   │
│  │  RenderDevice   │ │ CommandBuffer│ │   FrameState     │   │
│  │  (GPU抽象接口)  │ │ (零分配命令) │ │  (逐帧状态)      │   │
│  └───────┬────────┘ └──────────────┘ └──────────────────┘   │
│  ┌───────┴──────────────────────────────────────────────┐   │
│  │  RenderDeviceProxy (异步渲染代理 + 三缓冲RingBuffer) │   │
│  │  RenderingThread (渲染线程管理)                       │   │
│  │  GLRenderDevice (OpenGL实现)                          │   │
│  └──────────────────────────────────────────────────────┘   │
│  GPU资源: VBO/UBO/SSBO/Texture2D/Shader/FBO/VAO            │
├─────────────────────────────────────────────────────────────┤
│  Platform Layer (src/platform/)                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Platform (抽象)  ←  PlatformFactory                  │   │
│  │  ├── WGLPlatform  (Windows/GLFW)                     │   │
│  │  └── QNXPlatform (QNX/EGL)                            │   │
│  │  Window / InputEventsManager / InputProvider          │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Core Layer (src/core/)                                      │
│  ┌──────────┐ ┌────────┐ ┌──────────┐ ┌───────────────┐   │
│  │  Engine   │ │ Camera │ │FPSControl│ │ GlobalObject   │   │
│  │ (主循环)  │ │        │ │          │ │ (服务定位器)   │   │
│  └──────────┘ └────────┘ └──────────┘ └───────────────┘   │
│  BatchManager / Observable / CommandQueue / Thread          │
├─────────────────────────────────────────────────────────────┤
│  Math / Fonts / GLTF / Debug                                │
│  Vector2/3/4, Matrix3/4, Quaternion, Rect                  │
│  FontManager, GLTFLoader, DebugPlane                        │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、核心子系统详解

### 3.1 Engine — 引擎主循环

**文件**: `src/core/Engine.h`, `src/core/Engine.cpp`

Engine 是整个引擎的入口和驱动器，负责：

| 职责 | 说明 |
|------|------|
| 平台初始化 | 通过 `PlatformFactory::create()` 创建平台实例 |
| 相机管理 | 创建 `OrthographicCamera`，设置初始位置 (0,0,10000) |
| 帧率控制 | 通过 `FPSController` 控制目标帧率 |
| 主循环 | `render()` 中的 while 循环驱动每帧更新 |
| 生命周期钩子 | `preRender()` / `afterRender()` 的 Observable 通知 |
| 心跳 | `heartbeat()` 每 2s 统计实际 FPS |

**渲染主循环流程**（`Engine::render()`）—— 7 阶段显式编排：

```
while (!platform->shouldClose()):
  updateFrameState()                   ← 更新 deltaTime，重置逐帧计数器

  // ── 阶段 1: 输入 ──
  platform->beginFrame()               ← poll 输入事件 + resolve hit-test 目标
  platform->dispatchEvents()           ← 将触摸事件派发到目标 Widget

  // ── 阶段 2: 动画准备 ──
  m_preRender.notify()                 ← 通知 preRender 观察者
  TweenManager::update()               ← 更新所有动画/补间

  // ── 阶段 3: 渲染管线 ──
  platform->beginRenderPass()          ← GPU 准备：setViewport / clear / 绑定 framebuffer
  platform->updateWidgets()            ← Widget 树遍历：Component::update()（纯 CPU）
  platform->lateUpdateWidgets()        ← Widget 树遍历：Component::lateUpdate()（纯 CPU）
  platform->commitRenderPass()         ← GPU 提交：合批 → draw call

  // ── 阶段 4: 帧后处理 ──
  heartbeat()                          ← 每 2s 计算实际 FPS
  debugPlane->update()                 ← Debug 面板渲染
  m_afterRender.notify()               ← 通知 afterRender 观察者
  platform->endFrame()                 ← SwapBuffers / Present
  callAfterRenderFunctions()           ← 执行延迟回调
```

**EngineOptions 配置**:
- `multithread` (默认 true): 是否启用多线程异步渲染
- `enableRequestRender` (默认 false): 是否启用按需渲染
- `samples` (默认 1): MSAA 采样数
- `windowInfo`: 窗口配置（尺寸、位置、zorder 等）

---

### 3.2 Platform — 平台抽象层

**文件**: `src/platform/Platform.h`

Platform 是平台抽象基类，定义了跨平台的生命周期接口。渲染管线拆分为 4 个独立阶段，由 Engine 显式编排：

```
initialize() → [beginFrame() → dispatchEvents() → beginRenderPass() → updateWidgets() → commitRenderPass() → endFrame()] × N → terminate()
```

| 方法 | 职责 |
|------|------|
| `initialize(bool multithread)` | 创建窗口、GL 上下文、输入提供者（渲染线程由 Engine 在 initialize 之前启动） |
| `beginFrame(frameState)` | 拉取输入事件、设置 GL 上下文 |
| `dispatchEvents(frameState)` | 将输入事件派发到 hit-test 目标 Widget（Platform 基类统一实现） |
| `beginRenderPass(frameState)` | GPU 准备：sync 窗口尺寸、setViewport、clear、绑定 framebuffer |
| `updateWidgets(frameState)` | Widget 树递归遍历（纯 CPU，Platform 基类实现，委托给 Window） |
| `commitRenderPass(frameState)` | GPU 提交：合批渲染 → draw call |
| `endFrame()` | SwapBuffers / eglSwapBuffers |
| `terminate()` | 销毁 GL 上下文、窗口 |

**平台实现**：

| 平台 | 类 | 目录 |
|------|-----|------|
| Windows (GLFW + WGL) | `WGLPlatform` | `src/platform/wgl/` |
| QNX (EGL) | `QNXPlatform` | `src/platform/egl/` |

**PlatformFactory** 通过编译期宏选择平台：
```cpp
#ifdef OPENGL_EGL    → QNXPlatform
#ifdef OPENGL_GLFW   → WGLPlatform
```

**输入系统**:
- `InputProvider` 接口：平台相关的事件源（鼠标/触摸/键盘）
- `InputEventsManager`：统一管理输入事件，通过 `Observable<std::vector<TouchEvent>&>` 分发
- `Platform` 基类负责 `resolveInputTargets()` —— 在 Widget 树中查找触摸命中目标

**Window 抽象**:
- `Window` 继承自 `UIWidget`，本身是 Widget 树的根节点
- 渲染接口拆分为 `beginRenderPass` / `updateWidgets` / `commitRenderPass`，子类各自实现 GPU 部分，Widget 树遍历由基类统一实现
- `WindowInfo` 包含窗口名、位置、尺寸、samples、displayId、zorder 等属性
- `WGLWindow` / `EGLWindow` 为平台实现

---

### 3.3 Renderer — 渲染层

#### 3.3.1 RenderDevice（GPU 抽象接口）

**文件**: `src/renderer/RenderDevice.h`

纯虚接口，抽象所有 GPU 操作，与具体图形 API 解耦：

| 资源类型 | 操作 |
|----------|------|
| **VBO** | create / update / draw / delete |
| **Texture2D** | create / update / subUpdate / use / delete |
| **GPUProgram** (Shader) | create / use / setUniform (int/float/vec2/3/4/mat4/array) / delete |
| **UBO** | create / update / bind |
| **SSBO** | create / update |
| **RenderTarget** (FBO) | create / bind / unbind / delete |
| **状态** | blend / depthTest / depthWrite / cullFace / viewport / clear / clearColor |
| **Fence** | insertFence / waitFence / deleteFence (GPU 资源回收) |

#### 3.3.2 RenderDeviceProxy（多线程渲染代理）

**文件**: `src/renderer/RenderDeviceProxy.h`

这是渲染层最核心的设计。`RenderDeviceProxy` 是 `RenderDevice` 的子类，在主线程被调用，但实际 GPU 命令通过 **三缓冲 RingBuffer** 异步提交到渲染线程执行。

```
主线程 (Engine Loop)              渲染线程 (RenderingThread)
        │                                  │
   updateVBO() ──→ CommandBuffer[0]        │
   drawVBO()   ──→ CommandBuffer[0]        │
   endFrame()  ──→ signal ───────────→ executeFrame(buf[0])
        │                                  │  ├─ glBufferData(...)
   updateVBO() ──→ CommandBuffer[1]        │  ├─ glDrawElements(...)
        │             ...                   │  └─ insertFence → 回收资源
                                            │
   (RingBuffer 满时主线程阻塞等待)          │
```

**三缓冲设计 (`kRingSize = 3`)**：
- 3 个固定大小 16MB 的 `CommandBuffer`
- `Semaphore m_frameReadySem`: 主线程 signal，渲染线程 wait
- `Semaphore m_freeSlotsSem`: 渲染线程 signal，主线程 wait（初始 = kRingSize-1 = 2）
- 主线程最多领先渲染线程 2 帧

**CommandBuffer**（`src/renderer/CommandBuffer.h`）：
- 固定 16MB 预分配，永不 reallocate
- 零分配编码：`[CmdHeader(8B) | payload(对齐到8B)]`
- 区分 trivially-destructible（`push<T>`）和 non-trivial（`pushNT<T>`，placement-new + 析构链）
- 典型命令：`createVBO`, `updateVBO`, `drawVBO`, `useTexture`, 等

**资源回收（RecyclePool）**：
- 4 种对象池：`VBODataRecyclePool`, `UBODataRecyclePool`, `SSBODataRecyclePool`, `PixelDataRecyclePool`
- VBO/UBO/SSBO 数据通过 GPU Fence 延迟回收（确保 GPU 已完成使用）
- 像素缓冲在 `executeFrame()` 结束后立即回收（`glTexImage2D` 是同步的）

#### 3.3.3 FrameState（逐帧状态）

**文件**: `src/renderer/FrameState.h`

每帧传递的上下文数据，贯穿整个渲染管线：

| 字段 | 说明 |
|------|------|
| `frameNumber` | 帧序号 |
| `deltaTime` | 帧间隔（秒） |
| `camera` | 正交相机（2D） |
| `perspectiveCamera` | 透视相机（3D） |
| `inputEventsManager` | 输入事件管理器 |
| `batchManager` | 当前 Window 的批处理管理器（跨帧持有；每帧清空收集列表，批次结构可增量复用） |
| `ssboManager` | SSBO 管理器 |
| `drawCallCount` | Draw Call 计数 |
| `fps` | 实时帧率 |
| `isSSBOSupport` | 平台探测后写入的 SSBO 能力标记；必须在合批提交前同步到 FrameState |
| `scene3DPassContext` | 3D 场景渲染上下文 |
| `callAfterRender` | 帧末延迟回调队列 |

> **设计约束**：`FrameState` 当前同时包含时间、输入、相机、平台能力、渲染服务和调试计数，
> 已逐渐成为“帧级服务定位器”。后续建议拆分为只读 `FrameContext`、渲染提交用
> `RenderContext` 和统计用 `FrameMetrics`，并明确各字段在主线程/渲染线程上的所有权。

---

### 3.4 UI System — ECS + 场景图

#### 3.4.1 Widget（场景图节点）

**文件**: `src/ui/base/Widget.h`

Widget 是 UI 系统的核心基类，同时承担两个角色：

1. **场景图节点**：树形结构（`m_parent` / `m_children`），支持 `addChild` / `removeChild` / `removeFromStage`
2. **ECS Entity**：通过 `ComponentManager` 管理组件

关键属性：
- `m_visible` / `m_displayLayer`：可见性与渲染层级（-10 ~ 10）
- `m_widgetName` / `m_widgetType` / `m_uniqueID`：标识

#### 3.4.2 UIWidget — 可渲染的 Widget

**文件**: `src/ui/base/UIWidget.h`

继承自 Widget，增加渲染能力：
- `Transform`（位置、尺寸、缩放、旋转、锚点、pivot）
- `MeshFilter`（网格数据）
- `MeshRenderer`（材质 + 渲染）
- `Material`（Shader + Uniform + 纹理）

**这也是 Window 的基类** —— Window 本身是一个 UIWidget 作为场景根节点。

#### 3.4.3 Component / ComponentManager（ECS 组件系统）

**组件生命周期**:
```
constructor → onAttach() → awake() → start() → [update() → lateUpdate()] × N → onDetach() → onDestroy()
                              ↑ setEnabled(true)→onEnable() / setEnabled(false)→onDisable() 切换
```

| 方法 | 调用时机 |
|------|----------|
| `onAttach()` | `Widget::addComponent()` 中、`setGameObject()` 之后 |
| `awake()` | 紧接着 `onAttach()` 之后（首次激活） |
| `start()` | 首帧 `update()` 之前（预留，当前同 awake 时机） |
| `update(fs)` | 每帧，仅 enabled 组件 |
| `lateUpdate(fs)` | `update` 全部完成后（树遍历第二遍），仅 enabled 组件 |
| `onEnable()` | `setEnabled(true)` 时 |
| `onDisable()` | `setEnabled(false)` 时 |
| `onDetach()` | `ComponentManager::removeComponent()` 中、`onDestroy()` 之前 |
| `onDestroy()` | 组件销毁时 |

**ComponentManager** 以 `type_index` 为 key 存储组件，提供模板方法：
- `addComponent<T>(args...)` — 动态添加组件
- `getComponent<T>()` — 获取第一个该类型组件
- `getComponents<T>()` — 获取所有该类型组件
- `removeComponent<T>()` — 移除并销毁组件

**核心组件**：

| 组件 | 职责 |
|------|------|
| `Transform` | 位置/尺寸/缩放/旋转/锚点/pivot，局部→世界矩阵 |
| `Transform3D` | 3D 变换 |
| `MeshFilter` | 持有网格数据（顶点、索引、UV） |
| `MeshRenderer` | 持有 Material，在 update 中将渲染数据提交到 BatchManager |
| `MeshRenderer3D` | 3D 网格渲染 |
| `Interaction` | 触摸/点击交互处理 |
| `Shadow` | UI 阴影效果 |

---

### 3.5 BatchManager — 合批系统

**文件**: `src/core/BatchManager.h`

`BatchManager` 由 `Window` 跨帧持有，并通过 `FrameState` 暴露给组件。每帧开始时调用
`clear()` 轮换当前/上一帧的可渲染列表，但保留可复用的 `RenderBatch`；仅当渲染列表发生
变化时重建批次。它负责在不破坏渲染顺序的前提下尽量减少 Draw Call：

```
Widget::update() → MeshRenderer::update()
    → BatchManager::addRenderable(material, meshFilter, transform)
    → 收集阶段：记录 RenderableItem (含 displayLayer + insertionIndex)

BatchManager::renderBatches(frameState)
    → 增量检查：对比上一帧可渲染列表，未变化则复用批次结构
    → 排序阶段：当前实现按 (displayLayer, materialKey, insertionIndex) 排序
    → 合批阶段：materialKey 仅用于聚类，最终使用 Material::isEqual 校验兼容性
        → SSBO 路径（实例化渲染，单次 Draw Call）
        → 标准路径（批次内逐对象绘制，不能降低 Draw Call）
```

**SSBO 路径**：利用 Shader Storage Buffer Object 实现实例化批量渲染，将所有同材质的 transform 数据写入 SSBO，一次 Draw Call 绘制所有实例。这是引擎高性能的关键设计。

#### 合批正确性约束

合批首先是一个**渲染顺序问题**，其次才是材质分组问题。当前实现需要遵守以下约束：

1. **透明 UI 的 painter's order 不可任意重排**
   大多数 UI 使用 Alpha Blend。即使两个控件 `displayLayer` 相同，只要它们的覆盖区域相交，
   将 `A → B → A` 重排为 `A → A → B` 就可能改变最终像素。`displayLayer` 只能表达粗粒度层级，
   不能证明同层控件可交换。

2. **MaterialKey 不是 BatchKey**
   当前 `materialKey` 只包含 shader 名和常见主纹理指针，适合排序，不足以证明材质完全兼容。
   合批边界必须继续检查 Shader Variant、全部纹理、Blend/Depth/Cull/Scissor、RenderTarget 等状态。

3. **增量复用需要版本号，而不只是对象地址**
   Material、Mesh、裁剪状态可以在指针不变时原地修改。仅比较
   `material/meshFilter/transform` 指针无法发现这类变化。推荐为 `Material`、`MeshFilter`、
   `Widget render state` 增加单调递增的 revision，并把 revision 纳入批次签名。

4. **能力必须由 Platform 显式传递**
   `Platform::ensureRenderCapabilitiesInitialized()` 只更新平台成员；每帧还必须在
   `Platform::beginRenderPass()` 中把 `m_isSSBOSupport` 写入 `FrameState`。否则会退化到标准路径，
   出现“批次已形成但仍逐对象 draw”的现象。

#### 推荐的安全合批模型

建议将“全层材质排序”演进为 **RenderItem → 顺序段（segment）→ Batch**：

```text
Widget traversal
    → RenderItem{orderKey, batchKey, bounds, clipId, barrierFlags, revisions}
    → 按原始 painter's order 生成顺序段
        · RenderTarget / Scissor / Stencil / Mask / 3D Pass 变化时强制断段
        · 默认只合并连续且 BatchKey 相同的项目
        · 仅当可证明不相交或声明 opaque/reorderable 时，才允许段内重排
    → 生成 RenderBatch
```

`BatchKey` 至少应包含：

```text
pipeline/shaderVariant
textures + samplers
blend/depth/cull/colorMask
renderTarget
clip/scissor/stencil state
vertex layout / primitive topology
SSBO layout
```

这样能把“是否可重排”和“是否可合批”分成两个独立判断，避免为了减少 Draw Call 破坏 UI 视觉正确性。

---

### 3.6 Camera — 相机系统

**文件**: `src/core/Camera.h`, `src/core/OrthographicCamera.h`, `src/core/PerspectiveCamera.h`

相机继承体系：
```
Camera (抽象)
├── OrthographicCamera  — 2D UI 渲染（默认相机）
└── PerspectiveCamera   — 3D 场景渲染
    └── OrbitCamera     — 轨道相机（3D 模型交互）
```

Camera 基类提供：
- 位置/方向/上方向设置
- 投影矩阵 × 视图矩阵 → `projectionView`
- `screenToWorld()` / `worldToScreen()` 坐标转换

2D 默认相机放置在 z=10000，看向 -z 方向，near=0, far=11000。UI 元素默认在 z=0 平面渲染，利用深度测试实现 2D/3D 混合。

---

### 3.7 其他关键子系统

| 子系统 | 目录 | 说明 |
|--------|------|------|
| **GlobalObject** | `src/core/GlobalObject.h` | 单例服务定位器，管理 FontManager / TextureManager / RenderingThread / SSBOManager |
| **Observable** | `src/core/Observable.h` | 轻量观察者模式，支持 add/remove/notify，使用 snapshot 避免回调中修改 |
| **FPSController** | `src/core/FPSController.h` | 基于 `high_resolution_clock` 的帧率控制 |
| **Tween** | `src/ui/helpers/Tween.h` | 动画补间系统，TweenManager 单例管理所有动画 |
| **FontManager** | `src/fonts/FontManager.h` | 字体加载、DynamicFont、FontTexture 管理 |
| **TextureManager** | `src/core/TextureManager.h` | 纹理加载缓存（支持 basis_universal 压缩纹理） |
| **GLTF** | `src/gltf/` | GLTF/GLB 加载、场景构建、骨骼动画 |
| **DebugPlane** | `src/debug/DebugPlane.h` | Debug UI 面板（Widget 树可视化、性能统计） |
| **Math** | `src/math/` | Vector2/3/4, Matrix3/4, Quaternion, Rect |
| **Layout** | `src/ui/layout/` | HBox/VBox/Center/Grid/Margin 布局容器 |

---

## 四、数据流总结

```
输入事件                    Engine::render() 主循环
   │                              │
   ▼                              ▼
InputProvider              updateFrameState()
   │                              │
   ▼                              ▼
InputEventsManager     platform->beginFrame()
   │                              │
   ▼                              ▼
平台层 resolveInput     platform->dispatchEvents()
Targets                      │
   │                         ▼
   ▼                   m_preRender.notify()
Platform::dispatch            │
Events (基类)                 ▼
   │                    TweenManager::update()
   ▼                         │
Widget::dispatchTouch         ▼
   │                    platform->beginRenderPass()  ← GPU 准备
   ▼                         │
Interaction::onTouch          ▼
   │                    platform->updateWidgets()     ← Widget 树遍历
   ▼                         │
(状态变更/动画)               ▼
                       ComponentManager::updateComponents()
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
         Transform    MeshRenderer   Interaction
         (矩阵更新)   (提交渲染数据)  (交互逻辑)
                          │
                          ▼
                   BatchManager::addRenderable()
                          │
                          ▼
                   platform->commitRenderPass()      ← GPU 提交
                          │
                          ▼
                   BatchManager::renderBatches()
                          │
                          ▼
                   RenderDeviceProxy (命令编码)
                          │
                          ▼
                   CommandBuffer → 渲染线程 → GLRenderDevice → OpenGL
                          │
                          ▼
                   platform->endFrame() → SwapBuffers
                          │
                          ▼
                   m_afterRender.notify()
                   callAfterRenderFunctions()
```

---

## 五、多线程模型

```
┌──────────────────────┐         ┌──────────────────────────┐
│     主线程            │         │      渲染线程             │
│  (Engine Loop)       │         │  (RenderingThread)       │
├──────────────────────┤         ├──────────────────────────┤
│ • Widget 树遍历       │         │ • GL 命令执行              │
│ • Transform 矩阵更新  │  Ring   │ • glBufferData            │
│ • 合批判断/提交       │ Buffer  │ • glDrawElements          │
│ • 事件分发            │ ──────→ │ • glTexImage2D            │
│ • Tween 更新          │         │ • SwapBuffers             │
│ • 业务逻辑            │         │ • Fence 资源回收          │
└──────────────────────┘         └──────────────────────────┘
```

- **单线程模式** (`multithread=false`): RenderDeviceProxy 在 `endFrame()` 中同步执行 CommandBuffer
- **多线程模式** (`multithread=true`): 渲染线程独立运行，主线程通过三缓冲 RingBuffer 异步提交命令
- 同步原语：`Semaphore`（平台相关实现）

---

## 六、关键设计模式

| 模式 | 应用场景 |
|------|----------|
| **ECS** | Widget = Entity, Component = 行为片段，ComponentManager = 组件存储 |
| **组合模式** | Widget 树形结构，父→子递归更新和渲染 |
| **观察者模式** | `Observable<>` — preRender/afterRender 钩子、输入事件分发 |
| **代理模式** | `RenderDeviceProxy` — 多线程安全的 GPU 命令代理 |
| **工厂模式** | `PlatformFactory` — 编译期平台选择 |
| **单例模式** | `GlobalObject` — 全局服务定位器；`TweenManager` — 全局动画管理器 |
| **对象池模式** | `RecyclePool` — GPU 数据零分配回收 |
| **策略模式** | `InputProvider` — 不同平台的输入策略 |
| **模板方法** | `Platform::initialize()` — 基类定义骨架，子类实现细节 |

---

## 七、提升建议

> **章节定位**：本章是问题与技术债清单，不作为独立实施 Roadmap。新增或未完成事项必须先归入
> 第九章 P0～P4 的某个阶段，再进入开发；避免第七章和第九章形成两套任务列表。
>
> 状态约定：
> - ✅ 已完成
> - 🚧 已纳入第九章，正在或等待分阶段实施
> - 📌 独立技术债，尚未排期

### 7.1 架构层面

**1. GlobalObject 服务定位器 → 依赖注入** 🚧 P2～P3

当前 `GlobalObject` 是一个大而全的单例服务定位器，`Engine.cpp` 大量依赖它获取 FontManager、TextureManager、SSBOManager、RenderingThread。这导致：
- 隐式依赖，单元测试困难
- 全局状态，生命周期不可控
- 模块间耦合度高

建议：逐步将 `GlobalObject` 拆解，通过构造函数或 `Engine` 注入所需的共享服务。

**2. Platform::update() 职责过重** ✅ 已完成

~~当前 `Platform::update()` 同时承担事件处理、Widget 树更新和渲染提交~~ → 已拆分为 4 个独立阶段，由 Engine 显式编排：

- `dispatchEvents(frameState)` — 事件分发（渲染之前，消除 1 帧延迟）
- `beginRenderPass(frameState)` — GPU 准备（viewport、clear、framebuffer）
- `updateWidgets(frameState)` — Widget 树遍历（纯 CPU，可独立单元测试）
- `commitRenderPass(frameState)` — GPU 提交（合批渲染）

Engine::render() 现为 7 步显式流水线：`beginFrame → dispatchEvents → preRender/Tween → beginRenderPass → updateWidgets → commitRenderPass → endFrame`

**3. Window 继承 UIWidget 的合理性** 📌 独立技术债，建议在 P3 评估

`Window` 继承自 `UIWidget`（从而间接继承 `Widget`），这带来便利但也让 Window 承担了双重角色。Window 本身不需要 Transform/MeshRenderer 等渲染组件。考虑改为 Widget 持有 Window 引用（组合优于继承），或让 Window 成为纯粹的渲染目标抽象。

### 7.2 渲染层面

**4. RenderDevice 接口膨胀** ✅ 已完成

~~`RenderDevice` 是一个包含 ~60+ 个纯虚方法的巨型接口（VBO、Texture、Shader、UBO、SSBO、FBO、状态管理等）。~~ 已按资源类型拆分为四个子接口，`RenderDevice` 通过多重继承组合：

- `GPUBufferDevice` (VBO/UBO/SSBO/Fence)
- `GPUTextureDevice`
- `GPUShaderDevice`
- `GPURenderPassDevice` (FBO/State/Context)

新代码可按需依赖子接口以降低耦合；旧代码使用 `RenderDevice*` 不受影响。

**5. BatchManager 动态合批** 🚧 基础能力已完成，正确性与失效机制仍需完善

当前已具备：

1. **收集与构建分离**：Widget 遍历阶段只生成 `RenderableItem`，提交阶段统一构建批次。
2. **跨帧批次复用**：渲染列表稳定时复用上一帧 `RenderBatch`，避免重复排序和分组。
3. **SSBO 合批路径**：平台支持 SSBO 且批次包含多个实例时，可一次 Draw Call 绘制。
4. **材质兼容性二次校验**：`materialKey` 只负责排序，最终通过 `Material::isEqual()` 决定是否合并。

仍需处理：

- **同层透明元素不可无条件按材质重排**。当前 `(displayLayer, materialKey)` 排序只适合明确允许
  reorder 的内容；通用 UI 应默认保持 painter's order。
- **增量检测缺少 revision**。材质参数、纹理、Mesh、Scissor 原地改变时，对象指针可能不变，
  需要 `materialRevision / geometryRevision / renderStateRevision`。
- **BatchKey 不完整**。应纳入 Pipeline、全部纹理/采样器、Blend/Depth/Cull、RenderTarget、
  Clip/Stencil、VertexLayout 等状态。
- **标准路径只是分组，不是真正合批**。不支持 SSBO 时仍逐对象 Draw，需要动态顶点/索引合并、
  Multi-Draw 或实例化属性作为 fallback。
- **动态图集**：为大量小图标、字体和静态图片提供稳定的纹理页，减少因纹理切换产生的批次。

**6. 缺少 RenderGraph / Pass 抽象** 🚧 P3

当前渲染是顺序执行的（2D UI → 3D Scene → DebugPlane），缺少显式的渲染 Pass 图。引入 RenderGraph 可以：
- 自动管理 RenderTarget/Attachment 生命周期
- 自动进行 Barrier/依赖分析
- 便于插入后处理 Pass（bloom、blur、color grading）

建议不要一开始实现通用 DAG 编译器，而是分两步推进：

1. 先引入轻量 `RenderPass` 描述：`name / target / viewport / clear / execute / dependencies`，
   把 UI、3D、Debug、Offscreen 从 Window/Widget 中解耦。
2. 当出现多个离屏目标和后处理链后，再增加资源别名、生命周期分析和自动排序。

### 7.3 UI 系统层面

**7. Component 生命周期不完整** ✅ 已完成

~~当前生命周期：`awake()` → `start()` → `update()` → `onDestroy()`~~ → 已完善：

- **`onAttach()`** — 组件被 `Widget::addComponent()` 添加后、`setGameObject()` 之后调用
- **`onDetach()`** — 组件被 `ComponentManager::removeComponent()` 移除前调用（在 `onDestroy` 之前）
- **`onEnable()` / `onDisable()`** — `Component::setEnabled(true/false)` 中正确触发（之前只设置了 `m_enabled` 字段但未调用回调）
- **`lateUpdate(frameState)`** — 新生命周期阶段，在所有 standard update 完成后调用。Engine 在 `updateWidgets` 和 `commitRenderPass` 之间调用 `lateUpdateWidgets`，递归遍历整个 Widget 树

**完整生命周期**：`constructor → onAttach → awake → start → [update → lateUpdate] × N → onDetach → onDestroy`
`               `(setEnabled(true) → onEnable)   (setEnabled(false) → onDisable)`

**8. Widget 缺少布局脏标记传播** 🚧 P2

Transform 有 `m_matrixDirty` 标记，但尺寸变化时缺少自动向父/子传播的脏标记机制。当前依赖逐帧全量更新，对于大型 UI 树可考虑增量更新。

**9. 缺少样式系统** 📌 建议在 P2 基础完成后独立规划

当前属性（颜色、字体、边距等）通过代码直接设置，没有类似 CSS/样式表的抽象层。对于车载 HMI 等需要换肤/主题切换的场景，建议增加：
- 样式属性定义（color, font, padding, background 等）
- 样式继承/层叠规则
- 主题/皮肤切换机制
- 支持从 JSON/二进制加载样式

### 7.4 平台与工具层面

**10. PlatformFactory 编译期耦合** 🚧 P3～P4

当前通过 `#ifdef OPENGL_EGL / OPENGL_GLFW` 在编译期选择平台，导致：
- 无法在同一构建中支持多后端
- 新增平台需要修改工厂代码

建议改为运行时注册机制或插件模式，或至少通过链接时选择（同一套接口的不同 .cpp 实现文件）。

**11. 缺少性能分析基础设施** 🚧 P0 最小版，P4 完整版

当前仅有 DebugPlane 的基础统计（FPS、DrawCall 数）。建议增加：
- GPU 时间戳查询（`GL_TIMESTAMP` / `glQueryCounter`）
- Per-Pass 耗时统计
- 帧时间火焰图（主线程 + 渲染线程）
- 内存使用追踪（纹理、VBO、CommandBuffer 占用）

**12. 缺少单元测试和 CI** 🚧 P0 最小版，P4 完整版

代码库中未发现测试框架。建议：
- 引入 Google Test 或 Catch2
- 对 Math 库、Observable、CommandBuffer、RecyclePool 等纯逻辑模块编写单元测试
- 对 RenderDeviceProxy 编写多线程压力测试
- 配置 CI（GitHub Actions / 自建 Runner）进行编译验证 + 测试

### 7.5 代码质量

**13. 头文件包含路径不统一** ✅ 已完成

~~混用了相对路径（`"../ui/base/Widget.h"`）和绝对路径（`"FrameState.h"`）~~ → 已统一为基于 `src/` 根目录的 include path。CMake 已配置 `target_include_directories` 包含 `src/`，所有 `../` 和 `../../` 相对路径已替换为 `src/`-相对路径（如 `"ui/base/Widget.h"`、`"renderer/RenderDeviceProxy.h"`）。`extern/basis_universal/` 第三方代码维持不变。

**14. 命名不一致** ✅ 已完成

~~`OrthographicCameraSharePtr` vs `PlatformSharedPtr`（Share vs Shared）~~ → `OrthographicCameraSharePtr` 已重命名为 `OrthographicCameraSharedPtr`。

~~`WGLPlatform` vs `QNXEGLPlatform`（不一致的命名风格）~~ → `QNXEGLPlatform` 已重命名为 `QNXPlatform`，与 `WGLPlatform` 统一使用 `<平台><后缀>` 风格。

**15. 错误处理机制** 🚧 P3～P4

当前缺少统一的错误处理策略。OpenGL 调用失败时（如 shader 编译错误、纹理加载失败）通常只通过 LOG_E 输出日志。建议：
- 定义 `Result<T, Error>` 类型
- 关键路径（资源加载、shader 编译）返回可检查的错误
- 提供降级渲染策略（如缺失纹理用纯色替代）

---

## 八、推荐目标架构

### 8.1 模块边界

建议逐步收敛为以下单向依赖：

```text
Application / Samples
        ↓
UI Runtime ─────→ Asset API
        ↓              ↓
Render World → Render Pipeline
                       ↓
                  RHI / RenderDevice
                       ↓
                    Platform
```

| 模块 | 只负责 | 不应负责 |
|------|--------|----------|
| **UI Runtime** | Widget、布局、输入、动画、样式、生成 RenderItem | 直接调用 GPU、管理 GL 对象 |
| **Render World** | 不可变的帧快照、排序键、裁剪信息、资源句柄 | 修改 Widget 树 |
| **Render Pipeline** | Pass 编排、批次构建、状态排序、提交 | 业务逻辑和输入分发 |
| **RHI** | Buffer/Texture/Pipeline/Command/Sync 抽象 | UI 语义、字体语义 |
| **Asset** | 加载、缓存、热更新、预算、异步上传 | 控制 Widget 生命周期 |
| **Platform** | Window、Surface、Input、Context/Present | 合批策略 |

核心原则是：**主线程提交不可变 RenderSnapshot，渲染线程只消费快照和资源句柄**。不要把
`shared_ptr<Material/MeshFilter/Transform>` 直接跨线程当作长期渲染数据源，否则主线程修改对象时
很难定义同步边界。

### 8.2 建议的帧管线

```text
1. Input
   poll → hit test → dispatch

2. Simulation
   preRender → Tween → Component::update → lateUpdate

3. UI Resolve
   style dirty → layout dirty → transform dirty → paint dirty

4. Render Extraction
   Widget tree → immutable RenderSnapshot

5. Render Preparation
   culling → clip resolve → segment → batch → pass list

6. Submission
   encode CommandBuffer → render thread → GPU

7. Present & Metrics
   present → fence/recycle → CPU/GPU metrics
```

`RenderSnapshot` 推荐使用 frame arena/线性分配器，以 handle/index 引用资源，帧完成后整体回收。
这样既能降低 `shared_ptr` 原子引用计数开销，也能把“本帧渲染看到什么”固定下来。

### 8.3 Dirty Flag 体系

大型 UI 树不应永久依赖全量 `update()`。建议定义并向上/向下传播以下脏标记：

| Dirty 类型 | 典型触发 | 传播方向 | 处理阶段 |
|------------|----------|----------|----------|
| `StyleDirty` | class/theme/state 变化 | 向子节点继承 | Style Resolve |
| `LayoutDirty` | size/margin/text 变化 | 向父节点冒泡 | Layout |
| `TransformDirty` | position/scale/parent matrix 变化 | 向子节点下传 | Transform |
| `PaintDirty` | color/texture/UV/material 变化 | 当前节点 | Render Extraction |
| `OrderDirty` | child/displayLayer 变化 | 当前容器 | Render Preparation |
| `ClipDirty` | mask/scissor 变化 | 向子节点下传 | Clip Resolve |

短期内仍可保留组件逐帧 `update()`，但布局、矩阵和批次重建应由 dirty/revision 驱动。

### 8.4 资源与线程所有权

建议明确三类对象：

1. **CPU Asset**：图片、字体、Mesh 源数据，由 AssetManager 管理，可跨线程加载。
2. **GPU Resource Handle**：只暴露稳定 ID/代数，不暴露后端指针；创建和销毁由渲染线程执行。
3. **Frame Upload Data**：属于某个 frame slot，Fence 完成后统一回收。

资源句柄推荐使用 `{index, generation}`，避免异步销毁后旧命令误用复用槽位。资源删除流程：

```text
main thread release
    → enqueue destroy(handle, lastUsedFrame)
    → render thread waits corresponding fence
    → destroy backend object
    → generation++
```

### 8.5 可观测性预算

建议为每帧记录：

- CPU：Input、Update、Layout、Extraction、BatchBuild、CommandEncode、RenderThread Execute
- GPU：每个 Pass 的 timestamp
- 数量：Widget、可见 Widget、RenderItem、Segment、Batch、Draw、Triangle、纹理切换
- 内存：纹理/VBO/SSBO/FrameArena/CommandBuffer 当前值和峰值
- 合批原因：`shader mismatch / texture mismatch / clip barrier / order barrier / capacity`

仅统计 Draw Call 无法判断性能瓶颈。尤其应同时显示：

```text
RenderItems → Batches → DrawCalls
batch cache hit rate
command buffer used / capacity
main thread lead frames
```

---

## 九、分阶段演进路线

### 9.1 章节使用原则

后续开发以本章作为**唯一实施路线**：

```text
第九章：确定当前阶段、任务和验收标准
    ↓
第七章：确认任务要解决的现状问题与技术债
    ↓
第八章：确认模块边界、数据所有权和线程模型
    ↓
设计评审 → 实现 → 测试 → 验收
```

- 第七章负责回答“为什么要改”，但不能直接从中随意挑选任务开发。
- 第八章负责回答“最终要改成什么样”，用于设计和 Code Review，不是一次性重构清单。
- 第九章负责回答“现在先做什么”，所有新任务都应归入某个阶段并定义验收标准。
- P0 先建立最小测试和可观测性基础；不能等到 P4 才开始测试。P4 的目标是把最小测试扩展为
  完整 CI、压力测试、渲染回归和性能趋势系统。

### 9.2 第七章事项与实施阶段映射

| 第七章事项 | 实施阶段 | 说明 |
|------------|----------|------|
| GlobalObject → 依赖注入 | P2～P3 | 在 RenderSnapshot 和资源边界明确后逐步拆分 |
| Window 改为组合关系 | P3 或独立重构 | 与 RenderPass/RenderTarget 抽象一起评估 |
| BatchManager 动态合批完善 | P0～P1 | 先建立正确性基线，再完善 BatchKey/revision |
| RenderGraph / Pass | P3 | 先做轻量 RenderPass，不直接实现通用 DAG |
| Layout/Transform Dirty | P2 | 与增量 UI、RenderSnapshot 同步实施 |
| 样式系统 | P2 后独立规划 | 不阻塞当前渲染正确性和线程边界治理 |
| PlatformFactory 解耦 | P3～P4 | 与多后端构建和 CI 配套推进 |
| 性能分析 | P0 最小版，P4 完整版 | P0 提供合批统计，P4 增加 CPU/GPU 时间线 |
| 单元测试和 CI | P0 最小版，P4 完整版 | P0 覆盖 BatchBuilder，P4 覆盖全工程 |
| 错误处理 | P3～P4 | 资源系统稳定后引入统一 Result/Error |

### P0 — 正确性基线

P0 的目标不是继续扩大优化范围，而是让当前合批系统**可观察、可测试、可回归**。

#### P0.1 — BatchManager 可观测性 ✅ 已完成

已新增 `src/renderer/BatchStatistics.h`，并由 `FrameState::batchStatistics` 持有帧级统计：

```cpp
struct BatchStatistics {
    uint32_t renderItemCount = 0;
    uint32_t batchCount = 0;
    uint32_t ssboBatchCount = 0;
    uint32_t standardBatchCount = 0;
    uint32_t batchDrawCallCount = 0;
    uint32_t cacheHitCount = 0;
    uint32_t cacheMissCount = 0;
    uint32_t ssboFallbackBatchCount = 0;
    std::array<uint32_t, ...> breakReasonCounts;
};
```

当前已记录的断批原因：

```cpp
enum class BatchBreakReason {
    None,
    DisplayLayer,
    Shader,
    Texture,
    MaterialState,
    OrderBarrier,
};
```

其中 `MaterialState` 当前覆盖 `Material::isEqual()` 能识别的 Blend、双面和纹理集合差异。
Clip、RenderTarget、Geometry 等原因将在 P0.2/P1 引入完整 RenderItem/BatchKey 后细分。

`DebugPlane` 已扩展为展示：

```text
Items / Batches / Draws / BatchDraws
SSBO / Standard / SSBO Fallback Batches
Cache Hit/Miss
Break[L/S/T/M/O]
```

`Break[L/S/T/M/O]` 分别表示：

```text
L = DisplayLayer
S = Shader
T = Texture
M = MaterialState
O = OrderBarrier
```

统计刷新流程：

```text
Engine::updateFrameState()
    → reset BatchStatistics
BatchManager::renderBatches()
    → 记录 item、cache、batch、路径、fallback、断批原因和 batch draw
DebugPlane::update()
    → 展示本帧统计
```

**已完成验收**：

- `renderItemCount → batchCount → drawCallCount` 可以完整追踪。
- 每个断批点可以输出明确原因，而不是只能看到最终 Draw Call 数。
- 能区分“未形成批次”和“已形成批次但因 SSBO 不可用而退化”。
- MinGW 配置下 `ImageDemo` 编译通过。

#### P0.2 — 抽离可测试的 BatchBuilder ✅ 已完成

已新增：

```text
src/core/BatchBuilder.h
src/core/BatchBuilder.cpp
```

并将不依赖 GPU Context 的分组逻辑从 `BatchManager` 抽离：

```cpp
class BatchBuilder {
public:
    BatchBuildResult build(
        const std::vector<RenderItem>& items,
        const BatchBuildOptions& options);
};
```

当前职责边界：

- `BatchManager`：每帧收集、缓存管理、调用 BatchBuilder、执行 GPU 提交。
- `BatchBuilder`：保持 painter's order、计算断批点、生成只包含 item index 的 BatchGroup。
- `BatchExecutor`（可后续抽离）：SSBO/标准路径的实际资源更新与 Draw。

`BatchBuildResult` 当前包含：

```cpp
struct BatchGroup {
    std::vector<uint32_t> itemIndices;
};

struct BatchBuildResult {
    std::vector<BatchGroup> groups;
    std::vector<BatchBreakReason> breakReasons;
};
```

构建结果只引用输入列表的稳定 index，不持有临时栈内存，也不创建 `VertexArray`、SSBO 或其他 GPU
对象。`BatchManager` 根据 group 结果从 `RenderBatchPool` 物化实际 RenderBatch。

P0.2 同时修正了原有的透明 UI 顺序风险：

```text
旧行为：同 displayLayer 内按 materialKey 全局重排
新行为：严格保持 Widget 遍历/painter's order，只合并连续兼容项
```

因此 `A → B → A` 默认保持三个顺序批次，不会为了减少 Draw Call 被重排为 `A+A → B`；
连续的 10 个同材质 Image 仍可形成一个批次。

`BatchBuildOptions::preservePainterOrder` 已预留，后续只有在 P1 能明确证明 opaque/non-overlap
或显式声明 reorderable 时，才扩展安全重排策略。

**已完成验收**：

- BatchBuilder 不创建 OpenGL Context、不调用 `RENDERINGTHREAD`。
- BatchBuilder 不依赖 Window、FrameState、RenderBatchPool、VertexArray、SSBO。
- 可以通过普通单元测试输入 RenderItem 列表并断言 group 和 break reason。
- 默认保持 painter's order，不在没有显式证明时跨元素重排。
- `ImageDemo` 在 MinGW 配置下编译通过。

#### P0.3 — 最小自动测试

P0 即引入最小测试框架，不等待 P4。至少覆盖：

| 场景 | 输入 | 预期 |
|------|------|------|
| 同材质同纹理 | 10 个 Image | 1 个可 SSBO 合批的 Batch |
| 不同纹理 | texture A、texture B | 2 个 Batch，原因 `Texture` |
| 材质交错 | A → B → A | 默认保持 3 个顺序 Batch，不重排为 A+A → B |
| 不同 Blend | shader/texture 相同，Blend 不同 | 必须断批 |
| 不同 Scissor/Clip | 材质相同，裁剪不同 | 必须断批 |
| displayLayer 变化 | Widget 运行时换层 | 下一帧缓存失效并重建 |
| 材质原地修改 | 指针不变、revision 变化 | 缓存失效（P1 revision 完成后启用） |
| SSBO 关闭 | `isSSBOSupport=false` | 功能正确并记录标准路径退化 |

单元测试只验证 BatchBuilder 的纯逻辑；透明重叠、裁剪和最终像素正确性由集成渲染测试覆盖。

**验收标准**：

- 测试可以在无窗口、无 GPU Context 环境运行。
- 每次修改合批算法时都能自动发现顺序或兼容性回归。

#### P0.4 — ImageDemo 集成验收

将 `ImageDemo` 固化为集成基准之一：

```text
场景：
10 个相同纹理 MRImage
2 个 DebugPlane MRLabel

支持 SSBO：
Image Batch = 1 Draw
Label Batch = 1 Draw
Total = 2 Draws
```

建议增加可选的固定帧运行模式，而不是只能进入无限渲染循环，例如运行 3～5 帧后输出 JSON/日志：

```json
{
  "renderItems": 12,
  "batches": 2,
  "ssboBatches": 2,
  "drawCalls": 2
}
```

同时增加以下集成场景：

- 两张半透明图片重叠，验证重排前后像素一致。
- 图片使用不同纹理，验证正确断批。
- 动态修改 displayLayer/Texture/Blend，验证下一帧缓存失效。
- 强制关闭 SSBO，验证标准路径画面正确且统计明确。

**P0 总体验收标准**：

- 优化前后像素结果一致。
- 支持 SSBO 时 `ImageDemo` 稳定为预期 Draw Call。
- 关闭 SSBO 时功能正确，并明确报告退化原因。
- 批次缓存不会引用已回收对象。
- BatchBuilder 具备无 GPU 单元测试。
- 统计信息足以定位断批和缓存失效原因。

### P1 — 安全合批与版本化

- 引入完整 `BatchKey/PipelineStateKey`。
- 为 Material、Mesh、Clip、RenderState 增加 revision。
- 默认只合并连续项；为 opaque/non-overlap 内容提供显式 reorder 策略。
- 非 SSBO 平台增加动态几何合并 fallback。

**验收标准**：材质原地修改能够立即使缓存失效；透明重叠测试无视觉回归。

### P2 — 增量 UI 与 RenderSnapshot

- 增加 Style/Layout/Transform/Paint/Order/Clip Dirty Flag。
- 将 Widget 遍历结果提取为不可变 `RenderSnapshot`。
- 使用 frame arena 替代帧内大量小对象与 `shared_ptr` 拷贝。
- 渲染线程不再解引用可变 Widget/Component。

**验收标准**：静态 UI 的 Layout、Transform、BatchBuild 开销接近零；线程竞态边界可审计。

### P3 — Pass 与资源系统

- 引入轻量 RenderPass/RenderTarget 描述。
- 建立异步 Asset Pipeline、GPU 上传队列、资源预算和 LRU。
- 统一 GPU Handle、延迟销毁和设备丢失恢复策略。

**验收标准**：UI/3D/Debug/Offscreen Pass 可独立启停；资源释放由 Fence 保证安全。

### P4 — 工具链与质量门禁

- 单元测试：Math、CommandBuffer、BatchKey、Dirty Propagation。
- 渲染回归：离屏渲染 + golden image/perceptual diff。
- 压力测试：RingBuffer 满载、资源反复创建销毁、窗口 resize、请求渲染模式。
- CI 同时覆盖 MinGW/Windows 与 QNX 交叉编译配置。

**验收标准**：每个合并请求自动执行编译、单测和关键渲染基准，并保存性能趋势。

---

## 十、架构不变量（开发检查清单）

1. GPU API 只在拥有图形上下文的线程执行。
2. 已提交的命令不得引用可能在执行前失效的栈内存或可变对象。
3. 任何排序优化都必须先证明不会改变透明混合、裁剪、Stencil 和 RenderTarget 语义。
4. BatchKey 相同是合批的必要条件；顺序可交换是重排的必要条件，两者不可混为一谈。
5. 所有跨帧缓存必须有明确的 revision、失效条件和资源生命周期。
6. `FrameState` 中的平台能力必须在使用前由 Platform 显式初始化。
7. Widget 树只由主线程修改；渲染线程只消费不可变快照或命令。
8. 每个固定容量缓冲区必须有峰值统计和可诊断的溢出策略，不能只依赖 Debug `assert`。
9. 资源销毁必须晚于最后一次 GPU 使用，并通过 Fence/frame serial 证明。
10. 性能优化必须同时提供正确性测试、性能指标和可回退路径。
