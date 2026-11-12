# 双屏渲染（两个 displayId）可行性调研与实现记录

> 调研日期：2026-11-10。针对 QNX (EGL/Screen) 平台，评估 MorrowUI 引擎
> 同时渲染到两块物理屏幕（两个 displayId）的可行性、现状缺口与改造方案；
> 第八节为已落地的多屏渲染开关（Mirror / Independent）实现记录。

## 一、结论

**可以实现，且改动量可控。**

QNX Screen 子系统原生支持多 display 多窗口；引擎在「窗口 → display 绑定」这一层
已经做好了（`WindowInfo.displayId`），真正缺的是 Engine / Platform 层的
**多窗口编排**——当前架构硬编码了「一个 Platform 只有一个 Window」。

| 维度 | 现状 |
|------|------|
| QNX Screen / EGL 平台能力 | ✅ 原生支持，无障碍 |
| 窗口绑定指定 display | ✅ 已实现（`WindowInfo.displayId`） |
| 渲染线程多 surface 切换 | 🟡 命令层就绪，缺每帧 makeCurrent |
| 每窗口 BatchManager / 相机重配置 | ✅ 已隔离 |
| Platform / Engine 多窗口编排 | ❌ 单窗口假设，需改造 |
| 输入按来源窗口分发 | ❌ 不区分来源窗口 |

---

## 二、已具备的能力（不需要动）

### 2.1 窗口绑定指定 display ✅

`WindowInfo` 已有 `displayId` 字段（`src/platform/Window.h:38`，默认 3）。
`EGLWindow::initDisplay()`（`src/platform/egl/EGLWindow.cpp:91-120`）：

1. `SCREEN_PROPERTY_DISPLAY_COUNT` 获取屏数量；
2. `SCREEN_PROPERTY_DISPLAYS` 枚举所有 display，逐个读 `SCREEN_PROPERTY_ID`；
3. 按 `m_displayId` 匹配后，`SCREEN_PROPERTY_DISPLAY` 把窗口绑到对应屏。

**每个 EGLWindow 都能独立选择渲染到哪块屏**，这是双屏的核心前提。

### 2.2 命令流支持多 surface 切换 🟡（半就绪）

- 渲染线程命令已有 `Cmd_MakeCurrent` / `Cmd_Present`，且都带 surface 参数
  （`src/renderer/RenderDeviceProxy.cpp:308-324` 编码、`:930-939` 执行），
  一帧内多次切换 surface 在命令流上是通的。
- `GLRenderDevice::makeCurrent` 是「单 EGLContext 依次 current 到不同 surface」
  模式，且有防重复切换判断（`src/renderer/GLRenderDevice.cpp:42-61`）。
  这是 EGL 标准用法——shader / 纹理 / VBO 全部挂在 context 上，
  切 surface 不丢 GPU 资源。
- `eglSwapBuffers(eglDisplay, surface)` 不依赖当前 context，
  一帧内对两个 surface 各 present 一次没有障碍。

### 2.3 每窗口渲染状态已隔离 ✅

| 状态 | 说明 |
|------|------|
| BatchManager | 每个 `Window` 持有自己的实例（`src/platform/Window.cpp:11`），`EGLWindow::beginRenderPass` 中挂到 `frameState->batchManager` |
| viewport / clearColor / scissor | 都是命令流里的逐帧状态命令，按窗口顺序编码即可生效 |
| OrthographicCamera | 共享单实例，但每个窗口 `beginRenderPass` 都会 `camera->update(pos, size)` 重配置（`EGLWindow.cpp:197`），顺序执行两个窗口不冲突 |
| clipStack / framebufferWidth/Height / screenAlpha | 均在 `EGLWindow::beginRenderPass` 内逐窗口重置 |

### 2.4 QNX 平台侧无障碍

- 一个 `screen_ctx`（`SCREEN_APPLICATION_CONTEXT`，见 `EGLOperationsQNX::initScreen`）
  可创建任意多个 window，各自用 `SCREEN_PROPERTY_DISPLAY` 绑不同 display；
- 一个 EGLDisplay 上多 EGLSurface + 单 EGLContext 是 QNX 车机双屏
  （仪表 + 中控）的标准方案；
- 代码已经在使用上述 API，平台侧不需要新增任何能力。

---

## 三、缺失的部分（需要改造）

### 缺口 1：Platform / Engine 单窗口假设（主要工作量）

| 位置 | 现状 |
|------|------|
| `src/platform/Platform.h:67` | `m_window` 是单个 `WindowSharedPtr` |
| `src/platform/egl/QNXPlatform.cpp:26,47-62` | 构造只建 1 个 EGLWindow；`beginRenderPass / updateWidgets / commitRenderPass / endFrame` 全部直通这唯一窗口 |
| `src/core/Engine.h:21` | `EngineOptions` 只有一个 `windowInfo` |
| `src/core/Engine.cpp:93-107` | 主循环单轮 beginRenderPass → update → commit → endFrame |
| `src/core/Engine.h:39` | `getWindow()` 只返回唯一窗口，samples 全部挂它 |

### 缺口 2：每帧缺少 makeCurrent（隐藏坑）

`EGLWindow::beginRenderPass` **没有调用 makeCurrent**——目前只在窗口创建时
makeCurrent 一次（`EGLWindow.cpp:134`，`initSurface()`），单窗口场景下永远
current 所以没问题。双窗口必须在每个窗口的 `beginRenderPass` 里补一次：

```cpp
RENDERINGTHREAD->makeCurrent(m_eglSurface);
```

否则窗口 B 的所有绘制命令会画进窗口 A 的 surface（当前 context 仍绑着 A）。

### 缺口 3：输入不区分来源窗口

- `QNXInputProvider::poll`（`src/platform/egl/QNXInputProvider.cpp:63-126`）
  从单一 screen_ctx / screen_ev 拉事件，坐标只减一个固定的
  `m_windowX / m_windowY` 偏移，**没有查询 `SCREEN_PROPERTY_SOURCE_WINDOW`**；
- `Platform::resolveInputTargets`（`src/platform/Platform.cpp:78-79`）
  只在 `m_window` 一棵 Widget 树里做 hit-test。

双屏各有触摸时会串屏：A 屏的触摸会按 A 的坐标系在错误的窗口树里找目标。

---

## 四、实现路线对比

### 路线 A：单进程单引擎多窗口（推荐）

改造点清单：

1. `EngineOptions` 增加 `std::vector<WindowInfo> windowInfos`
   （保留 `windowInfo` 字段兼容旧用法，或作为主屏）；
2. `Platform::m_window` → `std::vector<WindowSharedPtr> m_windows`，
   `getWindow()` 返回第一个（旧代码不破）；
3. `QNXPlatform` 循环创建 EGLWindow（**共享同一个 `m_egl` 上下文**——
   不要建两个 EGLDisplay/EGLContext，否则 GPU 资源无法共享）；
4. `Engine::render()`：把 `beginRenderPass → updateWidgets → lateUpdateWidgets
   → commitRenderPass` 从一轮改成**对每个窗口各一轮**
   （窗口 A 完整管线 → 窗口 B 完整管线）；`endFrame()` 里对每个 surface 各
   present 一次；
5. `EGLWindow::beginRenderPass` 开头补 `makeCurrent`（缺口 2）；
6. 输入改造：poll 时查 `SCREEN_PROPERTY_SOURCE_WINDOW`，`TouchEvent` 增加
   来源窗口字段，`resolveInputTargets` 按来源选窗口树；
   `setWindowOffset` 改为 per-window；
7. `shouldClose()` / `terminate()` 遍历所有窗口。

**范围**：Platform / Engine 层约 200~400 行改动，
**不动 renderer、BatchManager、ECS**。

改造后主循环伪代码：

```
while (!shouldClose()):
  beginFrame()                        # poll 输入（含来源窗口）
  preRender.notify()
  if needRender:
    for window in windows:            # 每窗口一轮完整管线
      window.beginRenderPass(fs)      #   makeCurrent + camera 重配 + clear
      window.updateWidgets(fs)
      window.lateUpdateWidgets(fs)
      window.commitRenderPass(fs)     #   合批提交
    frameNumber++
    afterRender.notify()
    for window in windows:
      present(window.surface)         # 各自 eglSwapBuffers
    endFrame()
  fpsController.sleep()
```

### 路线 B：每屏一个 Engine / 进程

- **单进程两个 Engine 实例：不可行。** `GlobalObject`（RenderingThread /
  SSBOManager / FontManager / TextureManager）是全局单例，且
  `RenderingThread::run(platform)` 只能绑一个 platform，第二次 run 会直接覆盖
  （`src/renderer/RenderingThread.cpp:9-13`）。要先做单例体系多实例化，
  改造量反而比路线 A 大。
- **双进程（每屏一个进程）**：零代码改动即可跑（两个进程各传不同
  `displayId`），但代价：
  - 字体 / 纹理资源内存翻倍；
  - Widget 无法跨屏（两套独立 UI）；
  - 两进程各自与 Screen 合成器通信。
  适合「两块完全独立的屏」，不适合「一套 UI 分两屏」。

---

## 五、风险点提示

1. **CommandBuffer 16MB 固定上限**：双窗口两轮渲染命令进同一个帧 buffer
   （三缓冲 RingBuffer，每块 16MB，永不 reallocate）。UI 复杂时两屏叠加需要
   关注容量余量，必要时做单 buffer 容量评估或分窗口提交。
2. **双屏刷新率不一致**：串行 present 的帧耗时趋近 max(两屏 vsync 等待)；
   若一屏 60Hz 一屏 30Hz，`FPSController` 单一 targetFPS 也需要按屏细化。
3. **按需渲染门控**：`isNeedRender` 是全局标记（`enableRequestRender=true`
   时生效），双屏下建议细化为 per-window 脏标记，避免静止屏反复 present。
   功能上无害，但车机上功耗是硬指标。
4. **3D 场景共享状态**：`frameState->scene3DPassContext` / 透视相机是共享的，
   若两屏都要 GLTF 内容，需确认窗口轮次间 3D 上下文正确重置。
5. **suspend/resume 回归**：`QNXPlatform` 的 `m_skipEventFrameCount` 注释
   提到「多线程方案上下文移除的时候会异常」；双 surface 切换场景下
   休眠恢复路径值得专项回归。
6. **每窗口初始化重试**：`EGLWindow::initializeIfNeeded` 有 3 次重试上限，
   双窗口场景下某一屏未就绪（如 display 未上电）时，注意另一屏不应被阻塞。

---

## 六、建议下一步

1. 确认业务形态：一套 UI 拆两屏（路线 A）还是两块独立 UI（可先双进程验证）；
2. 若走路线 A，先补缺口 2（每帧 makeCurrent）做双 surface 最小验证
   （两窗口各画纯色，确认内容互不串屏）；
3. 再做 Platform 多窗口编排与输入分发改造；
4. 用 `samples` 下任一 Demo 扩展为双窗口冒烟用例，在 QNX 目标机上验证
   （关注 `TargetDisplayId=...` 日志，`EGLWindow::initDisplay` 已打印每块屏枚举信息）。

---

## 七、同内容双屏的优化方案：离屏渲染 + 上屏贴图

> 适用前提：两块屏显示的**内容完全一致**，仅窗口大小不同。
> 思路：渲染一次到离屏 RenderTarget（FBO → 纹理），每块屏各贴一次图
> （render-once, present-twice）。

### 7.1 为什么本引擎特别适合

| 有利条件 | 证据 |
|----------|------|
| FBO 基础设施现成且完整 | `createRenderTarget(w, h, colorTexture)` 已带深度附件（DEPTH_COMPONENT16 RBO，2D/3D 混合依赖）与 MSAA + 自动 resolve（`GLRenderDevice.cpp:852-935`；`unbindRenderTarget` 时 `glBlitFramebuffer` resolve 到纹理，`:960-979`）。毛玻璃 BackdropBlur 已在用 RT 链，有先例 |
| 单 EGLContext 红利 | 两窗口共享同一 context，纹理天然对两个 surface 可见，零跨上下文共享工作 |
| CPU 管线只跑一次 | Widget 遍历、Tween、BatchManager 合批全部只执行一遍 |
| 改造范围更小 | UI 管线不动（一棵树、一套坐标），只把渲染目标换成 FBO，新增薄层「上屏」阶段 |

### 7.2 性能账（1080p 量级）

**省下的**：第二遍完整场景渲染（draw call 提交开销、blend/overdraw 填充、3D 场景）。
UI 越复杂优势越大。

**付出的**：

- 显存：内容分辨率 RGBA 纹理（1080p ≈ 8.3MB）+ 深度 RBO；
- 带宽：每屏 blit 读 + 写 ≈ 33MB/帧额外流量，通常远小于省下的第二遍场景；
- 省不掉：两 surface 各自 `eglSwapBuffers` 的 vsync 等待，刷新率不同时帧节奏仍取较慢屏；
- ⚠️ 极简 UI（两三个全屏 quad、无 overdraw）可能持平甚至略亏，按实际场景评估。

**分辨率策略**：FBO 取较大屏的原生分辨率——大屏 1:1 锐利，小屏贴图时
`GL_LINEAR` 缩小（minify 质量无问题）。不要反向放大。

### 7.3 方案决策矩阵

| 场景 | 推荐方案 |
|------|----------|
| 内容一致 + 纵横比相同 | Screen 合成器镜像（近零成本）或 FBO+贴图（质量/行为可控） |
| 内容一致 + 纵横比不同 | **FBO + 每屏适配模式**（letterbox / 裁切 / 拉伸自定） |
| 内容不一致 | 路线 A（双窗口各渲染，见第四节） |

### 7.4 候选：Screen 合成器镜像

QNX Screen 支持 display 级镜像（`SCREEN_PROPERTY_MIRRORS` 或 graphics.conf 配置）：
应用只渲染主屏，合成器把整屏输出复制/缩放到副屏（可能走硬件管线，接近零成本）。
约束：

- 整屏**等比**镜像，纵横比不同会变形或加黑边；
- 无法按屏独立设置 zorder / alpha / sensitivity；
- 依赖 QNX SDP 版本支持（需在目标机 SDK 的 screen.h 确认宏存在）。

> 不建议「从窗口 back buffer 拷贝到另一窗口」（`glBlitFramebuffer` A→B）：
> back buffer 生命周期由驱动/EGL 管理，swap 后内容不保证可读，FBO 才是可靠路径。

### 7.5 引擎内落地要点

1. **窗口**：两个 EGLWindow（复用现有 `displayId` 绑定），仅主窗口跑 UI 管线，
   次窗口退化为 present-only；
2. **离屏渲染**：`createTexture2D` → `createRenderTarget(contentW, contentH, tex)`；
   主循环：主窗口 `beginRenderPass` → `bindRenderTarget` → update/lateUpdate →
   `commitRenderPass` → `unbindRenderTarget`（MSAA 自动 resolve 进纹理）；
3. **两个必须修正点**：
   - `EGLWindow::beginRenderPass` 的 `camera->update` 用窗口尺寸（`EGLWindow.cpp:197`），
     FBO 模式下要改用内容分辨率；
   - 窗口是 `SCREEN_PRE_MULTIPLIED_ALPHA`（`EGLWindow.cpp:70`），
     贴图 shader 需 `rgb *= a`，否则半透明 UI 边缘发白；
4. **新增薄层上屏阶段**（如 `PresentStage`）：对每个窗口
   `makeCurrent → clear → 全屏 quad 采样 RT 纹理 → present`，每屏 1 个 draw call；
5. **输入更简单**：hit-test 仍是一棵树一套 UI 坐标；
   窗口像素 → UI 坐标 = 贴图变换的逆映射（等比缩放 + 偏移）；
6. **按需渲染门控天然适配**：一次 `REQUESTRENDER` → 渲染一次 → 两屏同时更新，
   无需 per-window 脏标记。

### 7.6 时序说明

```
渲染线程每帧:  UI pass → FBO → blit A → swap A(等 A 的 vsync) → blit B → swap B
```

FBO 仅在渲染线程顺序使用，无数据竞争；两屏刷新率不一致时，
帧耗时仍由较慢屏主导（两方案相同，属 present 固有成本）。

---

## 八、实现记录（已落地）

第七章方案已实现为 **多屏渲染开关**：`Mirror`（默认，等比缩放 + 黑边）/ `Independent`
（各屏各自渲染）。单窗口与 Windows 平台行为完全不变。

### 8.1 使用方法

```cpp
EngineOptions engineOptions;
engineOptions.windowInfo.displayId = 3;            // 主屏（尺寸即镜像内容分辨率）
engineOptions.extraWindowInfos.push_back(secondary);// 副屏，displayId 须不同
engineOptions.screenMode = ScreenRenderMode::Mirror; // 默认值，可省略；改 Independent 则各自渲染
```

- Independent 模式给副屏挂内容：`engine->getWindows()[1]->addChild(...)`（widget 不能同时挂两个窗口）。

### 8.2 改动清单

| 文件 | 改动 |
|------|------|
| `src/renderer/PresentStage.h/.cpp` | **新增**：镜像上屏阶段。懒建 `OffscreenRenderTarget`（内容 RT）+ present 材质 + 裁剪空间 quad（照抄 BackdropBlurManager::buildPassQuad）；`beginContentPass` 绑 RT、`endContentPass` 解绑（MSAA 自动 resolve）、`presentAll` 逐窗口 makeCurrent → 清屏黑边 → letterbox 贴图（1 draw call）→ present |
| `assets/shaders/present.vert/.frag` | **新增**：全屏贴图 shader（`u_scale`/`u_offset` clip-space 变换，构建时自动嵌入动态库） |
| `src/platform/Window.h` | 新增 `ScreenRenderMode` 枚举；Window 基类新增 `isWindowInited()` / `getWindowSize()` 虚访问器 |
| `src/platform/Platform.h/.cpp` | 新增 `m_windows` 窗口集合（`m_window` 恒为第一个）、`setWindows()` / `setWindow()` / `getWindows()`；新增 `beginContentPass` / `endContentPass`（基类空实现）与 `isMirrorActive()`（基类 false） |
| `src/platform/PlatformFactory.h/.cpp` | `create(info, extraWindowInfos = {}, screenMode = Mirror)`；GLFW 分支忽略 extras 并 LOG_WARN |
| `src/platform/egl/QNXPlatform.h/.cpp` | 多窗口创建（共享同一 EGL 上下文）；持有 PresentStage；`endFrame` 按 mirror / 逐窗口 present 分流；`terminate` 先释放 PresentStage 资源（早于 GlobalObject::destroy） |
| `src/platform/egl/EGLWindow.cpp` | **关键修复**：`beginRenderPass` 开头补每帧 `makeCurrent`（多窗口必需，单窗口被去重短路）；新增 `isWindowInited` / `getWindowSize` |
| `src/core/Engine.h/.cpp` | `EngineOptions` 新增 `extraWindowInfos` / `screenMode`；`render()` 渲染段改为窗口循环 + mirror 分支（主窗口管线包在 RT 内）；新增 `getWindows()` |
| `samples/MultiScreenDemo.cpp` | **新增**：双屏演示（副屏 1920×720 vs 主屏 1920×1080，演示 letterbox），Mirror/Independent 可切换 |
| `docs/DUAL_DISPLAY_FEASIBILITY.md` | 本节 |

### 8.3 行为说明与边界

- **单窗口**（未配 extras）：`mirrorActive=false`，Engine 走窗口循环（等价原单窗口路径），零回归；
- **Mirror**：仅主窗口跑 UI 管线 → 内容 RT → 各屏贴图上屏；副屏分辨率不同时等比缩放 + 黑边居中（内容分辨率取主窗口尺寸，建议配较大屏）；
- **Independent**：每窗口各自 `beginRenderPass → update → lateUpdate → commit → present`；
- **v1 已知边界**：① Windows (GLFW) 平台忽略 extras（单窗口运行并打日志）；② 副屏触摸已路由（§九.3，`TouchEvent.windowIndex` + `SCREEN_PROPERTY_WINDOW` 反查）；③ present shader 原样拷贝 RT 内容（straight alpha），与引擎直写窗口的 alpha 行为一致；
- **关机顺序**：PresentStage 资源在 `QNXPlatform::terminate()`（render() 末尾）释放，早于 Engine 析构中的 `GlobalObject::destroy()`，与 BackdropBlurManager 的释放时序约定一致。

### 8.4 验证记录（2026-11-10）

- Windows mingw 构建：库 + 全部 samples 编译链接通过（samples 的 POST_BUILD assets 并发拷贝偶发竞争为既有问题，串行重试即成功）；
- Windows 运行回归：`TextDemo`（单窗口零回归）与 `MultiScreenDemo`（extras 忽略 + WARN、Mirror 单窗口不激活）运行正常、无错误日志；
- QNX 分支：用本机 QNX 8.0 aarch64 交叉编译器（`q++ -Vgcc_ntoaarch64le -c`，真实 screen.h/EGL 头）编译全部改动文件通过；
- **待 QNX 目标机验证**：双屏绑定（`TargetDisplayId` 日志）、Mirror 内容一致性与 letterbox、Independent 两屏内容独立、`enableRequestRender` 门控下两屏同步更新。

---

## 九、下游验证问题修复（2026-11-11）

第八节实现经下游项目（QianliRender）目标机验证发现三处问题，修复已回移植本仓库。
问题均只影响**多窗口**路径，单窗口行为不变。

### 9.1 VertexArray 同池 key 不同批次 → 顶点不重传（新旧内容交替闪烁）

`RenderBatchPool` 以 `shaderName|isSSBO` 为 key 池化 `RenderBatch`（含 `VertexArray`）。
同 key 的多个池对象会在**逻辑批次间轮换（LIFO）**：批次结构变化重建时（`buildBatches`
先 `releaseAll` 再逐组 `acquire`），上一帧批次 A 持有的 VertexArray 下一帧可能被
批次 B 持有。而 `VertexArray` 的网格上传缓存（mesh 指针 + revision 比对）会在以下
两类场景**误命中/残留**：

- 缓存记录指向上一任逻辑批次的网格列表，持有者切换后若列表恰好等价（数量一致），
  缓存判定通过 → 跳过上传 → 画出上一任批次的旧顶点数据；
- 缓存存的是 `weak_ptr<Mesh>`，Mesh 释放后堆地址被新 Mesh 复用且 revision 巧合
  相同时，指针比对失效。多屏 Independent 模式双窗口共享全局池、树内容各自独立
  变化，轮换与地址复用高频发生，表现为两屏新旧内容交替闪烁。

**修复**：`VertexArray` 新增 `invalidateMeshCache()`（清空 `m_uploadedMeshes`），
`RenderBatchPool::release / releaseAll` 归还池对象时调用。批次结构稳定的帧仍走
增量缓存路径（不 rebuild 就不归还），上传跳过优化保留；仅重建帧强制首帧重传。

### 9.2 Independent 多屏显示异常（先画的屏不上屏）

`eglSwapBuffers` 要求目标 surface 是**调用线程当前的 draw surface**，否则报
`EGL_BAD_MATCH` 且静默失败。Independent 模式渲染顺序 A→B，帧末 current 停留在
最后一屏 B；原 `QNXPlatform::endFrame` 逐窗口直接 `present` 不先 `makeCurrent`，
A 屏 swap 失败 → 只有最后一屏有画面/内容异常。

**修复**：`endFrame` Independent 分支逐窗口 `makeCurrent(surface)` 后再
`present(surface)`（与 `PresentStage::presentAll` 的顺序一致），并跳过
`isWindowInited() == false` 的窗口（某屏未上电/初始化失败不阻塞另一屏 present）。

### 9.3 多屏输入按来源窗口路由（TouchEvent.windowIndex）

原输入链路不区分来源窗口：QNX 触摸坐标只减固定的主窗口偏移，命中测试只查主窗口
树；共享相机在渲染 pass 中被各窗口反复改写，事件阶段按最后渲染的窗口换算坐标，
A 屏触摸会按错误坐标系在错误窗口树里找目标；两屏 touchID 各自从 0 编号，裸 ID
按压捕获表跨屏冲突。

**修复**（三段）：

| 位置 | 改动 |
|------|------|
| `TouchEvent` | 新增 `windowIndex` 字段（0 = 主窗口，非 QNX 平台恒为 0） |
| `QNXInputProvider` | 新增 `ScreenWindowSource`（句柄 + 偏移）反查表与 `setScreenWindows`；独立采集线程逐事件查 `SCREEN_PROPERTY_WINDOW` 反查序号、按各窗口位置换算局部坐标（表由主线程 initialize 写入，互斥锁保护、快照后查表）；合成 RELEASE 按 `(windowIndex, touchID)` 追踪，两屏独立 |
| `Platform::resolveInputTargets` | 按事件 `windowIndex` 选来源窗口：共享相机按来源窗口 pos/size 重配后再 `screenToWorld`；命中测试在来源窗口的 Scene2D 树进行；按压捕获表 key 改 `(windowIndex << 32) \| touchID` 复合键 |

配套：`Window` 基类新增 `getWindowPosition() / getWindowSize()` 虚访问器，
`EGLWindow` 实现（渲染 pass 与输入换算使用同一套窗口参数）并暴露
`getScreenWindow()` 句柄；`QNXPlatform::initialize` 在窗口初始化后注册句柄表。

单窗口 / Windows 平台回归评估：`windowIndex` 恒 0 → `sourceWindow == m_window`，
相机重配参数与渲染 pass 一致（幂等）；WGL 窗口 `getWindowSize()` 返回 {0,0} →
不触发相机重配，行为与改动前一致。

### 9.4 修复验证（2026-11-11）

- QNX 8.0 aarch64 交叉编译器（`q++ -Vgcc_ntoaarch64le -c`，真实 screen.h/EGL 头）
  编译全部改动文件通过：RenderBatchPool / Platform / QNXPlatform / EGLWindow /
  QNXInputProvider；
- 待目标机回归：Independent 双屏各自内容正确且两屏均持续上屏；双屏各自触摸
  命中各自控件树；Mirror 模式与单窗口零回归。
