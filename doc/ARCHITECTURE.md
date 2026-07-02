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
│  │  └── QNXEGLPlatform (QNX/EGL)                        │   │
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

**渲染主循环流程**（`Engine::render()`）：

```
while (!platform->shouldClose()):
  1. updateFrameState()        ← 更新 deltaTime，重置逐帧计数器
  2. platform->beginFrame()    ← 平台层准备帧（拉取输入，设置 GL 上下文）
  3. m_preRender.notify()      ← 通知 preRender 观察者
  4. TweenManager::update()    ← 更新所有动画/补间
  5. platform->update()        ← 核心更新：遍历 Widget 树 → 收集渲染数据 → 合批 → 绘制
  6. heartbeat()               ← 每 2s 计算一次实际 FPS
  7. debugPlane->update()      ← Debug 面板渲染
  8. m_afterRender.notify()    ← 通知 afterRender 观察者
  9. platform->endFrame()      ← SwapBuffer / Present
 10. callAfterRenderFunctions() ← 执行延迟回调
```

**EngineOptions 配置**:
- `multithread` (默认 true): 是否启用多线程异步渲染
- `enableRequestRender` (默认 false): 是否启用按需渲染
- `samples` (默认 1): MSAA 采样数
- `windowInfo`: 窗口配置（尺寸、位置、zorder 等）

---

### 3.2 Platform — 平台抽象层

**文件**: `src/platform/Platform.h`

Platform 是平台抽象基类，定义了跨平台的生命周期接口：

```
initialize() → [beginFrame() → update() → endFrame()] × N → terminate()
```

| 方法 | 职责 |
|------|------|
| `initialize(bool multithread)` | 创建窗口、GL 上下文、输入提供者、渲染线程 |
| `beginFrame(frameState)` | 拉取输入事件、设置 GL 上下文、清屏 |
| `update(frameState)` | 事件分发 + Widget 树更新 + 渲染提交 |
| `endFrame()` | SwapBuffers / eglSwapBuffers |
| `terminate()` | 销毁 GL 上下文、窗口、渲染线程 |

**平台实现**：

| 平台 | 类 | 目录 |
|------|-----|------|
| Windows (GLFW + WGL) | `WGLPlatform` | `src/platform/wgl/` |
| QNX (EGL) | `QNXEGLPlatform` | `src/platform/egl/` |

**PlatformFactory** 通过编译期宏选择平台：
```cpp
#ifdef OPENGL_EGL    → QNXEGLPlatform
#ifdef OPENGL_GLFW   → WGLPlatform
```

**输入系统**:
- `InputProvider` 接口：平台相关的事件源（鼠标/触摸/键盘）
- `InputEventsManager`：统一管理输入事件，通过 `Observable<std::vector<TouchEvent>&>` 分发
- `Platform` 基类负责 `resolveInputTargets()` —— 在 Widget 树中查找触摸命中目标

**Window 抽象**:
- `Window` 继承自 `UIWidget`，本身是 Widget 树的根节点
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
| `batchManager` | 批处理管理器（每帧重建） |
| `ssboManager` | SSBO 管理器 |
| `drawCallCount` | Draw Call 计数 |
| `fps` | 实时帧率 |
| `scene3DPassContext` | 3D 场景渲染上下文 |
| `callAfterRender` | 帧末延迟回调队列 |

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
awake() → start() → [update() × N] → onDestroy()
              ↑ onEnable() / onDisable() 切换
```

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

`BatchManager` 在每帧开始时创建（通过 `FrameState`），负责将多个 Widget 的渲染请求合并为最少次数的 Draw Call：

```
Widget::update() → MeshRenderer::update()
    → BatchManager::addRenderable(material, meshFilter, transform)
    → 按 Material/Shader/Texture 分组 → RenderBatch
    → BatchManager::renderBatches(frameState)
        → SSBO 路径（实例化渲染，单次 Draw Call）
        → 标准路径（逐 Batch 绘制）
```

**SSBO 路径**：利用 Shader Storage Buffer Object 实现实例化批量渲染，将所有同材质的 transform 数据写入 SSBO，一次 Draw Call 绘制所有实例。这是引擎高性能的关键设计。

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
Platform::eventHandler  m_preRender.notify()
   │                              │
   ▼                              ▼
Widget::dispatchTouch   TweenManager::update()
   │                              │
   ▼                              ▼
Interaction::onTouch    Window::update()  ← Widget 树遍历
   │                              │
   ▼                              ▼
(状态变更/动画)         ComponentManager::updateComponents()
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
| **单例模式** | `GlobalObject` — 全局服务定位器；`TweenManager` |
| **对象池模式** | `RecyclePool` — GPU 数据零分配回收 |
| **策略模式** | `InputProvider` — 不同平台的输入策略 |
| **模板方法** | `Platform::initialize()` — 基类定义骨架，子类实现细节 |

---

## 七、提升建议

### 7.1 架构层面

**1. GlobalObject 服务定位器 → 依赖注入**

当前 `GlobalObject` 是一个大而全的单例服务定位器，`Engine.cpp` 大量依赖它获取 FontManager、TextureManager、SSBOManager、RenderingThread。这导致：
- 隐式依赖，单元测试困难
- 全局状态，生命周期不可控
- 模块间耦合度高

建议：逐步将 `GlobalObject` 拆解，通过构造函数或 `Engine` 注入所需的共享服务。

**2. Platform::update() 职责过重**

当前 `Platform::update()` 同时承担事件处理、Widget 树更新和渲染提交。建议将这三个关注点分离为独立的方法或在 Engine 层面显式编排。

**3. Window 继承 UIWidget 的合理性**

`Window` 继承自 `UIWidget`（从而间接继承 `Widget`），这带来便利但也让 Window 承担了双重角色。Window 本身不需要 Transform/MeshRenderer 等渲染组件。考虑改为 Widget 持有 Window 引用（组合优于继承），或让 Window 成为纯粹的渲染目标抽象。

### 7.2 渲染层面

**4. RenderDevice 接口膨胀**

`RenderDevice` 是一个包含 ~60+ 个纯虚方法的巨型接口（VBO、Texture、Shader、UBO、SSBO、FBO、状态管理等）。建议按资源类型拆分为：
- `GPUBufferDevice` (VBO/UBO/SSBO)
- `GPUTextureDevice`
- `GPUShaderDevice`
- `GPURenderPassDevice` (FBO/State)

通过组合或多重继承降低单个接口的复杂度。

**5. BatchManager 不支持动态合批**

当前合批策略是按 Material/Shader/Texture 严格分组。对于运行时动态变化的 UI（如动画、滚动列表），可考虑：
- 基于纹理图集的动态图集打包
- Z-order 敏感的合批重排
- 增量合批（只重建变化的 batch）

**6. 缺少 RenderGraph / Pass 抽象**

当前渲染是顺序执行的（2D UI → 3D Scene → DebugPlane），缺少显式的渲染 Pass 图。引入 RenderGraph 可以：
- 自动管理 RenderTarget/Attachment 生命周期
- 自动进行 Barrier/依赖分析
- 便于插入后处理 Pass（bloom、blur、color grading）

### 7.3 UI 系统层面

**7. Component 生命周期不完整**

当前生命周期：`awake()` → `start()` → `update()` → `onDestroy()`。缺少：
- `onAttach()` / `onDetach()` — 组件被添加到/移除出 Widget 时
- `onEnable()` / `onDisable()` — 已有但未被 ComponentManager 主动调用（仅 `setEnabled()` 内部使用）
- 缺少统一的 `lateUpdate()` 阶段（用于依赖其他组件已更新的逻辑）

**8. Widget 缺少布局脏标记传播**

Transform 有 `m_matrixDirty` 标记，但尺寸变化时缺少自动向父/子传播的脏标记机制。当前依赖逐帧全量更新，对于大型 UI 树可考虑增量更新。

**9. 缺少样式系统**

当前属性（颜色、字体、边距等）通过代码直接设置，没有类似 CSS/样式表的抽象层。对于车载 HMI 等需要换肤/主题切换的场景，建议增加：
- 样式属性定义（color, font, padding, background 等）
- 样式继承/层叠规则
- 主题/皮肤切换机制
- 支持从 JSON/二进制加载样式

### 7.4 平台与工具层面

**10. PlatformFactory 编译期耦合**

当前通过 `#ifdef OPENGL_EGL / OPENGL_GLFW` 在编译期选择平台，导致：
- 无法在同一构建中支持多后端
- 新增平台需要修改工厂代码

建议改为运行时注册机制或插件模式，或至少通过链接时选择（同一套接口的不同 .cpp 实现文件）。

**11. 缺少性能分析基础设施**

当前仅有 DebugPlane 的基础统计（FPS、DrawCall 数）。建议增加：
- GPU 时间戳查询（`GL_TIMESTAMP` / `glQueryCounter`）
- Per-Pass 耗时统计
- 帧时间火焰图（主线程 + 渲染线程）
- 内存使用追踪（纹理、VBO、CommandBuffer 占用）

**12. 缺少单元测试和 CI**

代码库中未发现测试框架。建议：
- 引入 Google Test 或 Catch2
- 对 Math 库、Observable、CommandBuffer、RecyclePool 等纯逻辑模块编写单元测试
- 对 RenderDeviceProxy 编写多线程压力测试
- 配置 CI（GitHub Actions / 自建 Runner）进行编译验证 + 测试

### 7.5 代码质量

**13. 头文件包含路径不统一**

混用了相对路径（`"../ui/base/Widget.h"`）和绝对路径（`"FrameState.h"`），建议统一为基于 src 根目录的 include path（如 `"ui/base/Widget.h"`），通过 CMake 的 `target_include_directories` 配置。

**14. 命名不一致**

- 类名：`WGLPlatform` vs `QNXEGLPlatform`（WGL 无后缀，EGL 有 EGL 前缀）
- 别名：`OrthographicCameraSharePtr` vs `PlatformSharedPtr`（Share vs Shared）
- 建议统一为 `XxxSharedPtr` 或 `XxxPtr`

**15. 错误处理机制**

当前缺少统一的错误处理策略。OpenGL 调用失败时（如 shader 编译错误、纹理加载失败）通常只通过 LOG_E 输出日志。建议：
- 定义 `Result<T, Error>` 类型
- 关键路径（资源加载、shader 编译）返回可检查的错误
- 提供降级渲染策略（如缺失纹理用纯色替代）
