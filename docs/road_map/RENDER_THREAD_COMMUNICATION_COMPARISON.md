# MorrowUI 主线程↔渲染线程交互模型评估（对标 Godot / Filament / Unity / UE）

> 基准：Godot 4.8-dev 本地源码（逐行核实）；Filament main 分支（GitHub 多来源交叉）；
> Unity 6 官方文档；UE5 官方文档与源码导读。
> 性质：评估 + 对比结论文档。随代码演进维护。

---

## 1. 结论摘要

1. **MorrowUI 的 payload 方案是业界主流形态，且与最现代的两个实现同构。**
   「typed command struct + `[CommandHeader|Aligned Payload]` 字节流 + 定容环形槽位 +
   信号量背压 + fence 门控回收」正是 Filament（`CommandStream` + `CommandBufferQueue`）
   与自研渲染后端的经典设计；Godot 的 `CommandQueueMT` 是同族变体（typed struct +
   成员函数指针 + 无界线性缓冲）。**方案本身不需要推翻或重构。**
2. **payload 所有权协议（shared_ptr 随命令转移 + fence 后回池）与业界三类做法同属
   一个家族**：Godot 用引用计数句柄（`Ref<Image>` / CoW `Vector`）隐式转移；
   Filament 用 BufferDescriptor + 释放回调；UE 用「渲染线程自有 proxy + 命令捕获 +
   fence」。MorrowUI 的协议更简单、显式，UI 规模下 refcount 开销可忽略。
3. **帧间深度（kRingSize=3，主线程可领先 2 帧）比 Godot/UE/Unity 的典型
   「1 帧流水线」更解耦**，与 Vulkan「N frames in flight」共识一致；对帧成本不均的
   UI 场景（偶发全屏重建）是合理取舍。
4. **两处值得了解的业界差异（非缺陷）**：
   - **溢出策略**：MorrowUI Release 下「丢命令、提交残帧 + LOG_E」；Filament 同为
     定容但溢出视为**不可恢复错误直接崩溃**（杜绝残帧）；Godot 无界增长永不溢出
     （无背压但有内存风险）。UI 命令量距 16MB/槽很远，当前策略可维持；
     若要更稳，P2 方向见 §6.2。
   - **命令分派**：MorrowUI 用 header 类型 tag + `switch`（executeFrame ~50 case）；
     Godot 用虚函数 `call()`、Filament 在命令头存函数指针。当前规模 switch 无问题，
     命令数上百时再考虑函数指针化。
5. **多生产者/并行编码是全行业的前进方向（bgfx 多 encoder、Unity Graphics Jobs、
   UE 并行 draw command 生成），但所有主流引擎也都支持单生产者退化**——UI 引擎
   当前单生产者设计完全成立，列为远期备忘（P3）。

---

## 2. MorrowUI 现状速览（评估对象）

- **编码形态**：typed command struct + `[CommandHeader | 16 字节对齐 Payload]` 平缓冲；
  非平凡 payload 经 `pushNT` placement-new，析构链注册（`RenderDeviceProxy.cpp:74+`）。
- **帧隔离**：`kRingSize = 3` 环形槽位，每槽一个预分配 `CommandBuffer`
  （默认 16MB，`CommandBuffer.h:46`）；主线程 `m_frameReadySem` 发帧、`m_freeSlotsSem`
  满时阻塞——最多领先渲染线程 2 帧（`RenderDeviceProxy.h:175-225`）。
- **执行**：渲染线程 `runCommand()` 等信号量 → `executeFrame` 顺序 switch 分派 →
  `GLRenderDevice` 执行 → `endSSBOFrame()` fence 落账 → `tryRecycle()` fence 后回池。
- **payload 所有权**：shared_ptr 随命令转移（命令槽持引用），执行后转入帧级回收列表
  （`PendingFrame`），`waitFence(timeout=0)` 成功才回 `RecyclePool`
  （VBO 256 / UBO 256 / SSBO 64 / Pixel 16，像素池 fence 后即回）。
- **回读**：`std::promise` 同步往返命令（`checkSSBOSupport` / `reflectSSBOBlock`），
  仅启动期能力探测使用。
- **单线程模式**：`multithread=false` 时 proxy 方法直调 device，回收走同一条
  PendingFrame/fence 链（单线程=主线程即 GL 线程）。
- **溢出策略**：Debug assert；Release 丢弃超量命令 + `LOG_E`，帧按部分提交
  （`RenderDeviceProxy.cpp:907-914`）。

---

## 3. 各引擎做法

### 3.1 Godot 4.8（本地源码逐行核实）

**形态：RenderingServer 服务端模式——命令队列 + RID 句柄对象，主线程只提交命令。**

- **命令编码**：`CommandQueueMT`（`core/templates/command_queue_mt.h`）。
  每条命令 = `[uint64 size 头][placement-new 的 Command<T,M,Args...> 对象]`，
  命令体存 `T* instance` + **成员函数指针** + `Tuple<decay_t<Args>...>`；
  分派靠虚函数 `call()`（执行时参数逐个 move 出 Tuple）。单条命令上限 1024B
  （static_assert）。调用方 API 由 `server_wrap_mt_common.h` 的 FUNC0~15 宏生成。
- **容量与背压**：**无界线性缓冲**（`LocalVector<uint8_t>` 初始 reserve 64KB，
  push 时 resize 增长），push 永不阻塞、永不触发主线程 flush；flush 完成后
  `clear()` 只归零计数保留容量。Godot 3 的固定分块池 + 满时休眠机制在 4.x 已移除。
- **回读**：`push_and_sync` / `push_and_ret` / `sync()` —— sync 标记命令 +
  `sync_head/sync_tail` 计数器 + condvar（无 promise）；返回值写调用方栈上指针。
  `Main::iteration` 每帧开头 `RenderingServer::sync()`：主线程等渲染线程清空
  上一帧遗留命令——**典型 1 帧流水线**（主线程提交 N+1 时渲染线程画 N）。
- **大块数据所有权**：参数按 decay 拷贝进 Tuple，但 Godot 大块数据全是**引用计数
  句柄**——`Ref<Image>`（侵入式引用计数）、`Vector<uint8_t>`（CoW，`SafeNumeric`
  头引用计数）。拷贝的只是句柄，无深拷贝；渲染线程执行时 move 出 Tuple。
- **GPU 资源删除安全**：RS 层 free 命令不等 GPU；RD 层 `free_rid` 把驱动资源塞进
  `frames[frame].*_to_dispose_of` 延迟销毁槽，帧循环轮转到同一 frame 索引时
  `_free_pending_resources` 统一销毁（`rendering_device.cpp:7735-7835, 8157-8264`）
  ——与 MorrowUI 的 fence 门控回收是同一思想（按 in-flight 帧槽延迟销毁）。
- **渲染线程**：WorkerThreadPool 上的专用 pump task，主循环
  `yield(); command_queue.flush_all();`（condvar 睡眠，push 侧唤醒）。
  present/swap 在渲染线程执行。
- **单线程退化**：项目设置 `thread_model`；`create_thread=false` 时 wrapper 宏按
  `Thread::get_caller_id() != server_thread` 判定，直调 + `flush_if_pending()`。

### 3.2 Filament（Google，main 分支多来源交叉）

**形态：Engine 双线程（"1 thread for the user, 1 thread for the backend"）+
CommandStream/CommandBufferQueue，与 MorrowUI 结构同构度最高。**

- **命令编码**：Driver API 方法由 `DriverAPI.inc` 生成——上层调用即把 `CommandBase`
  块写入 **CircularBuffer**（单生产者单消费者环形缓冲，含 high-watermark 统计）。
- **缓冲交接**：`CommandBufferQueue` 持有 `SLOT_COUNT` 个缓冲槽，槽状态用原子
  `CommandFlag` 管理，后端线程取走已提交的缓冲执行，用完归还空闲链。
- **容量与背压**：定容。缓冲溢出被视为**不可恢复错误**
  （"Backend CommandStream overflow — commands are corrupted and unrecoverable"，
  直接 abort，见 GitHub discussion #7015 的崩溃路径）——宁可崩溃不提交残帧。
- **大块数据所有权**：backend 公开 API 的 `BufferDescriptor` 系列携带
  `CallbackHandler` 回调——后端线程确认不再需要该内存时回调释放方，
  由所有者决定回收（池化/归还）。与 MorrowUI「fence 后回 RecyclePool」是同一协议
  的两种实现（回调式 vs 池+轮询式）。
- **节奏**：主线程只与 `Engine` 交互，从不直接触碰后端；帧步进由
  `beginFrame/endFrame` 约定，后端线程独立消化命令流。

### 3.3 Unreal Engine 5（官方文档 + 源码导读）

**形态：Game Thread / Render Thread / RHI Thread（4.22+ 三层）+ 场景镜像（proxy）+
每帧渲染器快照——重镜像模型，与 MorrowUI 的短路径命令流是两个流派。**

- **镜像对象**：每个 `UPrimitiveComponent` 在游戏线程创建 `FPrimitiveSceneProxy`
  （`CreateSceneProxy()`），**所有权移交渲染线程**；渲染线程拥有平行的 `FScene`。
  游戏线程状态变更通过 `ENQUEUE_RENDER_COMMAND` 捕获 lambda 进渲染命令队列。
- **每帧快照**：帧末游戏线程构造 `FSceneRenderer`（拷贝 per-view 状态）并入队；
  渲染线程执行可见性、排序、draw command 生成（静态路径缓存、动态路径每帧收集）。
- **回读/同步**：`FlushRenderingCommands` / `FRenderCommandFence`——游戏线程按需
  阻塞等渲染线程追平（资源删除、截图、读写回）；平时不逐帧硬阻塞，典型 1 帧滞后。
- **大块数据**：游戏线程填充共享数组 / RT 自有资源；mesh 数据经 RHI 资源创建命令
  一次性转移。删除安全靠 fence + proxy 的 `RenderThread` 所有权（析构命令在 RT 执行）。
- 代价：proxy 样板与双份场景状态（UE 世界规模所需）；收益：游戏线程几乎不因渲染
  停顿。MorrowUI 明确不走这条路（架构原则：不建第二套渲染世界）。

### 3.4 Unity 6（官方文档）

**形态：主线程 + 渲染线程（GfxDevice worker）的「API 调用序列化」模型，三种模式。**

- `RenderingThreadingMode`：**LegacySingleThreaded / MultiThreaded / GraphicsJobs**。
  - MultiThreaded（默认）：主线程把渲染工作序列化为命令，**渲染线程把命令翻译成
    平台图形 API 调用**，渲染线程约落后主线程 1 帧；队列满时主线程停等。
  - GraphicsJobs：命令翻译拆成分发到多个 worker 的并行 job（Vulkan/D3D12 支持
    最好）——多消费者的方向。
- 单线程模式：全部图形调用在主线程执行（与 MorrowUI 的 `multithread=false` 同义）。
- 大块数据：主线程填充共享 native 容器，渲染线程消费（BatchRendererGroup 等为
  DOTS 路径提供主线程直填缓冲）。

### 3.5 简要参照：bgfx 与原生 GPU API

- **bgfx**：多 encoder（多线程各自编码独立命令缓冲，提交时合并）+ 单消费者执行；
  大块数据走引用计数 `bgfx::Memory`（拷贝或带释放回调）。
- **Vulkan/D3D12/Metal 原生模型**：「N frames in flight」——CPU 每帧写入各自的
  command buffer/constant ring，per-frame fence 决定何时复用。**MorrowUI 的
  kRingSize=3 + per-frame fence + B2 持久映射环形分段，正是该共识的 GL 化实现。**

---

## 4. 横向对照表

| 维度 | MorrowUI | Godot 4.8 | Filament | UE5 | Unity 6 |
|---|---|---|---|---|---|
| 交互形态 | typed command 流（短路径，无场景镜像） | RenderingServer 命令队列 + RID 对象 | CommandStream 命令流（Driver API 生成） | proxy 镜像 + 每帧 FSceneRenderer 快照 + 捕获 lambda | API 调用序列化（GfxDevice worker） |
| 命令头 | 类型 tag + size，`switch` 分派 | 裸 size 头，虚 `call()` 分派 | `CommandBase`（函数指针分派） | lambda（闭包即命令） | 内部序列化格式 |
| 容量 | 定容 16MB/槽 ×3 槽 | **无界增长**（reserve 64KB，flush 后保容量） | 定容（CircularBuffer + SLOT_COUNT 槽环） | 可增长（命令列表分配器） | 内部队列（满则主线程停等） |
| 溢出策略 | Debug assert；Release 丢命令+LOG_E，残帧提交 | 不存在溢出 | **不可恢复错误，abort** | 不适用 | 背压阻塞，不丢 |
| 背压点 | 主线程在 freeSlots 信号量阻塞（领先>2 帧时） | 无背压；每帧 `sync()` 隐式节流（1 帧流水线） | 队列槽耗尽时生产者等待 | 按需 `FlushRenderingCommands`；典型 1 帧滞后 | 队列满时主线程停等（~1 帧） |
| 主线程领先渲染线程 | 最多 2 帧 | ~1 帧（sync/draw 分离） | 由后端线程消化、帧约定 pacing | ~1 帧 | ~1 帧 |
| payload 所有权 | shared_ptr 随命令转移 → fence 后回池 | 引用计数句柄隐式转移（`Ref<Image>` / CoW Vector） | `BufferDescriptor` + 释放回调 | RT 自有 proxy + 捕获指针 + fence | 共享 native 容器 |
| GPU 删除安全 | 帧级 PendingFrame + fence | RD per-frame `_to_dispose_of` 槽 | descriptor 回调时机由后端控制 | `FRenderCommandFence` + RT 析构命令 | RT 侧释放 |
| 回读 | promise 往返（仅启动期能力探测） | sync 命令 + condvar 计数器（任意时点可用，DEBUG 告警） | fence/回读回调 | FlushRenderingCommands（贵，按需） | Readback API（异步回调） |
| 单线程模式 | 有（直调 + 同一回收链） | 有（thread_model 设置） | 后端线程常驻（可配置 standalone） | 有（No Threaded Rendering） | 有（LegacySingleThreaded） |
| 并行编码 | 无（单生产者） | 无（单 mutex 生产者） | 无（单生产者） | 有（并行 draw command 生成） | 有（GraphicsJobs 模式） |

---

## 5. 业界共识（MorrowUI 已对齐的部分）

1. **typed 命令 + 字节流 + 单消费者**：Godot / Filament / MorrowUI 三者同构，是
   「无镜像短路径」流派的收敛形态；UE/Unity 的重模型服务于开放世界规模，
   UI 引擎不需要。
2. **N frames in flight + per-frame fence**：Vulkan 共识；MorrowUI（3 槽 + 
   PendingFrame fence）与 Godot RD（frame 槽 `_to_dispose_of`）实现的是同一个东西。
3. **所有权显式协议**：shared_ptr 转移（MorrowUI）/ 引用计数句柄（Godot）/
   descriptor 回调（Filament）——三族等价，关键是「命令执行前数据必有效、
   GPU 完成后才回收」不变量；MorrowUI 用 fence 门控回收池严格满足。
4. **回读是特例不是常态**：所有引擎都把同步回读收敛到启动探测/显式异步回读，
   热路径零阻塞。MorrowUI 只在启动探测用 promise——正确用法。
5. **单线程模式是标配**：调试、嵌入式、工具场景都需要；MorrowUI 的单线程直调
   与 Godot/Unity 的退化路径语义一致。

## 6. 逐维度评估与可选演进

### 6.1 合理性确认（保持，不动）

- payload 方案整体：同构 Filament/Godot，短路径无镜像，符合架构原则 3.4。
- shared_ptr 所有权协议：与 Godot 引用计数同族；UI 帧内 payload 数量级小，
  原子引用计数开销无关紧要。
- 3 槽深度 + 信号量背压：比主流「1 帧滞后」更解耦，帧成本不均时主线程更平滑；
  内存代价可控（3×16MB 命令缓冲常驻，量产可按 `commandBufferCapacity` 调小——
  与 Unity「队列满才停等」相比是「预分配换确定性」的取舍，嵌入式友好）。
- 像素池 fence 后即回、VBO/SSBO 池 fence 门控：与 Filament 回调式协议同义，
  实现更直白。

### 6.2 可选演进（按需，不构成缺陷）

| 方向 | 现状 | 业界参照 | 建议 |
|---|---|---|---|
| 溢出策略 | Release 丢命令 + 残帧提交 | Filament：溢出即 abort（不提交残帧）；Godot：无界不丢 | UI 命令量距 16MB 很远，风险理论化；若要更稳：P2——溢出时改为「整帧丢弃 + 跳过 present + 强诊断」，比残帧更可控 |
| 命令分派 | header tag + `switch`（~50 case） | Godot 虚调用；Filament 函数指针 | 当前规模 switch 分支预测友好，保持；命令数过百再函数指针化 |
| payload 拷贝 | 大块数据 memcpy 进 payload（VBO/UBO 路径） | Godot CoW 句柄零拷贝；Filament descriptor 回调；MorrowUI SSBO 已走持久映射（B2） | SSBO 已解决；VBO/UBO 若实测成热点，可参照 B2 持久映射化或引入 descriptor+回调协议（P2/P3，需 profiling 驱动） |
| 回读协议 | promise 同步往返 | Godot sync 命令 + 计数器；UE Flush | 仅启动期使用，已是最优实践；若未来出现运行时回读需求（如截图），走 fence + 回调异步化，勿加新同步点 |
| 多编码器 | 无 | bgfx 多 encoder / Unity GraphicsJobs / UE 并行 | UI 帧命令量小、单生产者够用；远期若出现「单帧命令编码占主线程大头」的 profiling 证据再评估（P3） |
| 缓冲容量自适应 | 固定 `commandBufferCapacity` | Godot 无界；Filament 定容+高水位统计 | 已有溢出诊断；可加高水位统计辅助量产调参（P3，顺手） |

## 7. 参考资料索引

本地源码（Godot 4.8-dev，`E:\WorkSpace\Client\godot`）：

| 文件 | 内容 |
|---|---|
| `core/templates/command_queue_mt.h` | CommandQueueMT 全部：命令布局/push/sync/flush |
| `servers/rendering/rendering_server_default.h/.cpp` | wrapper 宏、线程 init/_thread_loop/_draw/draw/sync |
| `servers/rendering/rendering_device.cpp/.h` | `free_rid` 延迟销毁、`Frame` 槽 `_to_dispose_of` |
| `servers/server_wrap_mt_common.h` | FUNC 宏与 DEBUG sync 告警 |
| `core/object/worker_thread_pool.cpp` | pump task / yield / notify |
| `main/main.cpp` | thread_model 开关、iteration 的 sync/draw 时序 |
| `core/templates/cowdata.h` / `local_vector.h` | CoW 引用计数、clear 保容量 |

网络来源：

- Filament：[Engine.cpp（双线程注释/高水位）](https://github.com/google/filament/blob/main/filament/src/details/Engine.cpp)、
  [CommandStream overflow 不可恢复错误（discussion #7015）](https://github.com/google/filament/discussions/7015)、
  [Filament 架构文档](https://google.github.io/filament/Filament.html)、
  [Filament 架构解读（CSDN，CommandStream/CircularBuffer）](https://blog.csdn.net/junzia/article/details/141300106)
- Unity：[RenderingThreadingMode API](https://docs.unity3d.com/6000.6/Documentation/ScriptReference/Rendering.RenderingThreadingMode.html)、
  [Configure graphics jobs mode](https://docs.unity3d.com/6000.2/Documentation/Manual/configure-graphics-jobs-mode.html)、
  [PlayerSettings.graphicsJobs](https://docs.unity3d.com/2018.3/Documentation/ScriptReference/PlayerSettings-graphicsJobs.html)
- UE：[Graphics Programming Overview（官方）](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)、
  [Scene Proxies in Unreal Engine](https://www.dr-elliot.com/posts/general/unrealsceneproxies)、
  [Unreal Source Explained – rendering](https://github.com/dongwei76/unreal_source_explained/blob/master/main/rendering.md)
