# MorrowUI 高性能车载轻量化优化路线图

## 1. 文档定位

本文档针对以下目标场景，对引擎剩余优化空间做一次完整盘点：

- **目标产品形态**：高性能 3D GUI 车载轻量化引擎；
- **两个极致目标**：极致启动速度（冷启动到第一帧上屏）、极致运行速度（动效场景低帧耗时 + 静止场景零消耗）；
- **目标场景**：QNX 或核心系统的核心 UI / 动效；数据量不大，UI 节点 ≤ 300；
- **部署平台**：QNX 7.1/8.0（aarch64，OpenGL ES 3.x）为主，Windows/Linux 为开发宿主。

与 `ARCHITECTURE.md` 第 15 节的关系：该节是通用优化方向清单；本文档聚焦"启动速度 + 小规模场景运行速度"这两个此前未系统覆盖的维度，并按 ≤300 节点的场景约束**重新评估**了第 15 节部分条目的优先级（见第 11 节）。两份文档不重复展开同一事项。

分析基线：dev 分支 `6df5958`（2026-09-19）。文中所有代码位置均已逐一核实。

### 1.1 核心结论（TL;DR）

1. **最大的单项问题是构建配置**：当前所有非 MSVC 构建（含 QNX）无条件追加 `-g -O0`（`CMakeLists.txt:35-36`），无 Release 配置、无 LTO、无段裁剪。仅此一项就可能拖慢全部 CPU 代码 2~5 倍，并使二进制带完整调试符号。**修复成本最低、收益最大，应最先做。**
2. **启动路径上有四个同步大项**：8.4MB 默认字体同步读盘（`src/fonts/FontManager.cpp:10-16`）、48MB CommandBuffer 启动期预分配清零（`src/renderer/CommandBuffer.h:33` × 3 槽）、`checkSSBOSupport` 主线程↔渲染线程同步往返（`src/renderer/device/RenderDeviceProxy.cpp:535-543`）、首帧内逐个懒编译 shader 且无二进制缓存（`src/renderer/device/GLRenderDevice.cpp:586-668`）。
3. **动效运行时的最大持续成本不是 draw call**（SSBO 合批下 300 节点约 5~15 个 draw call，已接近最优），而是每帧 SSBO 填充与 uniform 提交中的**字符串键 map 查找**（每帧数千次）和渲染线程**无缓存的 `glGetUniformLocation`**。
4. **布局系统存在隐藏放大链**：布局容器每帧无条件重排 + `Transform::setPosition/setSize` 无值比较（`src/ui/base/Transform.cpp:17-41`）→ 子树矩阵全量失效 → 整批 VBO 重建重传。一行值检查即可切断。
5. **静止场景已达标**（request-render 门控下空闲成本为微秒级），但 **QNX 空闲等待仍是 60Hz 自旋 + 轮询**（`wakeEventLoop` 为空实现），静止功耗无法趋零。

---

## 2. 目标画像与度量基线

### 2.1 建议的度量维度

后续所有优化都应先建立基线、后验证收益（`ARCHITECTURE.md` 第 16 节的决策标准继续适用）。针对本文档的两个极致目标，建议度量：

| 维度 | 指标 | 采集方式建议 |
|---|---|---|
| 启动 | 各阶段耗时：EGL 初始化 / 字体读盘 / shader 编译 / 首帧 present | 启动期打点（现有 Log + 单调时钟），导出分解表 |
| 启动 | 冷启动 → 第一帧上屏总时间 | 目标设备实测（区分冷/热启动） |
| 运行 | 动效场景每帧 CPU 分解：tween / update / 收集 / 合批 / SSBO 填充 / 编码 | 15.3 节"性能时间线"的落地 |
| 运行 | draw call / batch 数、断批原因分布 | 已有 `BatchStatistics` + DebugPlane |
| 运行 | 静止场景 CPU 占用与唤醒频率 | 目标设备 `top`/pidin 采样 |
| 轻量化 | 常驻内存（重点：CommandBuffer 48MB、纹理、池水位） | ObjectRegistry 扩展（见 9.3） |
| 轻量化 | 二进制体积 / 链接的第三方库 | 构建产物对比 |

### 2.2 参考预算（建议目标值，待基线数据修正）

- 冷启动 → 首帧上屏：**< 300ms**（QNX 目标 SoC），其中引擎自身可控行为（不含进程加载）< 150ms；
- 动效满负荷（300 节点 + 持续动效）：主线程每帧 CPU **< 2ms**；
- 静止场景：CPU 占用 **< 0.5%**，唤醒频率由事件驱动；
- 常驻内存（引擎层，不含业务资源）：**< 64MB**（当前仅 CommandBuffer 就占 48MB）。

---

## 3. 现状盘点：已经做对的部分

以下能力已经落地且经代码核实有效，**后续优化不应破坏这些机制**：

| 能力 | 位置 | 效果 |
|---|---|---|
| 按需渲染闭环 | `src/core/Engine.cpp:155-161` | 静止时跳过动画/update/渲染/Present，空闲成本为事件等待 |
| 增量合批 | `src/core/BatchManager.cpp:132-138` | RenderItem 列表未变（纯位移动效）时整帧跳过批次重建 |
| 版本号缓存体系 | `Transform`（局部/世界矩阵版本）、`Mesh::m_revision`（VBO 上传）、`Material::m_batchCompatibilityRevision`（合批 key） | 各层缓存失效边界清晰，动效改 uniform 不打碎合批缓存 |
| SSBO 实例化合批 | `src/renderer/resource/ssbo/` | 同 shader+纹理+层+clip 的 N 节点 → 1 draw call、1 次 SSBO 上传 |
| CommandBuffer 零分配编码 | `src/renderer/CommandBuffer.h:62-89` | trivial payload 内联 push、非 trivial placement-new，无逐命令堆分配 |
| GPU 状态缓存 | `GLStateCache`（`GLRenderDevice` 内建） | 重复状态设置跳过 GL 调用 |
| GPU 资源异步销毁 + Fence 回收 | `Cmd_Delete*` + `glFenceSync` + `tryRecycle` | 多线程下生命周期安全，回收无阻塞等待 |
| 帧流水线 | 3 槽环形 + 信号量 | 主线程领先 2 帧才阻塞，等待为睡眠态 |
| OES 视频零拷贝 | `SafeStreamTexture` + EGLImage 复用 | 视频帧更新每帧仅一两条命令 |
| 字形懒光栅化 | `DynamicFont`（stb_truetype，非 FreeType） | 只解析用到的字形，字形缓存 map 命中即返回 |
| Shader 编译期内嵌 | `cmake/EmbedShaders.cmake` | 运行期无 shader 文件 IO |

结论：**架构骨架（短渲染路径、增量缓存、按需渲染）是对的**。剩余问题集中在四类：构建配置、启动路径的同步串行、热路径的字符串/哈希开销、平台层的等待/输入策略。

---

## 4. 优化项总览

优先级定义：P0 = 收益数量级大且实现代价小，立即推进；P1 = 对两个极致目标有直接 measurable 收益；P2 = 轻量化与平台专项；P3 = 长期方向（需求/数据驱动）。

状态截至 2026-09-20（dev 分支 `3960ddb` + R-3 增量）：M1 已合入；M2 曾实施后因 UI 位置错乱**整体回撤**，其中 R-3 已单独重新实施（见第 12 节 M2 状态说明），其余条目回到"未实施"。

| 编号 | 优化项 | 优先级 | 类别 | 预期收益 | 状态 |
|---|---|---|---|---|---|
| O-1 | Release 编译配置（-O2/LTO/gc-sections/strip） | **P0** | 构建 | 全部 CPU 路径 2~5×，体积大幅缩减 | ✅ M1 |
| O-2 | 诊断/调试设施量产默认关闭 | **P0** | 构建 | 启动分配与加锁开销归零，体积缩减 | ✅ M1 |
| O-3 | 第三方模块裁剪开关（GLTF/Basis 等） | **P0** | 构建 | 纯 2D 核心 UI 二进制显著缩小 | ✅ M1（仅 BASISU；GLTF 推迟） |
| S-1 | 默认字体加载优化（mmap/异步/裁剪） | **P0** | 启动 | 消除 8.4MB 同步读盘 | ✅ M1（mmap 未做） |
| S-2 | Shader 二进制缓存（glProgramBinary） | **P0** | 启动 | 二次启动省全部编译；变体裁剪降首次成本 | ✅ M1 |
| S-3 | CommandBuffer 容量可配置化 | **P0** | 启动/内存 | 48MB 常驻 → MB 级 | ✅ M1 |
| S-4 | 消除启动期同步往返（checkSSBOSupport） | P1 | 启动 | 启动关键路径去串行化 | ✅ M1 |
| S-5 | 字形 atlas 初始尺寸与扩容策略 | P1 | 启动 | 首帧 CJK 场景尖峰消除 | ✅ M1 |
| S-6 | 启动任务并行化时间线 | P1 | 启动 | EGL/字体/shader 并行 | 未实施 |
| R-1 | SSBO 逐实例填充去字符串查找 | **P1（运行时最大单点）** | 运行时 | 每帧数千次 map+string → 0 | 未实施（曾实施已回撤） |
| R-2 | uniform location 缓存（渲染线程） | P1 | 运行时 | 消灭每帧驱动侧字符串查询 | 未实施（曾实施已回撤） |
| R-3 | Transform setter 值相等检查 | **P1（一行改动，切断放大链）** | 运行时 | 布局场景整批 VBO 重传归零 | ✅ 2026-09-20 单独实施 |
| R-4 | 布局容器脏标记 | P1 | 运行时 | 静止布局零重排 | 未实施（曾实施已回撤） |
| R-5 | getComponent / 父 Transform 指针缓存 | P1 | 运行时 | 每帧数千次哈希查找+原子操作归零 | 未实施（曾实施已回撤） |
| R-6 | 热路径杂项清理（requestRender string 等） | P1 | 运行时 | 消除每帧堆分配 | 未实施（曾实施已回撤） |
| R-7 | eglSwapInterval 显式化 + FPSController 接线 | P1 | 运行时 | 帧节奏可控、防主线程空转 | 未实施 |
| T-1 | 字形上传脏矩形化 | P1 | 文本 | 新字帧 4MB 上传 → KB 级 | 未实施（曾实施已回撤） |
| T-2 | 量产预烘焙 SDF atlas | P2 | 文本 | 运行时零光栅化 | 未实施 |
| M-1 | 节点创建分配预算收敛（池化/共享 quad/UUID） | P2 | 内存 | 45-60 次分配/节点 → 个位数 | 未实施 |
| M-2 | Material 参数容器槽位化 | P2 | 内存/运行时 | 6 个 string-map → 紧凑结构 | 未实施 |
| M-3 | RecyclePool 上限与水位统计 | P2 | 内存 | 峰值后内存可归还 | 未实施 |
| M-4 | KTX2 加载峰值与转码线程化 | P2 | 内存/启动 | 2× 文件峰值消除，主线程尖峰移除 | 未实施 |
| M-5 | 2D 场景 depth buffer 去除 | P2 | 运行时 | 每帧 clear 带宽减半 | 未实施 |
| C-1 | Basis/KTX2 数据所有权修复 | **P1（正确性）** | 正确性 | 消除 double-free 风险 | ✅ M1 |
| C-2 | Tween 完成判定修复 | P1（正确性） | 正确性 | from==to 的动画不再首帧丢失 | ✅ 2026-09-20 单独实施（含 setLoop 架构级循环） |
| C-3 | CommandBuffer 溢出安全路径 | P2（正确性） | 正确性 | NDEBUG 下不再越界写穿 | ✅ M1（随 S-3 落地） |
| Q-1 | QNX 空闲等待事件驱动化 | **P1（静止功耗关键）** | 平台 | 静止 CPU 趋零，唤醒延迟 16.7ms → µs 级 | 未实施 |
| Q-2 | QNX 输入：阻塞策略 + keyboard/rotary 通道 | P1 | 平台 | 每帧 ~1ms 轮询消除；仪表硬键可用 | 未实施 |
| Q-3 | 热路径 dynamic_cast 清理 | P2 | 平台/运行时 | present/makeCurrent 每帧 RTTI 消除 | 未实施 |
| L-1 | Widget 树线性化 / RenderItem SoA | P3 | 架构 | ≤300 节点下收益有限，profiling 驱动 | 未实施 |

---

## 5. P0：构建与发布配置

### 5.1 Release 编译配置（O-1）——收益最大、代价最低

**现状**：`CMakeLists.txt:35-36` 对所有非 MSVC 构建（含 QNX）无条件追加：

```cmake
string(APPEND CMAKE_C_FLAGS " -fPIC -fpermissive -g -O0 -Wno-error")
string(APPEND CMAKE_CXX_FLAGS " -fPIC -frtti -std=gnu++17 -D_GLIBCXX_USE_C99=1 -fpermissive")
```

全仓库无 `-O2/-O3/-Os`、无 LTO、无 `-ffunction-sections -Wl,--gc-sections`、无 strip 配置、无 `CMAKE_BUILD_TYPE` 分支。即**当前 QNX 产物默认是 -O0 + 完整调试符号 + RTTI 强制开启**的库——这同时拖累运行速度（Widget 遍历/合批/矩阵路径全部未优化）和二进制体积。

**优化方向**：

1. 增加 `CMAKE_BUILD_TYPE` 分支，Release 配置（QNX qcc 均支持）：
   - `-O2`（或 `-Os`，体积敏感时）；aarch64 可评估 `-mcpu=` 目标 SoC；
   - `-fvisibility=hidden`；
   - `-ffunction-sections -fdata-sections` + 链接 `-Wl,--gc-sections`；
   - `-flto`（静态库 + LTO 可内联 Proxy→Device 调用链；注意与 `-fpermissive`/异常的兼容验证）；
   - 安装期 strip 或分离符号包。
2. Debug 保留现状（`-g -O0` 是合理的开发配置），关键是**不能让 -O0 泄漏到Release/交付配置**。
3. `-fno-rtti` 作为后续项：需先清理 `dynamic_pointer_cast`（`Interaction.cpp:23`）与 `type_index`（`ComponentManager.h:25`）两类硬依赖，与 Q-3 联动。

**验收**：同一 benchmark 场景（如 GearsDemo 持续动效）Release vs 当前配置的每帧 CPU 对比 + 二进制体积对比。

### 5.2 诊断与调试设施量产默认值（O-2）

**现状**：`MORROW_ENABLE_OBJECT_DIAGNOSTICS` 与 `MORROW_ENABLE_DEBUG_OVERLAY` 默认 ON（`src/CMakeLists.txt:6-7`）。前者使**每个 Widget/Component/Material 构造都走 `ObjectRegistry::registerObject`（make_shared + 字符串拷贝 + 互斥锁插入）**（`src/debug/ObjectRegistry.cpp:155-174`）；后者默认编译 DebugPlane 并在 Engine 构造期创建 2 个 MRLabel（`src/core/Engine.cpp:71-74`），即使不可见也参与首帧布局。

**优化方向**：Release/量产 preset 默认 OFF（保留 Debug 构建默认 ON）；DebugPlane 延迟到首帧 present 之后懒创建。≤300 节点时诊断注册本身量级不大（~1ms），但它是**每一类对象构造的固定税**，且默认值面向量产不合理。

### 5.3 第三方模块裁剪开关（O-3）

**现状**：`src/CMakeLists.txt:38-48,175-186` 将 core/loader/extern/ui/renderer/platform/fonts/debug/gltf **全部**编入单一静态库 `morrow`，无任何裁剪开关。其中：

- `basis_universal` transcoder 单文件约 1.75 万行（UASTC/ETC1S 解码表），无条件必选链接；
- `tinygltf` + `nlohmann/json.hpp`（约 2.46 万行头文件模板实例化），纯 2D 核心 UI 场景不需要；
- EmbeddedShaders 将全部 26 对 shader 源码编入 .rodata（对启动友好，保留）。

**优化方向**：增加 `MORROW_ENABLE_GLTF` / `MORROW_ENABLE_BASISU`（若资产侧全部预转码为 ETC2 直传，运行时 transcode 可整体裁掉）/ `MORROW_ENABLE_SCENE3D` CMake option，拆分为可选 target。核心 UI（300 节点 2D + 动效）的最小集合应不含 GLTF 栈。

**验收**：最小配置 vs 全量配置的 .text/.rodata 体积对比。

---

## 6. P0：启动路径

### 6.1 当前启动关键路径（已核实）

```
main()
 └─ Engine 构造（全部同步阻塞）
     ├─ eglInitialize + eglCreateContext（驱动加载，几十~几百 ms，不可避免但可并行）
     ├─ 渲染线程 spawn + RenderDeviceProxy 构造
     │    └─ CommandBuffer 16MB × 3 = 48MB 预分配并清零（CommandBuffer.h:33）
     ├─ screen window/surface 创建
     ├─ checkSSBOSupport：入队 + submit + future.get() 强制同步往返
     │    （RenderDeviceProxy.cpp:535-543 —— 启动期第一次主线程等待渲染线程）
     ├─ FontManager::initialize：同步 fopen+fread 完整 8,768,944 字节默认字体
     │    （FontManager.cpp:10-16；字形虽懒解析，但文件全量进内存）
     └─ DebugPlane 创建 + 2 个 MRLabel（默认配置）
 └─ Engine::render() 首帧
     ├─ [主线程] Widget update：MRLabel 文本布局 → 逐字形 SDF 生成
     │    （DynamicFont.cpp:141-223；CJK 首屏字符多时可触发 atlas 翻倍重建）
     ├─ [主线程] buildBatches → Material::apply → 首次懒编译 shader（逐 program 10-50ms）
     │    + 纹理同步 decode/transcode（KTX2 2048² 可达数十 ms）
     └─ endFrame 提交 → [渲染线程] 4MB 空 atlas 全量上传 → N 个 program 编译链接
        → 首 draw → eglSwapBuffers ← 第一帧
```

### 6.2 默认字体加载优化（S-1）

**现状**：`FontManager::initialize()`（`FontManager.cpp:10-16`）在 Engine 构造期同步读入 8.4MB OTF；每个 `DynamicFont` 构造即创建 1024×1024 atlas（4MB CPU buffer 分配清零 + 整张空图上传 GPU，`FontTexture.cpp:12-29`）；`m_fontFamilies` 按 name 去重而不按 path——业务再用别名 add 同一文件会双份驻留、双 atlas。

**优化方向**：

1. 文件加载改 **mmap**（`LoadFromMemory` 入口已存在，`DynamicFont.cpp:37-41`），8.4MB 读盘 → 页映射，且物理内存按需驻留；
2. `addFonts` 增加按 path 去重；默认字体与应用字体合一，避免双份；
3. 进一步（结合 T-2）：核心 UI 的固定文案可离线预烘焙 SDF atlas，运行时连 stb 解析都省掉，默认字体文件按需加载甚至不加载；
4. 起步阶段低成本方案：字体读盘移到工作线程，与 EGL 初始化并行（字形只在首帧文本布局时才被访问，窗口期足够）。

### 6.3 Shader 编译策略（S-2）

**现状**：

- 懒编译：每个 Material 首次 apply 时编译（`Material.cpp:324-327,386-389` → `GLRenderDevice.cpp:586-668`），SSBO 路径还会再编译一个 `ENABLE_SSBO` 变体；
- 源码内嵌（好），但 **`glProgramBinary` 磁盘缓存代码整体被注释**（`GLRenderDevice.cpp:592-661`），每次冷启动全量 GLSL 编译；
- 无预热 API，编译分散在首帧渲染过程中，直接推迟第一帧 present。

**优化方向**：

1. **恢复并产品化 glProgramBinary 缓存**：骨架已在（保存/加载逻辑被注释），补 key = 驱动版本 + shader 源码 hash + 变体宏，落盘到应用可写分区。二次冷启动省掉全部编译（车载场景进程常被杀重启，收益直接）；
2. **变体裁剪**：QNX 目标 GPU 明确支持 ES 3.1+ SSBO 时，配置固定只编译 SSBO 变体（当前双变体编译量翻倍）；
3. 提供 `Engine::preloadShaders(names)`：首帧 present **之后**在渲染线程空闲时段预热非首屏 shader，避免它们在运行中首次出现时卡顿；
4. 首帧最小集合：只编译首屏实际用到的 1~3 个 program（纯色 + 字体 + 首屏图），其余懒编译。

### 6.4 CommandBuffer 容量可配置化（S-3）

**现状**：`kDefaultCapacity = 16MB`（`CommandBuffer.h:33`），3 槽环形 = **48MB 常驻**，构造期 vector 值初始化（清零）——启动期一笔可观 memset + 首触 page fault，且对内存受限目标占比过高。注释自述 16MB 是为启动帧资源创建 + 纹理上传堆积预留（`CommandBuffer.h:30-33`），稳态帧实际用量远小于此。

**优化方向**：

1. 容量经 `EngineOptions` 可配置，量产默认 1~2MB 起步；
2. 溢出策略配套（与 C-3 联动）：固定容量 + 断言在 Release 下不安全，应支持"写满时同步 flush 当前端再续写"或至少明确报错；
3. 大 payload（纹理像素）可评估走独立旁路池而非命令流，使命令流容量可以开得很小；
4. ≤300 节点单线程模式可进一步降为 2 槽双缓冲。

### 6.5 消除启动期同步往返（S-4）

**现状**：`checkSSBOSupport()` 通过 `submitCurrentBufferAndAdvance()` + `result.get()` 强制主线程等待渲染线程完成 makeCurrent 和字符串查询（`RenderDeviceProxy.cpp:535-543`）。这是启动关键路径上的串行点：渲染线程此刻才真正执行 makeCurrent + `glGetString(GL_VERSION)`（`GLRenderDevice.cpp:82-95,160-183`）。

**优化方向**：车型/SoC 固定时，SSBO 支持由 `EngineOptions` 预置（`forceSSBO=true`），启动期跳过查询；或将其并入首帧命令流异步完成（首帧前 batch 决策已知即可）。同理 WGL 侧 `makeCurrent` 每次 `gladLoadGLLoader` 重扫函数指针（`GLRenderDevice.cpp:37-41`，仅影响 Windows 开发宿主）应只做一次。

### 6.6 字形 Atlas 初始尺寸与扩容（S-5）

**现状**：atlas 固定 1024² 起步，满时翻倍到 2048² 并**重建全部已缓存字形**（`DynamicFont.cpp:225-269`），且扩容会使共享该字体的所有 Label 下帧重排版。中文首屏字符多时，首帧即触发扩容 → 首帧卡顿的主要来源之一。

**优化方向**：初始尺寸按预估首屏字数选择（核心 UI 场景文案集合封闭，可直接起步 2048²）；扩容改为新增页而非全量重建（与 15.18 的 Atlas 分页方向一致，但在"文案封闭"的核心 UI 场景下，**正确的初始尺寸**这一步收益就已足够）。

### 6.7 启动任务并行化时间线（S-6）

目标形态：

```
T0   主线程：EGL/screen 初始化 ──────┐
     工作线程A：字体文件 mmap/预读 ──┤ 并行
     工作线程B：首屏纹理预解码/KTX2 转码 ──┘
T1   首帧提交：首屏 shader（1-3 个）+ 已预热纹理
T2   present 首帧后：渲染线程空闲段预热其余 shader / 二进制缓存写盘
```

配套：DebugPlane 延迟创建（6.2/5.2）；`checkSSBOSupport` 预置（6.5）。

---

## 7. P1：动效运行时热路径

场景假设：300 节点 + 持续动效（转场/呼吸灯/指针），每帧都有变化。当前每帧 CPU 估算构成（-O0 下）：两趟全树遍历 0.1-0.3ms、渲染收集 0.05-0.15ms、SSBO 填充 + 矩阵 0.05-0.2ms、布局（若有容器）最高 0.5ms+（含隐藏 VBO 重传）、元素固定成本 0.02-0.1ms/类。Release 编译（O-1）预计整体降到 1/3~1/5，之后按下列条目逐项收敛。

### 7.1 SSBO 逐实例填充去字符串查找（R-1）——运行时最大单点

**现状**：SSBO 路径每批每帧全量重填实例数据（`SSBOManager.cpp:14-48`），每实例走 `fillSSBOInstance`（`SSBOFieldBinding.cpp:82-91`），其中 `materialProperty` 字段按**字符串**查 Material 的 map：

- `getVector4Or(field.materialProperty, ...)`（`SSBOFieldBinding.cpp:64`）；
- PackedVector4 分支 4 次 `getFloatOr/tryGetVectorComponent`（`SSBOFieldBinding.cpp:16-33,68-75`）；
- 每次查询都构造临时 `std::string(m_attributePrefix + name)` 再 `unordered_map::find`（`Material.cpp:111-165`）。

UIInstanceData 每实例 6 字段（`SSBOLayoutBuilder.h:32-50`），300 实例 ≈ **每帧 3000~5000 次 map 查找 + 同量级临时字符串构造**。`ARCHITECTURE.md` 15.19 已将其列为 P2 候选并给出方向，本文档基于热路径分析将其**升级为 P1**。

**优化方向**（15.19 已有方案，此处确认优先级）：

1. layout 构建期把 `materialProperty` 解析为 Material 内预分配槽位索引（或稳定字段句柄），`writeSSBOField` 直接按索引读连续数组——每帧字符串/map 操作归零；
2. Material revision 未变化时复用已解析的 per-instance 常量（只有 worldMatrix 等真正逐帧变化的数据直接写入）；
3. 评估批次级 SSBO 容量复用，避免每批每帧 resize DTO。

**验收**：相同 RenderItem 数量下的 SSBO 填充耗时（15.3 节性能时间线落地后可测）。

### 7.2 uniform location 缓存（R-2）

**现状**：`GlProgram::getUniformLocation` 是裸 `glGetUniformLocation`，无任何缓存（`src/renderer/device/GlResourceObjects.h:28-30`），调用点遍布 `GLRenderDevice.cpp:797-871`；`bindUBO` 每批每帧做 `glGetUniformBlockIndex(programID, "Global")`（`GLRenderDevice.cpp:900-912`）。标准路径下 300 节点 × ~8 uniform ≈ **每帧 2400 次驱动侧字符串查询**；SSBO 路径每批也有 2-4 次。

**优化方向**：

1. `GlProgram` 内建 `name → GLint` 缓存（或 program 创建时一次性反射全部 active uniforms，按小整数 id 编码进命令）；
2. uniform 命令 payload 从 `std::string uniformName` 改为 (program, location/int-id, value) trivial 三元组——同时消灭编码端每条命令的 string 拷贝（`RenderDeviceProxy.cpp:619-730` 的 8 种 `SetGPUProgramParam*Payload`）；
3. BindUBO 的 "Global" 按 program 缓存绑定结果，未变化跳过。

### 7.3 Transform setter 值相等检查（R-3）——一行改动切断放大链

**现状**：`Transform::setPosition/setSize` 无值比较，直接 `markDirty()`/`setDirty()`（`Transform.cpp:17-41`）。后果链：

1. 布局容器（HBox/VBox/Margin/Center）**每帧无条件 layoutChildren**（如 `MarginContainer.cpp:57-60`），对子节点每帧 `setPosition+setSize`（`MarginContainer.cpp:78-83`）；
2. `setSize` 触发 `notifySizeChange` → MeshFilter 监听器**每次新建 `std::vector<Vector3>` 并 setVertices**（`MeshFilter.cpp:16-28`）→ `++m_revision`；
3. mesh revision 变化 → 该批 `needsMeshUpload` 失败 → **整个合并 VBO（批内全部 mesh）每帧全量重建重传**（`VertexArray.cpp:66-222`）。

这是布局场景里单项最重的隐藏成本（可达 0.5ms+/帧含 GL 驱动）。

**优化方向**：

1. `setPosition/setSize/setRotation/setScale` 前置值比较（值未变直接 return）——**单点改动，全链收益**；
2. MeshFilter 尺寸监听先比较新旧尺寸再重建几何；
3. 与 R-4 配合后，"静止布局"不再产生任何矩阵失效与 VBO 重传。

### 7.4 布局容器脏标记（R-4）

**现状**：四个布局容器全部在 `update()` 里每帧无条件重排（`HBoxContainer.cpp:26-29`、`VBoxContainer.cpp:26-29`、`MarginContainer.cpp:57-60`、`CenterContainer.cpp:16-19`）；HBox 对每个 child 每帧做 `dynamic_pointer_cast<MRSpacer>`（RTTI）+ `getComponent<Transform>()`（哈希查找）。正面对照：`MRSplitContainer` 已用 `addSizeChangeListener` 事件驱动重排，方向正确。

**优化方向**：布局容器记录"子节点集合 / 自身尺寸 / spacing / margin"的版本，任一未变则跳过重排；`dynamic_pointer_cast` 换成缓存过的类型标记。R-3 落地后即使重排，代价也大幅下降。

### 7.5 getComponent 与父 Transform 指针缓存（R-5）

**现状**：热路径上的组件获取全部经过 `ComponentManager::getComponent` = `unordered_map<type_index, ...>` 查找 + shared_ptr 拷贝（原子 refcount）（`ComponentManager.h:36-45`）：

- `MeshRenderer::update` 每帧每节点 2 次；
- `Transform::getWorldMatrix` 每次查询 `getComponentInParent<Transform>()` 1 次 + `updateMatrix` 再 1 次（`Transform.cpp:118-154,184`）——深度 d 的节点每帧 O(d) 次哈希查找；
- 全树 300 节点 ≈ 每帧数千次哈希查找 + 数千次原子 refcount。

**优化方向**：

1. `Transform` 直接缓存父 `Transform*` 裸指针（挂接/重挂接时更新；`m_parent` 成员已存在但未被世界矩阵路径使用）；
2. `MeshRenderer` 等组件在 awake 时缓存 `MeshFilter*`/`Transform*` 裸指针（组件 1:1 附着、同生命周期，树存活期安全）；
3. `addRenderable` 参数改 `const shared_ptr&` 或裸指针（收集期对象存活由树保证），消除每帧 900 次原子 inc/dec（`BatchManager.h:36-40`）；
4. `Widget::update/lateUpdate` 的 `FrameStateSharedPtr` 改 const 引用传递（`Widget.h:50-53`），消除每帧 600 次原子 refcount。

### 7.6 热路径杂项清理（R-6）

已核实的每帧固定成本，单项小、数量多，适合一次清扫：

| 项 | 位置 | 处理 |
|---|---|---|
| `Widget::requestRender(std::string caller)` 按值收 string 且**从不使用** | `Widget.h:66`、`Widget.cpp:144-146` | 删除参数（调用点 ~20 处）；`"cpu particles update"` 等 20 字符实参超 SSO 每次堆分配 |
| `MRButton::update` 每帧无条件 `applyCurrentColors()` → setVector 字符串 map 写 | `MRButton.cpp:212,330-332` | 颜色/状态脏位化 |
| `Shadow::update` 每帧 3 次 getComponent + 4-5 次 setFloat/setVector，无脏检查 | `Shadow.cpp:27-60` | 参数脏位化 |
| `MRCPUParticles2D::update` 每帧 4 个新 vector + rebuildMesh + revision++ | `MRParticles2D.cpp:94-160` | 成员 vector 复用；非发射且无活跃粒子时跳过；粒子效果优先走 shader `timeDelta` 模式（MRFlowingLight 已是正面样例） |
| 标准路径 `{batch.meshFilters[index]}` 每对象每帧临时 vector 堆分配 | `BatchManager.cpp:229` | 增加单元素重载 |
| `Observable::notify` 每次拷贝 slots vector 快照 | `Observable.h:149-160` | 迭代中标记删除 + 版本号方案；帧事件等高频 notify 收益直接 |
| `TweenManager` 完成判定 `getCurrentValue() == getTargetValue()` 浮点相等 | `Tween.cpp:135` | 改 `m_state`/时长到达判定（兼为正确性修复，见 C-2） |
| present/makeCurrent 热路径 `dynamic_cast<QNXPlatform*>` | `GLRenderDevice.cpp:43,68` | 缓存指针（兼为 Q-3） |

### 7.7 帧率控制与 Present 语义（R-7）

**现状**：全仓库无 `eglSwapInterval` 调用（依赖 EGL 默认 interval=1）；`FPSController` 构造为 30fps 但渲染循环**从未调用**其节流方法（`Engine.cpp:66` vs `Engine.cpp:137-198`），`setFPS` 实际无效；多线程模式下主线程可超前渲染线程 2 帧，输入到显示最大附加 2 帧延迟。

**优化方向**：

1. 显式 `eglSwapInterval(dpy, 1)` 固化 FIFO 语义（防平台默认差异）；
2. 需要低延迟动效（指针/转速表）时支持 interval=0 + 主循环 pacing——现成 `FPSController` 接上即可（`Engine::render` 帧尾一行调用）；
3. 评估 3 槽 → 2 槽以钳制主线程超频渲染（省电 + 降延迟上限）。

---

## 8. P1：文本与字体运行时

### 8.1 字形上传脏矩形化（T-1）

**现状**：`FontTexture::FlushToGPU` 一次上传**整张 atlas**（`updateSubTexture2D(..., 0, 0, getWidth(), getHeight(), ...)`，`FontTexture.cpp:47-51`），即使只新增了一个 32×32 字形：1024² atlas = 4MB memcpy（代理侧再拷一次进池 buffer）+ 4MB glTexSubImage2D。新字符首现帧有 4MB 级尖峰。

**优化方向**：记录脏矩形（或脏字形列表合并 region），`updateSubTexture2D` 本就支持 x/y/w/h 局部更新（`RenderDeviceProxy.cpp:462-480`）。改动局部、收益直接。

### 8.2 量产预烘焙 SDF atlas（T-2）

核心 UI 场景文案集合基本封闭，量产路径可离线预生成 SDF atlas + 字形表（项目内 `SafeStaticTextLayout` 已有自建字符图集先例），运行时零光栅化、零 8.4MB 字体驻留、首帧零 SDF 生成。与 S-1 联动后，文本子系统启动成本趋近于零。动态文案（如蓝牙设备名）保留现有运行时路径作为回退。

---

## 9. P2：内存与轻量化

### 9.1 节点创建分配预算收敛（M-1）

**现状**：创建一个典型节点（MRButton = Widget+Transform+MeshFilter+MeshRenderer+Material + 内嵌 MRLabel）约 **45-60 次堆分配**，全部独立分配、零池化：

- 每个 Widget 强制 `generate_uuid()`（stringstream 级联，10-20 次小分配，`MathUtils.cpp:14-34`）；
- `Mesh::createQuad` 每节点 5 个局部 vector + 5 次成员拷贝（`Mesh.cpp:131-176`）——**未变形 UI 的 quad 几何全库同构**；
- 工厂统一 `shared_ptr<T>(new T)`（对象与控制块分离双分配，如 `Material.cpp:17-19`）；
- 每个 Material 持有 shader 源码字符串副本（`Material.cpp:552-589`），同名 shader 的 Material 各持一份。

300 节点 ≈ 启动期 1.5 万次分配。

**优化方向**：

1. **共享静态 quad mesh**：所有未变形 2D UI 共用一份不可变 quad（省每节点 11 次分配 + 300 份重复几何）；
2. UUID 改 uint64 单调 ID 或按需生成（调试用途由诊断层接管）；
3. 工厂统一 `make_shared`；
4. Widget/Component 池化（UI 节点类型集合封闭，free-list 够用）作为后续项，前三项落地后分配次数已降一个数量级。

### 9.2 Material 参数容器槽位化（M-2）

**现状**：Material 内 **6 个 `unordered_map<std::string, ...>`**（`Material.h:164-186`），每 map 通常 ≤10 项，桶数组 + 节点开销（每节点 ~56B + string key 32B+）远超数据本身；所有 uniform/纹理按 `"u_" + name` 字符串键访问。UI shader 的 uniform 集合是封闭的。

**优化方向**：内建 shader 的 uniform 走编译期枚举槽位（StringId/小整数），Material 侧紧凑数组存储；自定义 shader 保留 map 路径兼容。与 R-1/R-2 是同一套"字符串 → 槽位"改造的三个层面，宜作为一个 epic 统一设计。

### 9.3 RecyclePool 上限与水位统计（M-3）

**现状**：4 类回收池（VBO/UBO/SSBO/像素）均为 `queue + mutex`，**无容量上限、无水位统计、无内存归还**（`RecyclePool.h:40-41`）——池大小 = 历史 in-flight 高水位，某帧上传大纹理/视频帧后对应 capacity 永久驻留；像素池每个 vector 保留其经历过的最大纹理尺寸。

**优化方向**：maxIdle 上限 + `shrink_to_fit` 归还策略；高水位统计暴露给 ObjectRegistry/DebugPlane（与 15.3 性能时间线的 upload bytes 指标合并做）。

### 9.4 KTX2 加载峰值与转码线程化（M-4）

**现状**：整个文件读入 `m_file` 再转码输出到 `result`（**2× 文件大小瞬时内存**，`Ktx2TextureLoader.cpp:18-63`）；转码在**首次 draw 的主线程**同步执行（`Texture.cpp:197-216`）；每次加载 `make_shared<Ktx2TextureLoader>` 且构造函数重复跑 `basisu_transcoder_init()`（`Ktx2TextureLoader.cpp:11-14`）；多线程模式上传前还有一次全尺寸 memcpy 进池 buffer（`RenderDeviceProxy.cpp:451-459`）。

**优化方向**：加载/转码移到工作线程（`m_onLoaded` 事件已存在可承接替换）；转码器单例化；资产侧预转码为 ETC2（QNX GPU 原生格式）后 transcode 退化为纯 memcpy，可与 5.3 的裁剪开关联动把 transcoder 整体裁掉。

### 9.5 2D 场景 depth buffer 去除（M-5）

**现状**：EGL config 强制 `EGL_DEPTH_SIZE 8`（`EGLOperationsQNX.cpp:63`），每帧 `glClear(COLOR|DEPTH)`（`EGLWindow.cpp:211`）。纯 2D UI（painter's order）不需要深度。

**优化方向**：无 3D 内容的窗口 config 请求 `EGL_DEPTH_SIZE 0`，clear 只清 COLOR——每帧 clear 带宽减半，config 匹配也更宽松。含 `MR3DSceneView` 的窗口保留现状（其离屏 FBO 自理深度）。

### 9.6 正确性修复项

| 编号 | 问题 | 位置 | 说明 |
|---|---|---|---|
| C-1 | **Basis/KTX2 数据所有权 double-free 风险**：`textureDataSharedPtr` 用 `default_delete<unsigned char[]>` 包装 `basisData.data()`，而 `basisData`（vector）仍持有该内存，双方析构时各释放一次；`deployTexture` 中 `reset()` 即触发对非 `new[]` 内存 `delete[]` | `Texture.cpp:289,304`（包装）、`Texture.cpp:256`（reset） | 改为别名 shared_ptr（`shared_ptr(ptr, [](auto*){})` 持有 owner）或拷贝进池 buffer；**建议尽快修**，当前能跑通属于分配器行为碰巧兼容 |
| C-2 | Tween 完成判定用浮点相等：`from == to` 的 tween 首帧 update 后即被删除，duration 内的每帧回调丢失 | `Tween.cpp:135` | 改为时长/状态判定 |
| C-3 | CommandBuffer 溢出保护仅 `assert`：NDEBUG 下越界直接写穿，无 clamp/报错/丢帧路径 | `CommandBuffer.h:114-124` | 与 S-3（容量可配置）联动补安全路径 + 峰值统计 |

---

## 10. P2：QNX 平台专项

### 10.1 空闲等待事件驱动化（Q-1）——静止功耗的关键

**现状**：`Platform::waitForEvents` 基类实现是 `std::this_thread::sleep_for(timeout)`，`wakeEventLoop` 是**空函数**（`Platform.cpp:28-35`），`QNXPlatform` 两者均未 override（Windows 侧已有 `glfwWaitEventsTimeout + glfwPostEmptyEvent` 的正确实现，`WGLPlatform.cpp:37-43`）。后果：

1. QNX 空闲态 = 60Hz 自旋：每 16.7ms 醒一次 → updateFrameState → 输入 poll（内含 ~1ms 阻塞）→ gate 判断 → 再睡，**静止仪表仍持续烧 CPU**（估算单核 1-3%），无法进入深度 idle；
2. 跨线程 `REQUESTRENDER`（视频流回调、异步加载完成）经 `wakeEventLoop` 唤醒是 **no-op**——渲染请求最多要等满 16.7ms 才被 gate 消费，异步结果上屏延迟一整个空闲周期。

**优化方向**（QNX 首选改造点）：

1. `QNXPlatform` override 两方法：等待侧用 QNX 原生 `ChannelCreate + MsgReceive` 或 `screen_get_event` 长超时阻塞（screen 事件本身即可作为唤醒源，可经 `screen_set_event_property` 关联 channel），唤醒侧 `MsgSendPulse`；
2. 空闲等待时长从固定 1/60 改为"无动画时无限等（纯事件驱动）、有 Tween 时等到最近动画到期时刻"——`TweenManager` 可算出最近到期时间；
3. 过渡方案（一天内可落地）：`idleWaitSeconds` 拉长 + 依赖输入事件超时唤醒，牺牲少量空闲响应换功耗。

### 10.2 输入路径（Q-2）

**现状**：`QNXInputProvider::poll` 用 `screen_get_event` 1ms 超时循环排空（`QNXInputProvider.cpp:14,74`）——每次 poll 无事件时固定阻塞 ~1ms，60fps 下吃掉 6% 帧预算，空闲时也在烧；只处理 MTOUCH，`SCREEN_EVENT_KEYBOARD`/`SCREEN_EVENT_POINTER` 认识但不处理（`QNXInputProvider.cpp:16-24,89-101`）——**仪表旋钮/硬键没有输入通道**（`TouchEvent` 结构已支持 KEY_DOWN/CHARACTER，`Platform.cpp:93` 已在消费，只差 provider 上报）。

**优化方向**：有事件排空时超时传 0（非阻塞），仅空闲路径用长超时（与 Q-1 合并为同一改造）；补 keyboard/rotary 的 keyCode 映射。

### 10.3 多窗口约束（现状知悉）

`GlobalObject` 进程级单例 + 单 `RenderingThread` + `Platform::initialize` 会重 new RenderDeviceProxy 且不 join 旧线程（`RenderingThread.cpp:12`），`Engine::~Engine` 经 `GlobalObject::destroy()` 清空全部管理器（`Engine.cpp:82-85`）——**同进程第二个 Engine 会踩坏第一个**。当前多屏（仪表+中控）只能一屏一进程。单进程多窗口需先把 GlobalObject/RenderingThread 改为 per-Engine 实例并让窗口显式 makeCurrent 自己的 surface。对"核心 UI 单屏"目标场景**不构成阻塞**，作为架构约束知悉即可。

---

## 11. 与既有路线图（ARCHITECTURE.md 第 15 节）的优先级对齐

按 ≤300 节点场景**重新评估**：

| 15 节条目 | 原状态 | 本文评估 |
|---|---|---|
| 15.19 P2 Material Uniform/UBO 提交收敛 | 候选 | **升级 P1**，与 R-1/R-2 合并为"字符串→槽位"统一改造（本文 7.1/7.2/9.2） |
| 15.19 P2 SSBO 打包去字符串查找 | 候选 | **升级 P1**（本文 R-1，运行时最大单点） |
| 15.3 性能时间线 | 非短期 | **前置为 P1 的依赖**——R 系列条目的验收都依赖阶段耗时分离，建议与首批优化同步落地最小版（每阶段耗时 + upload bytes） |
| 15.7 RenderItem SoA / 整数 ID | 非短期 | **维持非短期**：300 节点下收集+合批本身 <0.2ms（-O0 下），先做 R-5 的指针缓存即可覆盖大头 |
| 15.11 安全重排 | 非短期 | **降级**：300 节点 5-15 个 draw call 已接近最优，重排收益天花板低 |
| 15.18 字体 Atlas 分页 | 非短期 | **部分前置**：文案封闭场景用"正确初始尺寸 + 脏矩形上传"（S-5/T-1）即可，全量分页机制仍可等 |
| 15.10 纹理上传统计 | 非短期 | 与 M-3/M-4 合并推进 |
| 15.1/15.2 渲染回归与 CI | 非短期 | 维持；但 C-1/C-2 两个正确性修复建议带最小单测 |

本文档新增、15 节未覆盖的维度：**启动路径整体（第 6 节）、构建配置（第 5 节）、QNX 空闲功耗与输入（第 10 节）、每帧堆分配清扫（7.6）、布局隐藏放大链（7.3/7.4）**。

---

## 12. 实施顺序建议

按"收益/代价 + 依赖关系"排序为三个里程碑：

### M1：工程配置 + 启动（目标：冷启动时间数量级改善）

> **实施状态（2026-09-19，dev 分支）**：M1 七条已全部落地，Windows/clang Debug 全量编译通过。逐条状态：
>
> | 条目 | 状态 | 实施要点 |
> |---|---|---|
> | O-1 Release 编译配置 | ✅ | 根 CMakeLists 按 `CMAKE_BUILD_TYPE` 分支：Debug 保持 `-g -O0`；Release/RelWithDebInfo/MinSizeRel 追加 `-ffunction-sections -fdata-sections -fvisibility=hidden` + 链接 `-Wl,--gc-sections`，优化等级交给 CMake 内建 per-config flags；新增 `MORROW_ENABLE_LTO`、`MORROW_ENABLE_STRIP` 选项（默认 OFF） |
> | O-2 诊断默认值 + DebugPlane 懒创建 | ✅ | `MORROW_ENABLE_OBJECT_DIAGNOSTICS`/`MORROW_ENABLE_DEBUG_OVERLAY` 默认值跟随构建类型（Release 系默认 OFF，可显式覆盖）；DebugPlane 推迟到首次 setVisible/toggle 时创建（`Engine::ensureDebugPlane`） |
> | O-3 模块裁剪开关 | ✅（范围调整） | `MORROW_ENABLE_BASISU`（默认 ON）落地：裁掉 basis_universal 转码器与 Basis/KTX2 加载器，Texture.h/cpp 以宏守卫；**GLTF 裁剪未做**——tinygltf/nlohmann 与 `MeshRenderer3D`/`Scene3DAsyncLoader`/scene3d 子系统耦合，需要先做 scene3d 解耦重构，移入 M2 后续评估 |
> | S-3+C-3 CommandBuffer | ✅ | 容量经 `EngineOptions::deviceOptions.commandBufferCapacity` 配置（0=默认 16MB）；溢出安全路径：Debug assert，Release 丢弃命令（payload 在独立 arena 构造/析构，绝不越界），endFrame 输出 LOG_E 含丢弃数与容量 |
> | S-1 字体加载 | ✅ | 按路径去重（别名注册复用同一份字体数据与图集）；`skipDefaultFont` 跳过默认 8.4MB 字体；`asyncFontPreload` 工作线程预读与 EGL 初始化并行（`AdoptFontData` 零拷贝接管）；空图集惰性上传（首个字形时才整图部署，未使用字体零上传）；mmap 留作后续 |
> | S-2 Shader 二进制缓存 | ✅ | 新增 `ShaderBinaryCache`（renderer/device/）：**内容哈希键** = FNV-1a64(格式版本 + GL_VERSION/VENDOR/RENDERER + 完整预处理后 vert/frag 源码)，修改 shader 源码或更换驱动后哈希自动变化、旧缓存自然失效，无需手工版本号；文件含魔数/版本/键校验，临时文件+rename 原子写入；加载后 LINK_STATUS 校验失败自动回退源码编译；`shaderCacheFormatVersion` 保留引擎级强制失效入口；默认目录 QNX=/var/data/shaders、桌面=关闭（`EngineOptions::deviceOptions.shaderBinaryCacheDir` 可改）。**变体裁剪结论**：SSBO/非 SSBO 双变体本就按渲染路径懒编译（apply/applyBatch 各自首次使用才编译），无强制双编译；缓存落地后二次启动双变体加载成本≈0，不做强制单变体（有语义风险） |
> | S-4 SSBO 预设 | ✅ | `EngineOptions::deviceOptions.ssboSupportPreset`（-1 查询/0 强制关/1 强制开）；预设时跳过启动期 `checkSSBOSupport` 的主线程↔渲染线程同步往返 |
> | S-5 Atlas 初始尺寸 | ✅ | `EngineOptions::fontAtlasInitialSize`（0=1024，CJK 建议首启 2048）；配合空图集惰性上传，起步大图集不再付出"上传全零"的启动代价 |
> | C-1 Basis/KTX2 所有权 | ✅ | `textureDataSharedPtr` 改为别名持有：deleter 捕获 TextureInfo 引用保证像素存活到 GPU 命令执行完毕，最后引用释放时 `basisData.clear()`；消除对 vector 内部指针 `delete[]` 的双重释放 |
>
> 新增公共配置入口（`EngineOptions`）：`deviceOptions`（RenderDeviceOptions：commandBufferCapacity / shaderBinaryCacheDir / shaderCacheFormatVersion / ssboSupportPreset）、`skipDefaultFont`、`asyncFontPreload`、`fontAtlasInitialSize`。
>
> **构建验证（Windows / MSYS2 clang 21，2026-09-19）**：Debug 与 Release 全量构建通过；26/26 单元测试通过；Release（-O3 + gc-sections + 静态链接）ImageDemo 4.89MB vs Debug 7.88MB（-38%，未开 strip/LTO）。顺手修复了 samples 的 POST_BUILD 资产拷贝并行竞态（改为单一 `morrow_copy_assets` 自定义目标）。
>
> 待办：QNX 目标机上的启动分解实测（含二进制缓存命中率）、QNX Release 产物（-flto/-Os/-strip 组合）体积与性能基线、GLTF 裁剪所需的 scene3d 解耦评估。

1. O-1 Release 编译配置（含基线 benchmark 建立——一切后续优化的前提）；
2. O-2/O-3 诊断默认值 + 模块裁剪开关；
3. S-3 CommandBuffer 容量可配置 + C-3 溢出安全路径（同一改动）；
4. S-1 字体 mmap/异步 + 按 path 去重；
5. S-2 shader 二进制缓存 + 变体裁剪；
6. S-4 checkSSBOSupport 预置；S-5 atlas 初始尺寸；
7. C-1 Basis/KTX2 所有权修复（正确性，随 M1 带上）。

### M2：动效运行时（目标：满负荷动效主线程 < 2ms @Release）

> **实施状态（2026-09-20，dev 分支）**：M2 曾于 2026-09-19 全部实施（阶段耗时采集、R-1~R-6、T-1、C-2 及 MeshFilter/MeshRenderer/Shadow 组件缓存），随后在真实 demo 中出现 **UI 位置重叠错乱**（元素堆叠/缺失）。经两轮修复与端到端诊断（世界矩阵数值 + 帧缓冲像素双层校验，单线程路径全部通过）未能定位根因，**全部改动已于 2026-09-20 回撤**，代码回到 M1 状态。
>
> **R-3 已于 2026-09-20 单独重新实施**（值相等检查本身无害，位置错乱根因大概率在 R-1 增量填充一侧）：
>
> - **实现**：`setPosition/setSize/setScale/setRotation/setPivot` 全部 setter 前置值比较，值未变直接 return（NaN 因 `NaN != NaN` 恒不等不会被吞；`setSize(float,float)` 的比较含 `m_size.z == 0` 以保留该重载强制写 z=0 的语义）。`setSize` 值未变时连 `notifySizeChange` 都不触发，MeshFilter 几何重建随之归零（7.3 优化方向第 2 条自动覆盖）。
> - **关键架构确认（修正此前记录）**：上次记录的"子节点局部矩阵依赖父尺寸、无版本跟踪"缺口**实际不存在**——`Transform::getWorldMatrix` 的 `parentChanged` 检测（缓存父世界版本比对）会在父世界版本变化时强制重算子节点局部矩阵，重新读取父尺寸。因此 R-3 纯值比较是安全的，**无需**上次附加的 `invalidateChildrenOffset`（当时修的是不存在的缺口，也侧面说明错乱根因不在 R-3）。注意 `getLocalMatrix()` 单独调用不含 parentChanged 检测（引擎内无外部调用点，仅 getWorldMatrix 路径，行为无害）。
> - **验证**：新增 `tests/TransformValueCheckTests.cpp`（同值无操作/监听跳过、父尺寸后置与运行中传导、孙节点复合抵消不变量、NaN 不吞、z 语义）；重建 `samples/PosDiag.cpp` 端到端诊断（绝对定位/父尺寸运行中 resize/HBox/Margin 四场景，单线程矩阵+像素双层、`--mt` 多线程矩阵校验）。Debug 27/27 单测全过；Release 同套验证全过；ControlsDemo/AnimationEffectsDemo 冒烟无异常。
> - **测试期间发现的测试数学错误（非引擎错误）**：孙节点世界位置在直接父节点 resize 时不变——middle 局部平移含 `+middleW/2`、leaf 局部平移含 `-middleW/2`，两者相抵（子节点按自身左上语义锚定）。
>
> **C-2 已于 2026-09-20 单独重新实施（含架构级循环）**：
>
> - **问题陈述（修正）**：旧实现 `TweenManager` 用 `currentValue == targetValue` 浮点相等判完成。`from != to` 的常规路径完全正常（onUpdate 每帧、onComplete 到点触发后被回收）。唯一分歧场景是 `from == to`：第 1 帧 onUpdate 带正确值正常触发，但 tween 随即被回收，completion 分支永远走不到，**onComplete 不触发**（已对照旧代码实测确认）。
> - **实现**：`Tween` 增加 `m_finished`/`isFinished()`——仅在自然播放到时长终点时置位，`play()`（含 restart 路径）复位，`stop()` 不置位（保持可复播语义）；manager 回收条件改为 `isFinished()`。在 `from != to` 时与旧判定等价，差异仅在 `from == to`。
> - **架构级循环 `setLoop(loopCount)`**：`<0` 无限循环、`1`（默认）单次、`N>1` 共 N 轮；每轮边界不产生回调，超出时长结转保持相位；onComplete 仅全部轮次结束后触发一次；`duration<=0` 立即完成（防除零/空转）。**循环语义必须由状态化完成判定支撑**：循环 tween 在每轮边界值恰好等于 target 且必须继续播，值相等回收会把循环动画杀掉——旧的"onComplete 里 restart"能存活只是因为 `restart→stop()` 恰好把值重置回 from。回调内 restart 的旧用法仍兼容（play 复位 finished）。
> - **ImageDemo 已切换**：呼吸动画改用 `setLoop(-1)`，移除 onComplete 内 restart（顺带消除 tween 在自身回调里捕获自身 shared_ptr 的引用环）。
> - **restart 调用点已全量清理（2026-09-20）**：全仓库 9 处 onComplete-restart 循环全部改为 `setLoop(-1)`——引擎元素 5 处（MRFlowingLight / MRGearsOpening / MRGearsShine / MRGearsSelect / MRGearsIris，模式均为线性 tween 驱动 shader time uniform）、demo 4 处（AnimationEffectsDemo ×3、RangeControlsDemo ×1）。回调内 restart 现仅作为兼容语义保留（Tween::restart 本身保留，用于手动重播）。
> - **验证**：本地对照验证 7 组场景全过（from==to 全时长 + onComplete 一次、无限循环不回收不完成、3 轮有限循环中途不完成/结束后恰好一次、回调内 restart 兼容、常规完成一次性、stop 冻结可复播、kill 不完成）；全量构建 26/26 单测过；ImageDemo Release 冒烟无异常。
>
> 其余条目暂缓整体推进，重新实施时建议：
>
> 1. **先补位置回归测试再动代码**：端到端校验（矩阵数值 + glReadPixels 像素）必须覆盖 **`multithread = true` 默认路径**（`EngineOptions::multithread` 默认即为 true）——上一轮诊断只覆盖单线程；异步纹理加载导致的批次成员跨帧变化、渲染线程时序均未被触达（PosDiag 已支持 `--mt`）；
> 2. **分条小步合入**：按 R-5 → R-2 → R-1 → R-4 → R-6 → T-1 → C-2 逐条独立验证（构建 + 单测 + demo 视觉确认）后再进下一条，避免再次整体回撤；
> 3. 回撤前未验证完的头号嫌疑，重做 R-1 时优先排查：SSBO 增量填充在"批次实例数量不变但成员重排"及多线程时序下的正确性。

1. 最小版性能时间线（阶段耗时分离，兼作 M2 验收工具）；
2. R-3 Transform 值检查（先行，一行级改动立即止血布局场景）；
3. R-1/R-2/M-2 "字符串→槽位"统一改造（SSBO 填充 + uniform 命令 + Material 容器，一个 epic）；
4. R-4 布局脏标记；R-5 指针缓存族；
5. R-6 杂项清扫（requestRender/MRButton/Shadow/粒子/Observable）；
6. T-1 字形脏矩形上传；R-7 SwapInterval/FPSController；C-2 Tween 修复。

### M3：平台与轻量化（目标：静止功耗趋零 + 常驻内存达标）

1. Q-1 QNX 事件驱动等待 + 唤醒（静止功耗关键）；
2. Q-2 输入阻塞策略 + keyboard/rotary 通道；
3. M-1 节点分配收敛（共享 quad / UUID / make_shared）；
4. M-3 池上限与水位；M-4 KTX2 线程化；M-5 depth 去除；
5. T-2 预烘焙 SDF atlas（量产路径，依赖资管线）。

### 建议暂缓（维持 15 节非短期状态）

RenderItem SoA、安全重排、RenderGraph、Window/UI root 解耦、非 SSBO 复杂实例化、树线性化（L-1，除非 M2 后 profiling 显示遍历仍是主项）。

---

## 13. 验收与回归

- 每个里程碑前后用**同一组固定场景**采集：启动分解耗时 / 每帧 CPU 分解 / draw call 与断批分布 / upload bytes / 常驻内存 / 二进制体积；
- 场景建议：①纯静止首页（验空闲功耗）；②持续动效页（300 节点 + 呼吸灯/转场，验 M2）；③中文长文本首屏（验 S-5/T-1）；④列表增删动效（验批次重建路径）；
- 视觉正确性：SSBO 与标准路径画面一致、合批语义不变（15 节架构不变量第 7-9 条继续适用）；
- C-1/C-2 修复各带最小单测（纹理加载所有权、tween 完成时序）。
