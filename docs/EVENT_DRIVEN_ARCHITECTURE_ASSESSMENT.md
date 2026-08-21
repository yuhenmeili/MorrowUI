# MorrowUI 分层事件驱动架构适配性评估

> 分析基线：2026-08-21  
> 对照对象：`UnravelEngine/EVENT_DRIVEN_SYSTEM_ANALYSIS.md`、`UnravelEngine/engine/engine/events.h`  
> MorrowUI 范围：Engine 主循环、`Observable`、Platform/Input、Widget UI 事件、Editor 和渲染线程边界。

> 实施状态：P0、P1、P2 已于 2026-08-21 落地。本文前半部分保留实施前问题分析，
> 第 9 节记录的 P0-P2 已按当前源码更新。

## 1. 结论

MorrowUI **适合引入分层事件驱动架构，但不适合直接照搬 UnravelEngine 的事件表和接入方式**。

准确地说：

1. 当前 `Observable<>` 可以在改造后作为类型安全多播事件的底层原语；
2. 当前 `Engine::m_preRender` 和 `Engine::m_afterRender` 不能替代 UnravelEngine 的 `events.h`；
3. `events.h` 的价值不只是使用了某种 signal/event 容器，而是集中定义了引擎阶段、参数、顺序和生命周期契约；
4. MorrowUI 应保留 Widget 树直接遍历、Platform 直接协调和 UI 目标事件派发，只在跨系统阶段与生命周期边界引入事件；
5. 建议先建立轻量的 `EngineEvents`、`EditorEvents` 和主线程任务队列，不应为了形式统一把所有调用都改成事件。

推荐判断：

| 问题 | 结论 |
| --- | --- |
| 是否引入分层事件驱动架构 | 建议引入 |
| 是否直接复制 UnravelEngine `events.h` | 不建议 |
| `m_preRender/m_afterRender` 是否已等价于 `events.h` | 不等价 |
| `Observable` 是否可以继续使用 | 可以，但只能作为改造后的底层 primitive |
| 是否引入全局万能 EventBus | 不建议 |
| Widget/Component 每帧更新是否改成订阅事件 | 不建议 |
| 输入事件是否并入 Engine 全局事件 | 不建议，保留输入/UI 局部层 |
| 是否现在引入 EnTT signal 层 | 不建议，当前没有对应 ECS 生命周期需求 |

一句话概括：

> MorrowUI 应借鉴 UnravelEngine 的“分层、阶段和边界”设计，不应把两个无序 `Observable` 钩子包装成一个全局总线后就认为完成了事件驱动改造。

## 2. 两个项目的架构前提不同

UnravelEngine 是 runtime、editor、game runner、play mode、ECS、脚本和多个场景系统共同运行的综合引擎。它需要通过 `engine::events` 广播：

- 每帧生命周期；
- fixed update；
- play/pause/resume；
- OS event；
- project open；
- script recompile。

MorrowUI 当前的核心模型更集中：

```text
Engine
    -> Platform
        -> Window
            -> Widget tree
                -> Component update / lateUpdate
        -> RenderDeviceProxy / RenderingThread
```

其主要工作是输入命中、Widget 树更新、合批收集、GPU 命令提交和 Present。大量操作已经存在明确的所有者和调用顺序，不需要为了“事件驱动”而拆成广播。

因此，MorrowUI 引入事件架构的主要目的应是：

- 为 Engine、Editor、异步加载器和诊断模块提供稳定扩展点；
- 明确主循环阶段语义；
- 降低跨层反向依赖；
- 统一订阅生命周期；
- 避免依赖注册顺序；
- 为后续 editor/runtime 扩展留出边界。

它不应成为新的通用调度器、ECS 替代品或线程通信机制。

## 3. MorrowUI 当前事件与帧时序

### 3.1 实际主循环

当前 `Engine::render()` 的有效时序为：

```text
updateFrameState
    -> Platform::beginFrame
       - poll platform input
       - resolve input targets
       - notify resolved input observers
    -> Platform::dispatchEvents
       - dispatch to target Widget
       - run callAfterTouched
    -> request-render gate
       - no request: heartbeat + wait, skip remaining phases
    -> m_preRender.notify()
    -> TweenManager::update
    -> Platform::beginRenderPass
    -> Platform::updateWidgets
    -> Platform::lateUpdateWidgets
    -> Platform::commitRenderPass
    -> increment frame number / diagnostics
    -> m_afterRender.notify()
    -> Platform::endFrame
    -> FrameState::callAfterRender callbacks
```

这个顺序揭示了两个命名问题：

- `preRender` 实际发生在 Tween 和 Widget update 之前，更接近 `on_before_update` 或“主线程待处理工作泵”；
- `afterRender` 发生在命令提交之后、`endFrame/present` 之前，并不是真正的完整帧结束。

另外，`FrameState::callAfterRender` 发生在 `Platform::endFrame()` 之后，与 `m_afterRender` 不是同一个阶段。二者不能继续只靠名称区分。

### 3.2 当前事件机制已经分层，但没有形成明确契约

当前存在三种不同机制：

| 机制 | 当前用途 | 性质 |
| --- | --- | --- |
| `Observable<T...>` | Engine 前后钩子、resolved input 广播 | 通用多播回调 |
| `EventDispatcher` | Widget/Interaction 的 `TouchEventType` 监听 | UI 目标事件 |
| `FrameState` callback vector | `callAfterTouched`、`callAfterRender` | 当帧一次性延迟任务 |

这些机制本身可以共存，但必须明确边界：

- `Observable`：长期订阅的同步广播；
- `EventDispatcher`：特定 UI 目标上的输入事件；
- callback queue：只执行一次的延迟任务；
- 直接调用：已知所有者之间的强顺序协调。

## 4. 为什么 `m_preRender/m_afterRender` 不能替代 `events.h`

### 4.1 事件数量不是核心差异

即使继续添加：

```cpp
Observable<> m_frameBegin;
Observable<> m_beforeUpdate;
Observable<> m_beforeRender;
Observable<> m_frameEnd;
```

也只是增加了回调容器。要达到 UnravelEngine `events.h` 的架构作用，还需要定义：

- 每个阶段准确发生在什么操作之前或之后；
- 回调参数是否包含 `FrameState` 和 delta time；
- 同一阶段是否允许排序；
- 谁拥有事件；
- 订阅者销毁后如何自动断开；
- 派发期间 add/remove 的语义；
- 是否允许重入；
- 回调在哪个线程执行；
- 异常是否允许穿透；
- 按需渲染跳帧时哪些事件仍会触发。

`events.h` 是一组架构契约，`Observable` 只是可能承载该契约的容器。

### 4.2 当前 `Observable` 不具备帧调度需要的顺序语义

`Observable` 使用：

```cpp
std::unordered_map<std::string, std::function<void(T...)>>
```

因此回调顺序不稳定。它既没有 priority，也没有稳定的插入顺序。

对于 `Scene3DAsyncLoader::pumpPendingResult()` 这种独立工作，顺序通常无关紧要；但如果以后动画、布局、脚本、资源应用和 editor 都订阅同一个帧事件，隐式无序会迅速变成正确性风险。

建议优先通过**多个明确阶段**表达大顺序，只在同一阶段确有多个独立订阅者时使用少量 priority band。不要用大量整数 priority 代替清晰的阶段划分。

### 4.3 当前订阅生命周期依赖手工约定

当前 `add()` 返回 UUID 字符串，调用方必须：

```cpp
auto id = observable.add(callback);
observable.remove(id);
```

`Scene3DAsyncLoader` 同时使用 weak capture 和析构/取消时手工 remove；`EditorShell` 则捕获 `this` 并在析构时手工 remove。

风险包括：

- 新增提前返回路径时遗漏 remove；
- observer owner 已销毁但回调仍保留；
- event owner 先销毁时，保存的字符串 handle 没有状态语义；
- 重建 editor panel 或 runtime 对象时产生悬空捕获。

全局或长生命周期 Engine event 应提供 RAII `Connection`，至少支持 move、`disconnect()` 和析构自动断开。也可以增加 weak owner/sentinel，但不应继续要求每个订阅者手写字符串句柄管理。

### 4.4 当前派发修改语义不完整

`Observable::notify()` 会先复制 callback snapshot：

- notify 中新增 observer：本轮不执行；
- notify 中移除尚未执行的 observer：其 callback 已在 snapshot 中，本轮仍会执行；
- callback 可以嵌套 notify；
- callback 抛异常时，后续 observer 不再执行；
- observer 的执行顺序来自 `unordered_map`，不可预测。

“新增从下一轮生效”是合理语义，但“已移除 observer 本轮仍执行”必须明确接受或修正。作为 Engine 生命周期事件时，建议 slot 保留 `connected/removed` 状态，派发前再次检查，而不是只复制裸 `std::function`。

### 4.5 当前实现不适合作为跨线程事件总线

`Observable` 没有同步保护，`add/remove/notify/clear` 不能并发调用。

MorrowUI 已有 `RenderingThread` 和异步 GLTF 加载路径。事件化以后仍应坚持：

- Engine/Editor/UI event 只在主线程 connect、disconnect 和 emit；
- 渲染线程不直接订阅或派发 Engine event；
- worker 完成后投递到主线程任务队列；
- 主线程消费任务后通过现有 request-render 机制唤醒一帧；
- GPU 操作继续通过 `RenderDeviceProxy`，不能借事件绕过线程所有权。

事件系统不是线程安全队列。

## 5. 推荐的 MorrowUI 分层

### 5.1 Engine runtime events

由每个 `Engine` 实例拥有，不放进 `GlobalObject`：

```cpp
struct EngineEvents {
    Observable<> onFrameBegin;
    Observable<> onFrameEnd;
};
```

推荐语义：

| 事件 | 建议触发点 | 用途 |
| --- | --- | --- |
| `onFrameBegin` | 输入派发和 request-render gate 通过后、Tween 前 | 一次实际渲染帧开始、主线程结果应用 |
| `onFrameEnd` | `endFrame()` 和当帧一次性回调完成后 | CPU 侧完整帧收尾 |

P2 只引入这两个已经存在真实语义的阶段。后续若出现完整 CPU 更新后、GPU
提交前的真实订阅需求，再增加名称明确的事件，不预先建立空阶段。

### 5.2 Platform/Input events

保留 `InputEventsManager::getResolvedInputEventsDispatcher()` 的局部广播职责，不要把每个 `TouchEvent` 都复制到 `EngineEvents`。

建议后续将名称调整为：

```cpp
Observable<std::vector<TouchEvent>&> onInputResolved;
```

它表达的是“输入已完成目标解析”，适合 Editor 做全局输入路由。真正的目标控件事件继续走：

```text
Platform
    -> Widget::dispatchTouchEvent
        -> Interaction
            -> EventDispatcher
```

`EventDispatcher` 也应补充派发期间 remove/add 的安全语义，但它属于 UI 事件层，不需要和 Engine event 合并成同一种全局 API。

### 5.3 Editor events

Editor 当前通过 `EditorShell` 集中协调。随着 panel、selection、scene document、asset import 和 preview 增多，建议增加 editor 本地事件：

```cpp
struct EditorEvents {
    Observable<const std::string&> onSelectionChanged;
    Observable<> onSceneOpened;
    Observable<> onSceneClosed;
    Observable<> onRuntimeRebuilt;
    Observable<> onAssetDatabaseChanged;
};
```

具体 payload 应使用稳定领域类型，以上仅表示边界。Editor event 不应加入 runtime 的 `EngineEvents`，避免 morrow runtime 依赖 editor 类型。

### 5.4 一次性主线程任务

`FrameState::callAfterRender` 和异步加载结果本质上更接近 task queue，而不是长期事件订阅。

建议逐步建立：

```cpp
MainThreadDispatcher::post(Task);
MainThreadDispatcher::drain();
```

语义应明确为：

- 多生产者可投递；
- 仅主线程 drain；
- 每个任务最多执行一次；
- 投递时触发 request-render/wake；
- 本轮 drain 中新增任务是本轮还是下一轮执行，需要固定规则；
- Engine 退出时如何取消或清理。

`Scene3DAsyncLoader` 当前借 `preRender` 每帧轮询 pending result。主线程队列落地后，可由 worker completion 直接 post apply task，并请求渲染，不再为每个 loader 长期订阅每帧事件。

### 5.5 继续使用直接调用的部分

以下路径应继续直接调用：

```text
Engine -> Platform frame methods
Platform -> Window
Window -> Widget tree traversal
Widget -> ComponentManager
ComponentManager -> Component update/lateUpdate
Render submission -> RenderDeviceProxy
```

原因是这些调用：

- 所有者明确；
- 顺序严格；
- 数据上下文相同；
- 每帧高频；
- 不需要动态发现订阅者。

把每个 Widget/Component 都改成订阅 `onFrameUpdate` 会增加 slot 数量、破坏树顺序、加重生命周期管理，并让不可见节点跳过更新等规则变得分散。

## 6. 推荐的事件原语

没有引入 `hpp::event` 或新的 `Signal` 类型。P1 直接增强现有 `Observable`，
并一次性删除 UUID、`add()` 和 `remove()` 旧 API。

最低能力要求：

```text
类型安全签名
稳定连接句柄
RAII Connection
稳定执行顺序
可选 priority
派发期间安全 disconnect
明确的派发中 connect 语义
支持嵌套 emit 或显式禁止
主线程断言（debug）
无 UUID/字符串热路径
```

推荐接口形态：

```cpp
class Connection {
public:
    Connection(Connection&&) noexcept;
    Connection& operator=(Connection&&) noexcept;
    ~Connection();

    void disconnect();
    bool connected() const;
};

template<class... Args>
class Observable {
public:
    Connection connect(std::function<void(Args...)> callback,
                       int64_t priority = 0);
    void notify(Args... args);
    void clear();
};
```

实现容器优先考虑稳定 slot 列表和单调递增 `uint64_t` id。订阅数量是系统级规模时，不需要为理论上的极限性能引入复杂结构。

建议派发规则：

1. priority 高的先执行；
2. 同 priority 按连接顺序执行；
3. emit 中 connect 的 slot 从下一次 emit 生效；
4. emit 中 disconnect 的未执行 slot 本轮不再执行；
5. Connection 析构等价于 disconnect；
6. 默认只允许所属主线程操作；
7. Engine 事件回调原则上不得抛异常穿透事件总线。

如果没有真实的同阶段顺序需求，第一版可以只保证插入顺序，不实现 priority。等出现明确的系统级排序需求再增加 priority，避免把它过早变成隐式调度器。

## 7. 建议的目标结构

```text
Engine::render()
    |
    +-- EngineEvents
    |      +-- frame lifecycle extension points
    |
    +-- direct Platform coordination
    |      +-- input polling / target resolve
    |      +-- Widget update / lateUpdate
    |      +-- render pass / present
    |
    +-- Input/UI event layer
    |      +-- onInputResolved
    |      +-- Widget EventDispatcher
    |
    +-- MainThreadDispatcher
    |      +-- worker completion
    |      +-- one-shot deferred mutation
    |
    +-- EditorEvents
           +-- selection / document / asset / preview
```

核心原则：

> Engine event 表达跨系统阶段，UI event 表达目标交互，task queue 表达一次性异步交接，直接调用表达局部强顺序。

## 8. `m_preRender` 和 `m_afterRender` 的具体处理建议

### 8.1 `m_preRender`

当前真实订阅者中，`Scene3DAsyncLoader` 用它在主线程消费异步结果；samples 也用它做逐帧状态更新。

建议分两步：

1. 将其语义重命名为 `onBeforeUpdate`，事件参数改为 `FrameState&`；
2. 异步 loader 迁移到 `MainThreadDispatcher` 后，samples 的逐帧逻辑应根据用途进入 Widget/Component update、Tween 或明确的 frame event。

兼容期可以保留：

```cpp
[[deprecated("Use events().onBeforeUpdate")]]
Observable<>& preRender();
```

但不要让新代码继续订阅旧名称。

### 8.2 `m_afterRender`

当前源码中未发现实际 runtime 订阅者。其触发点又位于 `commitRenderPass()` 后、`endFrame()` 前。

建议先确认需要的是哪一种语义：

- GPU 命令已编码/提交：`onAfterSubmit`；
- swap/present 调用已结束：`onFrameEnd`；
- GPU 真正执行完成：不能使用普通 frame event，应依赖 fence/semaphore。

不能把 `afterRender` 描述成“GPU 渲染完成”。在多线程渲染模式下，它最多表示主线程已经完成本帧命令组织。

### 8.3 `callAfterRender`

这是每帧清空的一次性 callback list，应该按 task queue 管理，不应迁移为长期 `Observable`。

还应注意其当前执行位置在 `Platform::endFrame()` 之后。若调用方需要在 Present 前执行，应新增名称明确的队列，而不是改变现有时序后造成隐式回归。

## 9. 渐进迁移方案

### P0：定义语义并修正帧边界 ✅

- 增加本文档并同步 `ARCHITECTURE.md`；
- 删除语义模糊的 `preRender()`、`afterRender()` 及其独立成员；
- `onFrameBegin` 在 request-render gate 通过后、Tween/Widget update 前触发；
- `onFrameEnd` 在 `Platform::endFrame()` 和当帧一次性回调完成后触发；
- 明确 Engine/UI/task 三类机制的边界；
- 新代码停止增加语义模糊的 `Observable<>` 成员。

### P1：改造底层 `Observable` ✅

- 使用单调递增整数 slot id；
- priority 降序执行，同 priority 保持连接顺序；
- 增加 move-only RAII `Connection`；
- notify 中 connect 的 observer 下一轮生效；
- notify 中 disconnect 的待执行 observer 本轮不再执行；
- 删除 UUID、`add()` 和 `remove()` 旧 API，不保留兼容层；
- 增加 `ObservableTests` 覆盖顺序、生命周期、修改订阅和嵌套 notify。

### P2：引入 `EngineEvents` ✅

- `Engine` 实例持有 `EngineEvents`；
- 当前只定义 `onFrameBegin` 和 `onFrameEnd` 两个真实生命周期边界；
- 删除旧 `preRender/afterRender` API，不保留 deprecated 转发；
- `Scene3DAsyncLoader`、samples 和其他订阅者已迁移到 `events().onFrameBegin`；
- 不预先复制 play mode、script、project 等 MorrowUI 尚不存在的事件。

### P3：建立主线程任务队列

- worker completion 通过 queue 交给主线程；
- queue post 时 request render 并唤醒等待；
- 迁移 `Scene3DAsyncLoader` 的每帧 polling observer；
- 梳理 `callAfterTouched/callAfterRender`，按语义保留或迁移到命名队列。

### P4：建立 EditorEvents

- 从 selection changed、runtime rebuilt、asset database changed 等真实跨模块通知开始；
- Editor event 由 editor host/session 拥有；
- 禁止 runtime include editor event 类型；
- panel 使用 RAII Connection，支持销毁和重建。

### P5：按证据扩展

只有出现以下需求时再扩展：

- 多个 runtime subsystem 需要同一帧阶段；
- 确有 priority 顺序契约；
- play/preview mode 生命周期稳定成型；
- scene/component 生命周期需要局部 signal；
- profiling 表明当前事件容器产生可见成本。

## 10. 测试要求

底层 `Observable` 至少覆盖：

1. 按稳定顺序派发；
2. priority 排序及同 priority 连接顺序；
3. 回调中断开自身；
4. 回调中断开尚未执行的 slot；
5. 回调中新增 slot 下一轮生效；
6. nested emit；
7. Connection move 和析构；
8. event owner 先销毁；
9. observer owner 先销毁；
10. clear 后 Connection 状态；
11. 空 callback；
12. 异常策略。

Engine 时序测试至少记录并断言：

```text
frame begin
input resolved
input dispatched
before update
tween update
widget update
widget late update
before submit
after submit
end frame
deferred callbacks
```

同时覆盖：

- 连续渲染模式；
- request-render 空闲跳帧；
- worker completion 唤醒；
- 单线程和多线程渲染；
- Engine 退出时仍有 pending task。

## 11. 风险与约束

### 11.1 不要建立万能 EventBus

字符串 topic、`std::any` payload 或全局 singleton bus 会丢失类型、所有权和层级边界，也使调用关系难以检索。MorrowUI 更适合显式的聚合事件结构。

### 11.2 不要用 priority 掩盖错误分层

如果两个回调存在强依赖，优先选择：

- 拆成不同 frame phase；
- 由共同 coordinator 直接调用；
- 建立明确的数据依赖。

priority 只适合相同阶段中的少量系统排序。

### 11.3 不要把 GPU 完成事件和 CPU 阶段事件混淆

`commitRenderPass()`、`RenderDeviceProxy::endFrame()`、swap/present 和 GPU fence 完成是不同时间点。事件名称必须说明它代表 CPU 命令提交、Present 调用还是 GPU 完成。

### 11.4 不要让事件隐藏按需渲染需求

当前 request-render gate 会跳过 `preRender` 之后的所有阶段。任何需要在空闲时也执行的工作不能仅依赖 render-only frame event。

主线程 task queue 在收到任务时必须主动 request render/wake；持续逻辑则必须明确请求下一帧，不能假设事件会自动每帧发生。

### 11.5 不要事件化 Widget 热路径

Widget 树天然表达：

- 父子关系；
- 可见性裁剪；
- stable child order；
- Component 所有权；
- update/lateUpdate 阶段。

保留直接遍历比每个节点订阅全局 event 更清晰，也更容易优化。

## 12. 最终建议

建议采用以下决策：

1. **采纳分层事件驱动思想**，用于 Engine、Editor 和异步主线程交接边界；
2. **不把 `m_preRender/m_afterRender` 视为 `events.h` 的替代品**；
3. **将 `Observable` 改造成可靠的多播事件 primitive**；
4. **建立 Engine 实例级 `EngineEvents` 聚合结构**，从少量真实阶段开始；
5. **保留 Platform、Window、Widget 和 Component 的直接协调链**；
6. **保留 UI 目标事件的独立 EventDispatcher 层**；
7. **新增 MainThreadDispatcher 处理 worker completion 和一次性任务**；
8. **Editor 使用自己的 EditorEvents，不污染 runtime**；
9. **不引入当前没有业务支撑的 ECS/play mode/script 事件**；
10. **先测试事件语义和帧时序，再迁移现有订阅者**。

最终目标不是“所有模块都通过事件通信”，而是：

```text
固定且可测试的帧阶段
    +
类型安全且生命周期可靠的系统级 Observable
    +
局部 UI 事件
    +
一次性主线程任务
    +
明确的直接调用链
```

这与 UnravelEngine 分层事件架构的核心原则一致，同时保留了 MorrowUI 当前 Widget 树和渲染管线的结构优势。
