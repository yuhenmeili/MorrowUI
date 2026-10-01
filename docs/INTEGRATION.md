# MorrowUI 对外集成指南

状态：生效（Phase 3 契约固化产出）
日期：2026-09-30
关联：[road_map/PUBLIC_API_ENCAPSULATION_PROPOSAL.md](road_map/PUBLIC_API_ENCAPSULATION_PROPOSAL.md)（边界工程与门禁）、
[ARCHITECTURE.md](ARCHITECTURE.md) §14（架构不变量 15~19）

本指南是外部集成方与应用开发者的唯一契约文档：如何接入、能用什么、不能用什么、
旧代码怎么迁、升级时引擎承诺什么。公共 API 面的冻结清单以
`cmake/morrow_public_entries.cmake` 为准（M3 门禁），本指南是其人读版。

---

## 1. 集成方式

```cmake
add_subdirectory(MorrowUI)                 # 或按交付包引入
target_link_libraries(your_app PRIVATE morrow)
```

- `morrow` 静态库以 `PUBLIC include` 唯一导出 `include/` 目录，所有公共头经
  `morrow/` 前缀引用：`#include "morrow/Engine.h"`；
- 数学库 `morrow_math` 随 morrow 传递链接，其头为平名交付
  （`#include "Vector3.h"`），无 GL 依赖；
- 平台前提：Windows/Linux 桌面走 GLFW（Windows 下仓库自带 `libs/GLFW` 链接目录，
  Linux 需系统 glfw3）；QNX 走 EGL/screen（需 QNX SDK）；
- 版本信息：`morrow/version.h`（`MORROW_VERSION_MAJOR/MINOR/PATCH`，
  当前 0.1.0），应用可据此做特性开关。

构建选项（CMake，功能宏同时作为 PUBLIC define 传递给应用编译）：

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `MORROW_ENABLE_BASISU` | ON | Basis/KTX2 纹理转码支持；纯 png/jpg 资产可关闭以缩小二进制 |
| `MORROW_ENABLE_OBJECT_DIAGNOSTICS` | Debug ON / Release OFF | 对象注册快照（`Engine::writeObjectSnapshot`） |
| `MORROW_ENABLE_DEBUG_OVERLAY` | Debug ON / Release OFF | 运行时 FPS/批统计覆盖层（F3 切换） |

## 2. 最小接口说明（公共面）

对外产品面 = 「引擎入口 + UI 组件 + 材质/纹理 + 相机」四类，另加受控的诊断面：

| 域 | 头（省略 `morrow/` 前缀） | 说明 |
| --- | --- | --- |
| 引擎入口 | `Engine.h`、`EngineEvents.h`、`WindowInfo.h`、`FontInfo.h`、`RenderDeviceOptions.h`、`version.h` | 生命周期、窗口配置（`WindowInfo`）、字体注册（`FontInfo` + `Engine::addFonts`）、清屏色/剪贴板/光标/背景模糊运行时开关、原始键盘与帧尺寸事件 |
| UI 树与控件 | `base/UIWidget.h`、`base/Root2D.h`、`base/{Transform,Interaction,Shadow,TouchEvent}.h`、`elements/MR*.h`、`layout/*.h`、`controllers/OrbitController.h`、`helpers/Tween.h` | 全部 MR* 控件、布局容器、Tween 动画；UI 挂载经 `Engine::getRootWidget()` |
| 事件 | `base/EventDispatcher.h`、`core/Observable.h` | 事件分发与可观察对象 |
| 资源 | `Texture.h`、`Material.h`、`DriverEnums.h`、`ResourceHandle.h`、`StaticAtlasManager.h`、`atlas/TextureAtlas.h` | 纹理加载/内存构造/更新、材质参数、自研枚举与 Hw 句柄 |
| 相机与 3D | `Camera.h`、`PerspectiveCamera.h`、`OrthographicCamera.h`、`OrbitCamera.h`、`elements/MR3DSceneView.h`、`helpers/Scene3DAsyncLoader.h`、`scene3d/{FrameState,IBLPrecompute}.h`、`GLTFTypes.h` | 3D 场景视图、异步加载、IBL 预计算、GLTF 公共数据模型 |
| 文本 | `elements/{MRLabel,MRRichTextLabel,MRTextEdit,MRLineEdit}.h` | 文字面 = 控件 + 字体名参数；字体加载管理为引擎内部能力 |
| 诊断（Tier B） | `debug/ObjectRegistry.h`、`BatchStatistics.h` | 对象快照与批统计，随 `MORROW_ENABLE_OBJECT_DIAGNOSTICS` 编译 |
| 工具 | `utils/{Log,GlobalTools}.h` | 日志与全局工具（随 `morrow_utils` 交付） |

**承诺边界**：上表入口（= 冻结白名单）是受演进策略保护的承诺面。`include/morrow/`
下还有少量头（如 `base/MeshFilter.h`、`base/Mesh.h`、`atlas/Animation.h`）是入口头
include 链的传递依赖——物理上可达，但**不作为独立承诺面**：它们是控件几何/图集
基础设施，接口随引擎控件需要自由演进，业务代码不应直接依赖（见 §4）。

## 3. Shader 契约（对外唯一 shader 入口）

对外 shader 能力收敛为一个入口 + 一组参数接口：

```cpp
material->setShader("image_normal");        // 具名内置 shader
material->setTexture("u_texture", texture); // 参数按名设置
material->setVector("u_tint", Vector4(1, 1, 1, 1));
material->setFloat("u_alpha", 0.8f);
```

- `Material::setShader(const std::string& shaderName)`：具名 = 构建期内嵌的内置
  shader（`assets/shaders/`，开发调试期运行时回退读取同名文件）；
- 参数接口：`setTexture / setVector(Vector2/3/4) / setMatrix4 / setFloat /
  setInt / setIntArray`；
- 用户接触不到 GLSL 源码、program 句柄、uniform 位置与任何 GL 类型。

**内置 shader 清单（v0.1.0，冻结只增不改）**：

| 分类 | 名字 | 使用方 |
| --- | --- | --- |
| 控件外观 | `button`、`texture_button`、`image_normal`、`default_color`、`default_image`、`progress_bar`、`shadow`、`font`、`font_shadow` | 按钮族、MRImage、图元、进度条、Shadow、文本 |
| 动画与特效 | `bounce`、`anchor_point_scale`、`brake_pedal`、`flowing_light`、`frame_animation`、`gears_iris`、`gears_opening`、`gears_select`、`gears_shine`、`canvas_modulate`、`image_text_debug` | 对应 MR* 特效控件与调试叠加 |
| 引擎内部 pass | `backdrop`、`backdrop_composite`、`backdrop_downsample`、`backdrop_kawase`、`backdrop_tint`、`image_oes`、`particle_cpu`、`particle_gpu`、`gltf_pbr`、`gltf_unlit`、`scene3d_display`、`text_postprocess` | 背景模糊链、OES 外部纹理、粒子、GLTF 材质、3D 显示、文本后处理（随控件/子系统自动使用） |

契约规则：

1. 已列名字在个大版本内不删除、不改语义；新增名字属于版本动作（引擎内部评审合入）；
2. "引擎内部 pass"类随子系统自动绑定，不构成用户 Material 的选型承诺；
3. 需要"目前没有的渲染效果"时的合法路径：提需求走引擎侧新增控件/新增内置 shader，
   不重开自定义 GLSL 入口。

## 4. 边界约束（越界即编译失败）

| 约束 | 内容 | 保障 |
| --- | --- | --- |
| 零 GL/EGL/GLFW | 公共面无 GL 系类型、头文件、宏与平台宏分支；`renderer/`、`platform/` 等实现目录对标准集成流程不可达 | M2 负向编译（configure 期），M1 公共头扫描（随构建） |
| 无自定义 shader | `setShaderFromMemory` 已删除；GLSL 编译入口全部内部 | 接口不存在 |
| 无自定义几何承诺 | `Mesh`/`MeshFilter`/`MeshRenderer` 是控件几何基础设施（非承诺面，§2）；对外承诺的是控件，不是自定义顶点管线 | 演进不受兼容约束 |
| 原生句柄只进不出 | 外部窗口经 `WindowInfo` 原生句柄**注入**；引擎不回吐可强转的窗口/表面句柄（`getSurface` 已删除） | 接口不存在 |
| 单窗口假设 | 窗口由引擎创建维护；`EngineOptions::windowInfo` 配置 | 公共面无 Window 类型 |

仓库内 `editor/` 不构成集成样板——它是引擎开发工具，经评审豁免使用实现私有头；
应用侧唯一正确姿势是 `morrow/` 公共路径（`samples/` 是样板）。

## 5. 迁移对照表（旧内部用法 → 公共替代）

面向 Phase 2 之前接触过内部头的存量代码：

| 旧用法 | 公共替代 |
| --- | --- |
| `#include "Engine.h"` 等 src 内部/相对路径 | `#include "morrow/Engine.h"` 等公共路径 |
| `engine->getWindow()->setClearColor(...)` | `engine->setClearColor(...)` |
| `getWindow()->events()` 键盘回调、`GLFW_KEY_*` 键值 | `engine->events().onRawKeyboardInput`（`TouchEvent`/`TouchKeyCode`，含桌面键值） |
| `getWindow()->events()` 尺寸回调、`glfwGetFramebufferSize` | `engine->events().onFramebufferSizeChanged`、`engine->framebufferSize()` |
| `glfwSetClipboardString` 等剪贴板直调 | `engine->setClipboardText(...)` / `engine->clipboardText()` |
| `Window::getSurface()` 强转 `GLFWwindow*`/`EGLSurface` | 已删除；原生能力经 Engine API 补位（帧尺寸/剪贴板/光标形状），无句柄回吐 |
| `Material::setShaderFromMemory(name, vert, frag)` | 已删除；`Material::setShader(具名)` + 参数接口（§3） |
| 废弃 `Shader` 类（`GLuint m_ID`） | 已删除；同上 |
| `FontManager`/`FontGlyph` 直接操作 | `Engine::addFonts(std::vector<FontInfo>)` + 控件字体名参数 |
| `GpuTypes.h`（`TextureData`/`VBOData`/`VertexAttribute`） | `VertexAttribute`/`VertexAttributeType` → `morrow/DriverEnums.h`；其余内部化 |
| `Texture::basisData`（basisu 容器） | `std::vector<uint8_t>` 字节流（边界转换在加载器内） |
| `MeshRenderer::getVertexArray()` | 内部化（批处理内部通道） |
| `MR3DSceneView::getLighting()/getIBL()` | `setSunLight/setAmbientLight/setIBL(FromDirectory)/clearIBL` 公共设置面 |
| `SceneNode`（3D 场景节点） | 重命名为 `SceneNode3D`（`morrow/base/SceneNode3D.h`，域后缀与 `Transform3D`/`MeshRenderer3D` 一致）；2D UI 树根仍为 `Root2D` |
| `renderer/device/*`、`platform/egl|wgl/*`、`BatchManager`、SSBO/反射、`OpenglUtils` | 无公共替代——引擎内部能力 |

## 6. API 演进策略

目标：**升级零改动**——引擎升级后应用重新编译、链接新版库，零代码修改
（源码兼容；静态库形态下不承诺二进制兼容）。

1. **公共路径与内部结构解耦（硬规则）**：`include/morrow/` 是独立设计的产品面，
   目录只允许产品域词汇；src/ 内部任何重构（挪文件、拆并模块、改名）永不映射到
   公共路径，应用 `#include` 一行不动；
2. **薄头厚 cpp**：公共头只放声明；inline 实现、模板、默认参数值属于被冻结的
   接口面，保持最少；
3. **只加不改**：新增类、方法、重载、带默认值参数 = 源码兼容；删除与改签名只
   出现在大版本；枚举只增不删——业务 switch 不应对枚举做穷尽假设；
4. **语义化版本**：`version.h` 三段宏。当前 0.x 阶段公共面以"只加不改"为原则，
   如需破坏性变更随 minor 释放并更新本指南迁移表；1.0 起严格 semver；
5. **废弃缓冲**：废弃 API 以 `[[deprecated("use X since 1.y")]]` 标记，至少保留
   两个小版本，大版本才删除；
6. **门禁兜底**：M3 入口白名单 + 可达闭包随每次构建校验（`cmake/
   morrow_public_entries.cmake`）——公共头意外增删/孤儿滞留直接构建失败，
   兼容性承诺由机制而非约定保障。
