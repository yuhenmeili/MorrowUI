# 对外接口封装提案：完全隐藏 OpenGL ES / EGL

状态：方案定稿 v4（评审决策完成，可进入实施）
日期：2026-09-29
关联文档：`ARCHITECTURE.md`（架构不变量）、`RESOURCE_HANDLE_REFACTOR.md`（Hw 句柄）

> 更新记录：
>
> - v4（2026-09-29）：合入 API 稳定性评审结论——明确**源码兼容**目标、新增
>   **公共路径与内部结构解耦**硬规则、**薄头厚 cpp** 原则、semver + 废弃缓冲
>   演进策略（§2.1 G6、§7.5）、新增门禁 M5（公共 API 一致性测试）与
>   `version.h`（§7.2、§8）。
> - v4.1（2026-09-29）：Phase 0/1 实施完成，同步实施偏差（§3.2 AtlasParser 死
>   字段删除、§6.1 前向声明方案、§9 Phase 1 状态与验证数据）。
> - v3（2026-09-29）：五项评审决策落稿（§12）：① 维持静态库，不升级动态库
>   （交付形态升级阶段取消、符号级门禁移除）；② `setShaderFromMemory` 直接删除；
>   ③ `Window::getSurface()` 直接删除；④ Tier A/B 白名单维持；⑤ 公共头保持
>   单类一头 + 子目录结构。
> - v2（2026-09-29）：① 评审决策——**自定义 shader 与自定义几何提交不对外开放**，
>   从 Tier B 移入引擎内部能力（§6.4、§6.5、§12）；② HMI 相关代码与 samples 已删除
>   （cd3aaec），全文同步清理 `ui/hmi`、`Safe*` 引用，`setShaderFromMemory` 现无
>   使用者，处置简化为"移除公共接口"。

---

## 1. 背景与问题

MorrowUI 面向多用户（多团队/外部集成方）交付，当前形态下**所有内部头文件都是事实公开的**：

- `src/CMakeLists.txt:239-256` 把 `src/` 根、`core`、`loader`、`extern`、`fonts`、`ui`、
  `renderer`、`renderer/device`、`renderer/resource`、`renderer/scene3d`、`platform`、
  `debug`、`gltf`、`includes/common`、shader 生成目录**全部设为 PUBLIC include**；
- Linux/Windows 下 vendor 头（glad、GLFW、KHR）也经 `includes/opengl/wgl` PUBLIC 导出
  （`src/CMakeLists.txt:121,172,255`）。

因此用户可以直接：

- `#include "GLRenderDevice.h"` 拿到 GL 上下文内的一切；
- `#include "EGLWindow.h"` / `EGLOperationsQNX.h` 拿到 `ESContext`（含 `EGLDisplay`、
  `EGLContext`、`EGLConfig`，见 `ESContext.h:13-20`）；
- `#include "OpenglUtils.h"` 直接使用 `GLenum/GLint` 级别的映射工具；
- 直接使用废弃 `Shader` 类的 public 成员 `GLuint m_ID`（`Shader.h:103`）篡改 program；
- 通过 `Window::getSurface()`（`Window.h:90`）拿 `void*` 强转回 `GLFWwindow*`/
  `EGLSurface`，绕过引擎直接操作窗口系统（editor 已经在这么用，见 §3.3）。

更严重的是**传递泄漏**：任何用户代码只要 `#include "Engine.h"`，就会沿

```text
Engine.h:10 → Window.h:10 → UIWidget.h:9-10 → MeshRenderer.h:12
    → RenderDeviceProxyBase.h:12 → GLRenderDevice.h:8-13
    → EGLHeader.h + GLESHeader.h（或 wgl/OpenglHeader.h）
```

在编译期被动拿到全部 GLES/EGL/glad 头文件。`Material.h:15,17` 是第二条等价泄漏链。

后果：

1. 使用方式不可测：用户可修改引擎渲染状态、shader、EGL 上下文，引擎无法保证渲染
   不变量（painter's order、状态缓存一致性、线程规则）；
2. 演进被绑架：任何内部头文件改动都可能破坏用户代码，内部重构空间被锁死；
3. 平台差异泄漏：`OPENGL_EGL`/`OPENGL_GLFW` 宏是 PUBLIC define
   （`src/CMakeLists.txt:116,123,174`），公共头里大量 `#ifdef` 分支
   （`Shader.h:21-25` 等），用户代码也能感知平台后端差异。

## 2. 目标与非目标

### 2.1 目标

- **G1 彻底隐藏**：GLES / EGL / GL / GLFW / glad 及一切 GL 原生类型、头文件、宏，对外
  不可 include、不可调用、不可修改；
- **G2 最小对外接口**：对外只保留白名单化的组件接口（见 §5），白名单之外的符号与
  头文件对标准集成流程不可见；
- **G3 能力收口**：自定义 shader、自定义几何提交**不对外开放**（评审决策），仅引擎
  内部实现可用；对外只暴露具名内置 shader 与引擎控件的标准几何路径；
- **G4 构建期强制**：越界（include 内部头、使用 GL 类型）在编译/CI 阶段即失败，
  不依赖口头约定；
- **G5 不改变运行时架构**：短渲染路径、合批、Hw 句柄、RenderDevice/Proxy/
  CommandBuffer 结构全部保持不变。本次是**边界工程**，不是渲染抽象层重写；
- **G6 升级零改动**：引擎持续升级迭代时保证应用端**源码兼容**——应用重新编译
  链接新版库、零代码修改。由公共路径解耦、演进规则与门禁保障
  （§7.5、§8 M5）。

### 2.2 非目标

- 不引入跨图形 API 抽象（Vulkan 等后端）；GL/ES 仍是唯一后端；
- 不改变单线程/多线程提交模型与命令编码方式；
- 不实现"声明式 Effect"等高级材质系统（符合 ARCHITECTURE.md §2.1 需求驱动原则）；
  引擎内部 shader 仍为 GLSL，但该事实不进入公共 API；
- 不提供对外插件式的自定义渲染能力；未来若出现合法需求，走内部评审新增具名
  内置 shader，而非重开公共自定义入口。

## 3. 现状盘点（问题清单与证据）

### 3.1 头文件泄漏链

| 链 | 路径 | 断点 |
|---|---|---|
| A | `Engine.h:10 → Window.h → UIWidget.h → MeshRenderer.h:12 → RenderDeviceProxyBase.h:12 → GLRenderDevice.h → EGL/GLES 头` | `MeshRenderer.h:12`（header 内无 Proxy 使用，纯遗留 include） |
| B | `Material.h:15 → RenderDeviceProxyBase.h → GLRenderDevice.h`；`Material.h:17 → Shader.h:21-25 → GLESHeader` | 同上 + `Shader.h` |
| C | `Engine.h:19 → renderer/device/RenderDeviceOptions.h` | 该头本身干净（纯 POD 配置），但路径在内部目录 |

### 3.2 公共可达头中的 GL 类型点

| 文件 | 证据 | 处置 |
|---|---|---|
| `renderer/resource/Shader.h` | `:21-25` 平台宏分支 include GL 头；`:33-34` `pair<string, GLint>`；`:96` `GLuint` 参数；`:103` **public `GLuint m_ID`**。类已标注"暂时废弃"（`:40`） | 删除（先确认无引用） |
| `renderer/resource/PixelDatatype.h` | `:8-12` include GL 头；`:19,21` `GLenum` 签名 | 改用 `DriverEnums` 自研枚举 |
| `renderer/resource/PixelFormat.h` | `:12-16` include GL 头；`:23,25` `GLenum` 签名 | 同上 |
| `renderer/resource/VertexArray.h` | `:13-17` include GL 头（方法签名已 Hw 化，include 为遗留） | 删除 include |
| `renderer/atlas/AtlasParser.h` | `:29` **public 字段 `GLint uWrap = GL_CLAMP_TO_EDGE`** | 实施修订：字段全仓零使用（死字段），直接删除；未新增 `SamplerWrap` 枚举，出现真实 wrap 需求时再按 DriverEnums 枚举补 |
| `platform/utils/OpenglUtils.h` | `:9-13` include GL 头；`:20-33` `GLenum/GLint` 签名 | 现状已实现内聚（仅 GLRenderDevice.cpp 与自身 .cpp 引用，不在任何公共头链上）；物理私有化由 Phase 2 include 划分完成，本阶段无改动 |
| `renderer/device/*`（GLRenderDevice、GlResourceObjects、ResourceRegistry、ShaderBinaryCache） | 全量 `GLuint/GLenum` 与内联 `glDelete*` | 划入私有实现区（§7） |

### 3.3 原生句柄出口

- `Window.h:90` `virtual void* getSurface() const;`——WGL 实现返回 `GLFWwindow*`，
  EGL 实现返回 `EGLSurface`，无契约约定；
- editor 反例：`EditorShell.h:28` include `wgl/OpenglHeader.h`；
  `EditorShell.cpp:111,194,435` 把 `getSurface()` 强转回 `GLFWwindow*` 后调用
  `glfwGetFramebufferSize` / `glfwSetClipboardString` / 安装 glfw 键盘回调；
  `EditorShell.cpp:762` 起大量使用 `GLFW_KEY_*` 常量；
- 设备接口内仅有两处不透明出口（均可保留，属内部面）：
  `GPURenderPassDevice.h:21,23` `makeCurrent(void*)` / `present(void*)`、
  `GPUBufferDevice.h:45-54` fence 的 `void*`。

### 3.4 Shader 入口

- `Material::setShader(具名)`（`Material.h:91`）：内嵌 shader（构建期
  `cmake/EmbedShaders.cmake` 生成）优先，回退运行时读 `assets/shaders/`
  （`Material.cpp:566-607`）；
- `Material::setShaderFromMemory(name, vertSrc, fragSrc)`（`Material.h:97`）：可注入
  任意 GLSL 源码。**当前零使用者**——原使用方 `ui/hmi/Safe*` 组件已随 cd3aaec
  删除，处置因此简化为"移除公共接口"（§6.4）；
- 废弃 `Shader` 类可由源码字符串或文件路径构造，暴露 `GLuint m_ID`。

### 3.5 已经具备的基础（本次可行性的关键）

- **Hw 句柄体系已落地**：`ResourceHandle.h:23-50` 全部 GPU 资源为 `uint32_t` 值类型
  句柄（`HwTexture2D/HwVBO/HwUBO/HwSSBO/HwGPUProgram/HwRenderTarget`）；
  `Texture.h`、`Material.h` 接口已句柄化，不含 GL 类型；
- **`DriverEnums.h` 是完整的自研枚举层**（`TextureFormat/SamplerMinFilter/
  SamplerMagFilter/PixelDataFormat/PixelDataType/VertexAttributeType/CullFaceMode/
  BlendFactor/PrimitiveType`），无任何 GL 依赖；
- **`gl*` 调用高度集中**：`core/`、`ui/`、`utils/`、`loader/`、`fonts/`、`gltf/`、
  `debug/` 中 gl* 调用为 **0**；全部集中在 `GLRenderDevice.cpp`（约 182 处）、
  `GlResourceObjects.h`（内联删除）、`ShaderBinaryCache.cpp`、废弃 `Shader.cpp`；
- `Platform.h`、`Window.h`、`PlatformFactory.h` 接口层无 EGL/GLFW 类型
  （PlatformFactory.cpp:11-17 按宏分发具体实现，用户不接触具体 Platform 类）；
- Release 构建已带 `-fvisibility=hidden`（根 `CMakeLists.txt:71`），为符号隐藏留了
  铺垫（注意：静态库下该选项无实际效果，见 §7.3）。

结论：**抽象层已经就位，问题只在边界没有关闭**。工作重心是把"约定上的内部"变成
"构建上不可达"。

## 4. 总体方案：三道防线

改造后的分层：

```text
┌─ 对外（唯一公开面）────────────────────────────────┐
│  include/morrow/        ← 唯一 PUBLIC include 目录    │
│  Engine / Window / UIWidget / elements / layout      │
│  Transform / MeshFilter / MeshRenderer               │
│  Material(具名 shader) / Texture / Font / Tween      │
│  —— 零 GL/EGL/GLFW 类型、头、宏 ——                   │
│  —— 零自定义 shader / 自定义几何入口 ——              │
└──────────────┬────────────────────────────────────┘
               │ 仅经由公开头链接
┌─ 引擎实现（PRIVATE，物理留在 src/）─────────────────┐
│  core / ui / renderer / platform / fonts / gltf ...  │
│    RenderDevice（Hw 句柄接口，四个子接口）            │
│    ├─ RenderDeviceProxy / CommandBuffer / RingSlots  │
│    └─ GLRenderDevice + GlResourceObjects + Registry  │ ← 唯一 gl* 区
│    GLSL 编译入口（内置 shader 构建期内嵌）            │ ← 自定义能力仅内部
│         PlatformFactory → QNXPlatform(EGL) /         │
│                           WGLPlatform(GLFW)          │ ← 唯一窗口系统区
└────────────────────────────────────────────────────┘
```

本次落地前三道防线（评审决策：维持静态库，第四道符号边界不采用——静态库形态下
符号级强制不可行，残余风险经评审接受，见 §7.3）：

| 防线 | 手段 | 挡住什么 |
|---|---|---|
| 1 物理隔离 | 公共头独立目录 `include/morrow/`，实现目录全部 PRIVATE include | 用户 `#include` 内部头 |
| 2 头文件净化 | 公共头零 GL 类型/GL 头/平台宏分支；GL 后端对公共头不可见（前向声明，工厂收敛） | 传递泄漏 |
| 3 构建边界 | 负向编译测试 + 公共头扫描 CI 门禁 | 无意越界、回归 |

## 5. 对外最小组件接口（白名单提案）

分三级管理。分级即文档，公共头目录的文件清单 = Tier A + Tier B 明细。

### 5.1 Tier A — 完全公开（业务集成的全部所需）

| 模块 | 头文件（迁移后路径示意） | 说明 |
|---|---|---|
| 引擎生命周期 | `Engine.h`、`EngineEvents.h` | `EngineOptions` 含窗口、线程模式、按需渲染 |
| 窗口 | `Window.h`、`WindowInfo` | `getSurface()` 收敛（§6.3） |
| UI 树 | `UIWidget.h`、`elements/*`、`layout/*`、`controllers/*` | 现有 MR* 控件全量保留 |
| 组件 | `Transform.h`、`Interaction.h`、`MeshFilter.h`/`MeshRenderer.h`（引擎控件标准路径） | 自定义几何提交不对外（§6.5） |
| 事件 | `TouchEvent.h`、`EventDispatcher/EventConnection`、`Observable` | |
| 资源 | `Texture.h`（加载/内存/更新，Hw 化）、`TextureAtlas.h`、`StaticAtlasManager.h` | |
| 文本 | `FontManager.h`、`MRLabel` 等（需 pimpl 切断 stb_truetype 传递，见 §6.6） | |
| 动画 | `Tween.h` | |
| 帧上下文 | `FrameState.h`（只读视图） | |
| 数学 | `math/`（无 GL 依赖，维持现状） | |
| 日志 | `Log.h` | |

### 5.2 Tier B — 受控公开（评审后开放 / 编译开关）

| 模块 | 现状入口 | 受控方式 |
|---|---|---|
| 3D 场景 | `MR3DSceneView`、`Scene3DPassContext`、`IBLPrecompute` | 保留；`IBLPrecompute` 从 samples 相对路径 include 改为正规路径 |
| 诊断 | `ObjectRegistry` 快照（`MORROW_ENABLE_OBJECT_DIAGNOSTICS`） | 已有开关，维持 |
| 调试覆盖层 | `EngineOptions::debugOverlayVisible` | 已有开关，维持 |
| 平台注入 | `WindowInfo` 原生窗口句柄（QNX screen 集成） | 只进不出（§6.3） |

> 明确不开放（评审决策）：自定义 shader（`setShaderFromMemory`）、自定义几何提交
> （`MeshFilter` 自定义顶点布局/直连上传）。两者均为引擎内部能力，见 §6.4、§6.5。

### 5.3 Tier C — 内部隐藏（不可见、不可链接）

- `renderer/device/` 全部：`RenderDevice` 四子接口、`GLRenderDevice`、
  `RenderDeviceProxy(Base)`、`RenderingThread`、`CommandBuffer`、
  `GlResourceObjects`、`ResourceRegistry`、`ShaderBinaryCache`、`PlatformSemaphore`；
- `renderer/` 内部：`BatchManager/BatchBuilder/BatchStatistics`、`GpuTypes`、
  `SSBO` 内部（`SSBOLayout` 注册表、`ShaderReflection`）、`EmbeddedShaders`、
  `AtlasParser`（经 `TextureAtlas` 公共面间接使用，头内部化）；
- shader 源码注入与 GLSL 编译入口：`Material::setShaderFromMemory` 移除出公共
  `Material`，如内部需要经实现层内部入口（§6.4）；
- `MeshFilter` 自定义几何构造/上传接口内部化（§6.5）；
- `platform/` 实现：`egl/*`（EGLHeader、GLESHeader、ESContext、EGLOperations*、
  EGLWindow、QNXPlatform）、`wgl/*`（OpenglHeader、WGLPlatform、WGLWindow、glad）、
  `OpenglUtils`；
- 废弃 `Shader` 类（`renderer/resource/Shader.h`）——**删除**；
- `extern/`（stb、nlohmann、basis_universal）与 `includes/`（vendor GL/GLFW 头、
  `eglextQCOM.h`）；
- `core/` 内部（`MainThreadDispatcher`、`BatchBuilder` 等）。

## 6. 关键设计点

### 6.1 切断 ProxyBase → GLRenderDevice 的头依赖

问题核心：`RenderDeviceProxyBase.h:12` include `GLRenderDevice.h`，`:46` 持有
`GLRenderDevicePtr m_realDevice` 成员，把后端具体类型焊进了头文件。

调整：

1. `MeshRenderer.h:12`、`Material.h:15` 删除对 `RenderDeviceProxyBase.h` 的 include
   （两个头文件内均无 Proxy 使用；如 .cpp 需要则移入 .cpp）；
2. `RenderDeviceProxyBase.h` 对后端只留**前向声明**：`class GLRenderDevice;` +
   `std::shared_ptr<GLRenderDevice> m_realDevice`（实施修订：`GLRenderDevice` 并非
   `RenderDevice` 子类——前端命令接口由 Proxy 自身实现，二者是平行层次——因此
   不改持接口指针，前向声明即可让头文件零 GL 依赖；共享指针成员配合 .cpp 内
   析构定义不要求完整类型）；
3. `GLRenderDevice` 的构造收敛到**唯一工厂点**（`RenderDeviceProxyBase.cpp` 内部或
   独立 `DeviceFactory.cpp`，由 `Platform` 图形上下文就绪后调用），
   `#include "GLRenderDevice.h"` 只出现在该 .cpp；
4. 现有不变量保持：多线程模式不得绕过 Proxy 直接调用 GLRenderDevice
   （ARCHITECTURE.md §12.4），且此后**从语言层面也无法绕过**——外部拿不到该类型。

### 6.2 公共头 GL 类型清理

按 §3.2 表逐项执行，原则：

- 公共头只允许出现 `DriverEnums.h` 的自研枚举与 `Hw*` 句柄；
- GL 常量/枚举映射（`OpenglUtils` 职责）全部下沉到对应 .cpp；
- `DriverEnums.h` 新增 `enum class SamplerWrap : uint8_t`（替代
  `GL_CLAMP_TO_EDGE` 等）；
- 平台宏 `OPENGL_EGL`/`OPENGL_GLFW` 从 PUBLIC define 降为 PRIVATE（`src/CMakeLists.txt`
  中 `MORROW_PUBLIC_DEFINES` 改 PRIVATE 传入）。前置条件是公共头不再有
  `#ifdef OPENGL_*` 分支（§3.2 清单完成后即满足）。
  `MORROW_ENABLE_BASISU`/`MORROW_ENABLE_OBJECT_DIAGNOSTICS`/
  `MORROW_ENABLE_DEBUG_OVERLAY` 等功能宏同步评估：公共头需要的（如
  `EngineOptions::debugOverlayVisible` 的编译裁剪语义）保留为 PUBLIC，其余 PRIVATE。

### 6.3 Window 原生句柄：只进不出

1. **注入点保留**：QNX screen 集成、外部创建的窗口等场景，通过 `WindowInfo` 提供
   原生句柄（`void*`/`intptr_t`），引擎持有但**不回吐**；
2. `Window::getSurface()` **直接删除**（评审决策），替代能力 API：
   `Window::framebufferSize()`、`Window::setClipboardText()/clipboardText()`
   （平台实现内部分发，WGL 走 glfw、EGL 走 screen）；
3. editor 迁移：键盘回调/剪贴板/framebuffer 尺寸改走上述引擎 API；键盘键值定义
   引擎级 `KeyCode`（以 `TouchEvent.h:22` 的 `TouchKeyCode` 体系扩展 desktop 键值），
   替代 `GLFW_KEY_*`；
4. 多窗口回调管理（editor 当前的 `map<GLFWwindow*, ...>`）由引擎 InputProvider
   生命周期接管，editor 只消费语义事件。

### 6.4 Shader 入口：对外仅具名内置 shader

1. **对外唯一入口**：`Material::setShader(具名)`。具名 = 内置白名单（embedded
   shaders，`assets/shaders/` 构建期内嵌），参数走 `Material` 参数接口
   （`setVector/setFloat/setTexture/...`）。用户接触不到 GLSL、program、uniform 位置；
2. **移除 GLSL 注入入口**：`Material::setShaderFromMemory`（`Material.h:97`）
   **直接删除**（评审决策）。该入口当前零使用者（原 `ui/hmi/Safe*` 已随 cd3aaec
   删除）；未来引擎内部组件需要时，经实现层内部入口恢复（internal 命名空间
   自由函数或 `Material` 私有通道），不进公共头；
3. **删除废弃 `Shader` 类**（`Shader.h/cpp`）：它是唯一直接暴露 `GLuint` 的公开 API，
   也是泄漏链 B 的一环。删除前全仓确认引用；
4. **无需编译开关**：自定义能力不对外，`MORROW_ENABLE_CUSTOM_SHADER` 之类的外部
   开关不再需要；引擎内部 shader 编译/内嵌机制（`EmbedShaders.cmake`、
   `common/*.glsl` chunk、`ShaderBinaryCache`）全部随实现层隐藏，作为内部实现细节
   自由演进；
5. `assets/shaders/` 运行时文件回退（`Material.cpp:566-607`）仅服务于内置具名
   shader 的开发调试，语义不变。

### 6.5 渲染状态与几何入口收编

- **组件级语义状态保留公开**：`Material` 的 blend/cull、`MeshRenderer` 绘制开关
  等——这些是 UI 语义（半透明、双面、裁剪参与），属于组件接口的一部分，不是
  "GL 状态"；
- **引擎级渲染状态封闭**：viewport、clear、stencil、GL state cache、RenderTarget
  绑定全部随 `renderer/device/` 隐藏（现状已基本满足，本提案补上物理边界 + CI
  门禁防回归）；
- **自定义几何提交不对外**：`MeshFilter` 的自定义顶点布局构造与直连上传接口
  内部化；`MeshRenderer/MeshFilter` 保留在公共面的原因是引擎控件（MRImage、
  Shadow、文本等）的几何管线经由此层，属组件基础设施——对外承诺的是"控件"，不是
  "自定义几何"；几何能力扩展走引擎内部新增控件/新增内置 shader 路径；
- `BatchManager/BatchBuilder` 保持内部（现有 CPU 单测继续在实现目录内编译，
  不受影响）。

### 6.6 第三方头内部化

- `includes/common/`（stb_*、nlohmann、**eglextQCOM.h**）与 `includes/opengl/wgl/`
  （glad、GLFW、KHR）全部退出 PUBLIC include；
- `eglextQCOM.h` 属于芯片扩展定义，尤其不应外流；
- 字体公共面检查传递依赖：`DynamicFont.h:17 → stb_truetype.h`，公共化时对
  `FontManager`/字体入口使用 pimpl 或接口拆分，stb 留在 .cpp；
- 公共 API 当前无 json 依赖面（ObjectRegistry 快照为内部调试能力），nlohmann 无需
  对外暴露。

## 7. 构建与交付形态

### 7.1 CMake 目标与 include 划分

最小改动方案（单 target）：

```cmake
add_library(morrow STATIC ${MORROW_SRC})
# 对外唯一可见：
target_include_directories(morrow PUBLIC  ${CMAKE_SOURCE_DIR}/include)
# 实现自用（不再传递）：
target_include_directories(morrow PRIVATE
    src src/core src/ui src/renderer src/renderer/device
    src/renderer/resource src/renderer/scene3d src/platform
    src/fonts src/loader src/gltf src/debug src/math
    ${MORROW_SHADER_GENERATED_DIR})
target_compile_definitions(morrow PRIVATE ${MORROW_PRIVATE_DEFINES})  # OPENGL_EGL 等
```

- `math/utils` 已是独立 target（`morrow_math`/`morrow_utils`），其接口如需公开则
  同样只允许公共头目录出口；
- `editor` 与 `samples` 作为**边界回归样板**：只允许 include `include/morrow` 下的头
  （见 §8 M4）。

### 7.2 公共头目录组织

```text
include/morrow/
  version.h           # MORROW_VERSION_* 宏（唯一新增的非类头）
  Engine.h  Window.h  WindowInfo.h  RenderDeviceOptions.h
  UIWidget.h  Texture.h  TextureAtlas.h  Material.h ...
  elements/  layout/  controllers/  effects/
  math/  DriverEnums.h  ...
```

- 迁移策略：**移动 + 内部转发头过渡**。Phase 2 先把 Tier A/B 头移入
  `include/morrow/`，原 `src/` 位置留一次性转发头（`#pragma once` +
  相对 include），实现内部 include 不用一次性改完；过渡期结束后删除转发头；
- `RenderDeviceOptions.h` 本身干净（纯 POD），提升为公共头（`Engine.h` 直接引用它）；
- 头粒度维持现有**单类一头 + 子目录**结构（评审决策），不引入 umbrella 头；
- **公共路径与内部结构解耦（硬规则）**：公共目录只允许产品域词汇
  （`elements/`、`layout/`、`controllers/`、`effects/` 与根上核心头），
  `renderer/`、`core/`、`platform/` 等实现域路径不得出现在公共面——src/ 的
  任何重构永不映射到公共路径，应用 `#include` 一行不动（§7.5）；
- 命名空间不变（`morrow`），不引入 `morrow::api` 之类的二级命名空间，避免无谓的
  代码翻新。

### 7.3 库形态与符号边界（评审决策：维持静态库）

决策：**维持静态库 + 头级隔离，不升级动态库**（§12-1）。

- 落地方式即 §7.1/§7.2 的 PUBLIC/PRIVATE include 划分 + §8 CI 门禁；
- 强度边界：标准集成流程（include 公共头 + 链接库）不可越界；静态库形态无法
  阻止有意者硬声明内部符号链接——`-fvisibility=hidden` 对静态档案无效，
  version script 只约束动态符号表，该残余风险经评审接受；
- 若未来交付要求符号级强制或稳定 ABI，再独立评估动态库 + `MORROW_API` 导出
  （Release 已有的 `-fvisibility=hidden`，根 `CMakeLists.txt:71`，届时才真正生效），
  不纳入本提案范围。

### 7.4 安装与消费（可选，按外部交付需求触发）

当前仓库无任何 `install()` 规则，消费模式是仓库内 `add_subdirectory`。静态库形态
同样可以安装交付（头文件 + .a/.lib），外部多用户交付时补齐：

```cmake
install(DIRECTORY include/morrow DESTINATION include)
install(TARGETS morrow ARCHIVE DESTINATION lib EXPORT morrowConfig)
install(EXPORT morrowConfig DESTINATION lib/cmake/morrow)
```

外部工程只拿到 `include/morrow` + 库文件，物理上不存在内部头。

### 7.5 API 稳定性与演进策略

目标定义：静态库形态下，"升级不改应用代码" = **源码兼容**——应用重新编译并
链接新版库，零代码修改。不追求二进制兼容（那需要 pimpl 全员化或 C ABI 边界，
成本远超收益，见第 5 条）。

1. **公共路径与内部结构解耦（硬规则）**：`include/morrow/` 是独立设计的产品面，
   目录只允许产品域词汇；`renderer/`、`core/`、`platform/` 等实现域路径不得出现在
   公共面。src/ 的任何重构（挪文件、拆并模块、改名）**永不映射到公共路径**。
   头文件粒度（umbrella vs 单类一头）本身不决定稳定性，路径解耦才是；
2. **薄头厚 cpp**：公共头只放声明（类、方法签名、枚举），实现全部在 .cpp。
   头里的 inline 实现、模板、默认参数值都属于被冻结的接口面，越少越好——
   声明几乎不变的头 + 自由迭代的 .cpp，正是"接口稳定、内部自由"的直接兑现。
   pimpl 仅用于少数需要最大演进空间的类（如 `FontManager`，背后拖着
   stb_truetype），不做全员化（静态库 + 源码兼容目标下加成员变量本就不破坏
   源码兼容，全员化收益有限）；
3. **只加不改的演进规则**：新增类、方法、重载、带默认值的参数均为源码兼容；
   删除与改签名只允许出现在大版本。枚举只增不删，且要求业务 switch 不对
   枚举做穷尽假设（新增枚举值不破坏编译）；
4. **版本化与废弃缓冲**：`version.h` 提供 `MORROW_VERSION_MAJOR/MINOR/PATCH`，
   语义化版本管理；废弃 API 以 `[[deprecated("use X since 1.y")]]` 标记，
   保留至少两个小版本，大版本才删除。废弃缓冲是"接口稳定"与"引擎持续迭代"
   之间的关键缓冲带；
5. **不采用**：umbrella 头（编译耦合放大、掩盖真实 API 依赖面，§7.2 已决策）；
   C ABI + 不透明句柄（最强稳定形态，但静态库 + 源码兼容目标下属过度设计，
   现有 `Hw*` 句柄文化已覆盖其核心收益）。

## 8. 防越界机制（CI 门禁）

| 编号 | 门禁 | 实现 |
|---|---|---|
| M1 | 公共头零 GL 词汇 | 脚本扫描 `include/morrow/**`：禁止出现 `#include` GLES/EGL/gl/glfw/glad 及 token `GLuint|GLint|GLenum|GLbitfield|EGL[A-Z]|GLFW`；命中即失败 |
| M2 | 负向编译测试 | CMake `try_compile` 一个 `#include "renderer/device/GLRenderDevice.h"`（及 EGLWindow.h、OpenglUtils.h 等代表样本）的 TU，**必须编译失败**，否则配置失败 |
| M3 | include 图校验 | 公共头不得 include `include/morrow` 之外的任何引擎头（脚本沿 `#include "` 递归校验，防止转发头/相对路径绕过） |
| M4 | 边界回归样板 | samples/editor 全量仅 include 公共面；samples 中出现 `src/` 相对路径 include（如 `GLTFDemo.cpp:1` 的 `../src/ui/helpers/...`）视为失败 |
| M5 | 公共 API 一致性测试 | 独立测试目标，覆盖全部公开类/方法/枚举的使用方式，随引擎每次构建编译；任何破坏源码兼容的改动（删类、改签名、移除枚举值、公共路径变更）使其编译失败，发版前拦截 |

M1/M2 成本极低（纯 CMake/脚本），Phase 1 结束即可上线，长期防止边界回退。
（编号 M* 为 CI 门禁，与 §2 的目标 G* 相互独立。）

## 9. 实施路线

各阶段独立可交付、可暂停；每阶段要求 Windows/Linux/QNX 三平台编译通过 +
单线程/多线程渲染一致 + 现有 samples 目检无回归。

### Phase 0 — 冻结与基线（无行为变化）

- 定稿 Tier A/B/C 白名单（即本文档 §5 评审通过版）；
- 记录泄漏基线（当前 `Engine.h` 预处理产物含 GL 头清单），供 Phase 1 验收对照；
- 盘点外部用户代码对内部头的实际依赖（如已有集成方）。

### Phase 1 — 头文件净化与能力收口（不动构建结构）

> 状态：✅ 已完成（2026-09-29）。Windows/MinGW(Ninja) 全量构建通过（morrow +
> 15 samples + editor + 26 组测试；BuildQueueTests 失败为既有环境问题——其默认
> "MinGW Makefiles" 生成器在本机不可用，与本批改动无关）；DebugDemo
> `--report-json --frames 30` 真实渲染链路合批断言通过；M1 门禁接入构建
> （`morrow_gate_public_api` 目标，随 morrow 依赖执行）：Engine.h 闭包 71 头
> （全量 GL 泄漏）→ 60 头、零 GL/EGL/GLFW 词汇。实施偏差见 §3.2/§6.1 修订。
> QNX/Linux 交叉编译待验证。

任务：

1. §6.1：删 `MeshRenderer.h:12`、`Material.h:15` 的 ProxyBase include；
   ProxyBase 改持 `unique_ptr<RenderDevice>` + 工厂收敛；
2. §6.2：`Shader` 废弃类删除；`PixelDatatype/PixelFormat/VertexArray` 去 GL；
   `AtlasParser` 改 `DriverEnums::SamplerWrap`（新增枚举）；`OpenglUtils` 只剩 .cpp；
3. §6.4：`Material::setShaderFromMemory` 直接删除（当前零使用者）；
4. editor 本阶段允许继续用内部头（Phase 2 统一迁移）；
5. 上线 CI 门禁 M1（对 `src/core/Engine.h` 传递闭包扫描）与 M2 雏形。

验收：对 `Engine.h` 做预处理（`-E`），产物中无任何 GL/EGL/glad/GLFW 头与
`GLuint/GLenum` token；公共 `Material` 无 shader 源码注入入口；samples 零改动
编译运行正常。

风险：`MeshRenderer/Material` 的 .cpp 可能隐式依赖被删 include（编译期即可暴露，
低风险）；`SamplerWrap` 改动触及 atlas 默认值语义，需确认 `GL_CLAMP_TO_EDGE` 行为
等价映射。

### Phase 2 — 构建边界（本提案主体落地）

任务：

1. 建立 `include/morrow/`，按 §7.2 迁移 Tier A/B 头（移动 + 转发头过渡），
   新增 `version.h`；
2. §7.1 CMake PUBLIC/PRIVATE 划分；平台宏/功能宏降 PRIVATE（公共头已无分支依赖）；
3. samples/editor 收敛到公共面：
   - `TextureAtlas.h`、`Texture.h` 已在 Tier A（samples 现有 include 路径改公共路径）；
   - `IBLPrecompute` 走正规公共路径（Tier B）；
   - editor 落地 §6.3 能力 API（`framebufferSize`/剪贴板/KeyCode），删除
     `OpenglHeader.h` include 与 `getSurface()` 强转；
4. 删除 `Window::getSurface()`（评审决策）；
5. CI 门禁 M1~M4 全量生效；建立公共 API 一致性测试集（M5）雏形，覆盖 Tier A
   全部公开签名。

验收：samples + editor 在仅公开 include 路径下编译运行；M2 负向测试生效；
`grep -r "getSurface" samples editor` 为空。

风险：本阶段触碰公共头物理位置，属破坏性布局变更——转发头过渡期内外部用户
include 路径兼容；`FontManager`/字体 pimpl 切断 stb 传递若牵扯较大，可单独子阶段。

### Phase 3 — 契约固化

- 对外 shader 契约固化为"仅具名内置 shader + Material 参数接口"文档；
- 验证 `setShaderFromMemory`/自定义几何入口在公共面已不可达（配合 M1~M5 门禁）；
- M5 覆盖补全至 Tier B（3D 场景、诊断入口）；
- API 演进策略（§7.5：只加不改、semver、废弃两版本缓冲）写入对外集成指南并生效；
- ARCHITECTURE.md §14 增补不变量（§10）；
- 对外集成指南（`docs/` 下新增：最小接口说明 + 迁移对照表）。

## 10. 拟新增架构不变量（并入 ARCHITECTURE.md §14）

15. 公共 API 头（`include/morrow/`）不得出现 GL/EGL/GLFW 类型、头文件、宏与
    平台宏分支；GL 后端仅 `renderer/device/` 与 `platform/` 实现可见；
16. `gl*` 调用只允许存在于 `GLRenderDevice`、`GlResourceObjects`、
    `ShaderBinaryCache` 与废弃代码清理后的等价私有实现内；
17. 原生窗口句柄只进不出：经 `WindowInfo` 注入，引擎不回吐可强转的窗口/表面句柄；
18. 对外 shader 入口仅具名内置 shader；GLSL 源码注入与自定义几何提交是引擎内部
    能力，不进入公共 API；废弃 `Shader` 类不得复活为公开 API；
19. 公共头路径是独立的产品面：目录只允许产品域词汇，src/ 内部重构永不映射到
    公共路径；公共头只放声明（薄头厚 cpp）；API 演进遵循"只加不改 + 语义化
    版本 + 废弃两版本缓冲"（§7.5）。

## 11. 风险与兼容性

| 风险 | 影响 | 缓解 |
|---|---|---|
| 外部存量代码 include 了内部头 | 升级编译失败 | 转发头过渡期（Phase 2）；发布迁移对照表（旧头 → 公共替代） |
| `Shader` 废弃类删除 | 依赖 `GLuint m_ID` 的用户代码失败 | 全仓确认 + 迁移说明；类本已标注废弃 |
| editor GLFW 直接依赖 | editor 需同步改造 | §6.3 能力 API 与 editor 迁移同阶段交付 |
| QNX 集成点 | screen/EGL 集成需要原生句柄 | `WindowInfo` 注入点保留；集成侧仅在注入时接触原生类型 |
| `SamplerWrap`/枚举替换语义偏差 | 纹理环绕行为变化 | 等价映射表 + ImageDemo/图集用例目检 |
| Phase 1 触碰核心头 | 渲染回归 | 当前干净基线起步；BatchBuilder 单测 + samples 目检；Phase 1 改动小且编译期可验证 |
| 未来出现合法的自定义渲染需求 | 需求无公开出口 | 走引擎内部路径：新增内置 shader（构建期内嵌）或新增控件，经评审合入；不重开公共自定义入口 |
| 内部组件曾用 `setShaderFromMemory` | 无 | 使用方已随 cd3aaec 删除，当前零使用者，无迁移负担 |

## 12. 评审决策记录（2026-09-29）

| # | 问题 | 决策 |
|---|---|---|
| 1 | 库形态 | **维持静态库 + 头级隔离，不升级动态库**；符号级门禁（原 G4 符号抽查）随之移除（§7.3） |
| 2 | `setShaderFromMemory` | **直接删除**；未来内部需要时经实现层内部入口恢复（§6.4） |
| 3 | `Window::getSurface()` | **直接删除**，以能力 API 替代（§6.3） |
| 4 | Tier A/B 白名单 | 维持 §5 清单，无增删 |
| 5 | 公共头粒度 | 保持**单类一头 + 子目录**结构，不引入 umbrella 头（§7.2） |
| 6 | API 稳定性策略（追加评审） | 源码兼容目标 + 公共路径与内部结构解耦 + 薄头厚 cpp + 只加不改/semver/废弃两版本缓冲 + M5 一致性测试门禁（§2.1 G6、§7.5） |

---

## 附：改动量级评估

| 阶段 | 头文件改动 | .cpp 改动 | 构建脚本 | 外部代码影响 |
|---|---|---|---|---|
| Phase 1 | ~8 个（§3.2 清单 + Material.h） | ProxyBase/工厂 + 枚举映射下沉 + setShaderFromMemory 删除 | 无 | 零 |
| Phase 2 | Tier A/B 头迁移（移动 + 转发头） | editor 能力 API 化 | src/CMakeLists.txt 根 CMakeLists.txt CI | samples include 路径、editor GLFW 调用点 |
| Phase 3 | 无 | 无 | 无 | 契约文档 |
