# MorrowUI QNX 侧事件系统评估与主流引擎对比

> 分析基线:2026-11-03
> 范围:Platform 输入采集面(QNX screen / WGL GLFW)、Engine 帧循环输入阶段,
> 以及与 Godot 4 / Unity / UE5 输入架构的逐维度对比。
> 关联文档:`EVENT_DRIVEN_ARCHITECTURE_ASSESSMENT.md`(引擎内部 Observable/EngineEvents
> 分层,本文不重复)、`ARCHITECTURE.md` §5.1(输入阶段)、§11(Platform)。
>
> 复核记录(2026-11-03):按产品定位(QNX = 车机触摸屏,无鼠标/键盘硬件,
> hover 不是目标交互)复核后,原 v1 列为 P1 的"POINTER/KEYBOARD 映射缺失"
> 降级为需求驱动项(§3 P3-1);唤醒契约缺失维持 P1,且触摸-only 前提下其
> 目标方案(事件线程)由"备选"升级为目标状态(§6 R1)。

## 1. 结论

**引擎级输入架构是合理的,与三家主流引擎同构;需要补的是 QNX 的等待循环
唤醒契约,不在"轮询 vs 推送"的模型选择本身。**

| 评估项 | 结论 |
| --- | --- |
| 输入在帧首、主线程 poll → 命中解析 → 派发 | 合理,行业标准做法(三家引擎最终都在主线程、帧边界消费输入) |
| `IInputProvider` 平台无关事件抽象 | 合理,与 Godot `InputEvent` / UE Slate 事件的翻译层同构 |
| 按需渲染空闲期输入仍被 poll(gate 之前) | 合理,触摸仍能触发 `REQUESTRENDER` |
| QNX 只映射 MTOUCH 三类事件 | 合理(车机触摸为主,POINTER/KEYBOARD 无硬件来源,hover 非目标语义)。唯一缺陷是 `isUnhandledEventType` 把两类列为"已知"却不映射、也不注明意图,需注释澄清(P3 卫生项);映射本身列为需求驱动,见 §6 R2 |
| QNX 未实现 `waitForEvents`/`wakeEventLoop` | **不合理(P1)**:worker 唤醒契约失效;且触摸延迟、空闲功耗、等待节拍三者被 sleep 形态绑死,只有事件线程形态能解耦,见 §6 R1 |
| 全帧共享一个时间戳 | 可优化(P2):对点击/长按影响可忽略(≤3%),但速度类判定(惯性、缩放速率)的 Δt 被量化到帧周期,属原理性上限,建议补(机理分析见 §6 R3) |
| drain 超时 1ms 计入帧时间 | 可优化(P2):连续渲染模式每帧最多 +1ms(60fps 下约 6% 预算) |
| 是否应改为"输入线程 + 推送" | 事件消费面保持主线程、帧边界不变;把采集前置为独立事件线程是 R1 方案 B 的实现形态(Qt QPA 同款),针对触摸唤醒延迟与空闲功耗,不是通用异步化 |

一句话概括:

> MorrowUI 的输入消费模型(帧首 poll、主线程、帧边界 flush)与 Godot/Unity/UE 一致,
> 不是落后设计;真正要补的是把等待循环接上唤醒契约(waitForEvents/wakeEventLoop)
> ——对触摸-only 车机,这等价于"任务能否即时触发渲染、触摸能否即时唤醒、
> 静止画面能否零节拍"三个硬指标;POINTER/KEYBOARD 映射与触摸→鼠标仿真按产品
> 定位收敛为需求驱动项,不预先补齐。

---

## 2. MorrowUI 当前 QNX 事件系统(事实描述)

### 2.1 采集链路

```text
QNX Screen 服务
    → screen_get_event(ctx, ev, 1ms 超时循环)      QNXInputProvider::poll
    → 翻译为平台无关 TouchEvent 向量                (MTOUCH_TOUCH/MOVE/RELEASE)
    → InputEventsManager::poll 填充本帧事件         Engine 主线程
    → Platform::resolveInputTargets                 坐标统一 + 命中测试 + capture
    → Platform::dispatchEvents                      target->dispatchTouchEvent
    → 同帧进入 update / lateUpdate / 渲染提交
```

帧循环中的位置(`Engine::render`,`Engine.cpp:247-301`):

```text
updateFrameState
    → Platform::beginFrame        ← QNX 在这里 poll + resolve(输入先于一切更新)
    → dispatchEvents              ← 派发到 Widget,置 needRender → REQUESTRENDER
    → request-render gate         ← 空闲时 heartbeat + waitForEvents(1/60s) + continue
    → ... 正常渲染帧 ...
```

关键实现事实:

| 事实 | 位置 |
| --- | --- |
| drain 循环 `while (!screen_get_event(..., 1ms))`,队列空时最后一次调用阻塞 ≤1ms | `QNXInputProvider.cpp:14,74` |
| 只映射 `SCREEN_EVENT_MTOUCH_TOUCH/MOVE/RELEASE`;`POINTER`/`KEYBOARD` 落入 `default: continue` 被丢弃 | `QNXInputProvider.cpp:89-101` |
| `isUnhandledEventType` 把 POINTER/KEYBOARD 列为"已知类型"(不再打未处理日志),但实际未映射、也未注明是有意丢弃 | `QNXInputProvider.cpp:16-24` |
| 本帧全部事件共享 drain 开始时取的一个单调时钟时间戳 | `QNXInputProvider.cpp:72` |
| 坐标为 screen 全局坐标减窗口偏移,无 DPI 换算 | `QNXInputProvider.cpp:114-115` |
| 上一帧 TOUCH/MOVE 的 touchID 若本帧消失,合成 RELEASE(防丢释放安全网) | `QNXInputProvider.cpp:135-147` |
| 按需渲染空闲 skip 帧丢弃全部输入并清空追踪状态 | `QNXInputProvider.cpp:128-133` |
| 休眠恢复后丢 N 帧输入的权宜方案(注释承认无完美方案) | `QNXPlatform.h:44-47` |
| `waitForEvents` = `std::this_thread::sleep_for`,`wakeEventLoop` = 空实现(QNX 未覆盖) | `Platform.cpp:30-37` |
| WGL 侧为 `glfwWaitEventsTimeout` / `glfwPostEmptyEvent`(有真实唤醒) | `WGLPlatform.cpp:38-44` |
| `RenderingThread` 在 requestRender 时调用 `wakeEventLoop()` 唤醒平台等待 | `RenderingThread.cpp:26` |

### 2.2 设计上的优点

1. **输入先于更新**:poll 在 `beginFrame`,命中解析和派发在更新阶段之前,
   事件导致的状态修改同一帧进入渲染——与 `ARCHITECTURE.md` §5.1 契约一致。
2. **平台抽象干净**:screen API 只出现在 `QNXInputProvider`/`EGLOperationsQNX`,
   引擎其余部分只见 `TouchEvent` 向量;WGL/QNX 两侧消费逻辑完全共享。
3. **帧边界 flush 天然成立**:每帧 poll 一次、批处理一次,不存在 OS 回调里直接改
   Widget 状态的重入问题(这正是 Godot buffering / Unity 事件队列要解决的问题)。
4. **RELEASE 合成安全网**:screen 偶发丢释放事件时控件不会卡在按下态,且实现廉价。
5. **输入不走锁**:主线程独占 poll 与消费,输入路径零同步原语。

---

## 3. QNX 侧问题清单

### P1 `waitForEvents`/`wakeEventLoop` 未实现(空闲为 sleep 节拍)

QNX 未覆盖基类实现(`Platform.cpp:30-37`):

```cpp
void Platform::waitForEvents(double timeoutSeconds) {
    std::this_thread::sleep_for(...);   // QNX 实际走到这里
}
void Platform::wakeEventLoop() {
    // 空实现
}
```

两个后果:

1. **worker 唤醒失效**:异步任务 post 到 `MainThreadDispatcher` 时,
   `RenderingThread` 会调用 `wakeEventLoop()`(`RenderingThread.cpp:26`),
   QNX 上是 no-op——主线程必须等 sleep 到期(最多 16.6ms)才消费任务并渲染。
   WGL 上 `glfwPostEmptyEvent` 立即唤醒,同一契约在 QNX 上失效。
2. **触摸延迟、空闲功耗与等待节拍三者绑死**:空闲期输入处理完全由
   `waitForEvents` 上限驱动。上限钉在 ~16.6ms 保触摸响应,则静止画面仍以
   60Hz 节拍唤醒(每轮 sleep 16.6ms + drain ≤1ms,单核占空比约 6%,车机
   静态页面的功耗损失);拉长上限省电,则触摸按下到首帧响应线性变差。
   `sleep_for`/`wait_for` 形态下这是一个无解的绑定——事件到达无法打断等待,
   这正是 R1 方案 B(事件线程,到达即醒)存在的理由。

### P2-1 全帧共享时间戳

`poll()` 在 drain 开始取一次单调时钟,批内所有事件共用(`QNXInputProvider.cpp:72`)。
screen 事件本身携带硬件时间戳,丢弃后双指手势、滑动速度、双击间隔、多指先后
只能按帧粒度估计。**触摸-only 产品定位下这个问题更重要而非更不重要**——
触摸场景没有鼠标移动的连续采样,手势全靠事件时序。UI 点击语义不受影响,
属精度损失而非错误。

### P2-2 drain 超时 1ms 计入帧时间

`kScreenEventWaitTimeNs = 1ms` 的语义是"排空队列后最多再等 1ms"。连续渲染模式下,
无输入的帧也会为最后一次 `screen_get_event` 付出 ≤1ms 阻塞,直接计入帧时间
(60fps 预算 16.6ms 的约 6%)。排空语义应当用 0 超时,阻塞语义属于
`waitForEvents`(见 R1/R4)。

### P3-1 POINTER / KEYBOARD 未映射(平台差异,需求驱动)

事实(不变):`isUnhandledEventType` 把 `SCREEN_EVENT_POINTER`、
`SCREEN_EVENT_KEYBOARD` 与 MTOUCH 三类一起列为"已知类型"
(`QNXInputProvider.cpp:16-24`),但 switch 只映射 MTOUCH,这两类走
`default: continue`(`QNXInputProvider.cpp:99-101`)被丢弃。

评估(按"QNX = 车机触摸屏"的产品定位,v1 的 P1 判定据此降级):

- 目标硬件无鼠标/键盘/滚轮,hover 不是目标交互——补映射是投机工作,违反
  `ARCHITECTURE.md` §3.4 需求驱动原则,**暂不实施,不排期**;
- 引擎语义面(`TouchEvent.deviceType=MOUSE`、键盘焦点、
  `EngineEvents::onRawKeyboardInput`、`MRTextEdit`/`MRLineEdit` 键盘输入)
  继续由 WGL 侧完整承载。平台能力差异是正常现象,与 Godot/Unity 在移动端
  不承诺物理键鼠同一性质,**不作为缺陷跟踪**;
- 唯一值得做的是注释级澄清(两行,不加日志、不改行为——放开了反而会因
  事件刷屏):在 `isUnhandledEventType` 处注明"POINTER/KEYBOARD 在触摸-only
  目标上已知且有意丢弃,映射为需求驱动项(见 road_map)",消除"声称已知、
  实际丢弃"对后续维护者的误导。

重新启动映射工作的触发条件(出现任一即可重估):

- QNX 目标需要软键盘文本输入(导航目的地、用户名等),且输入经 screen 键盘
  事件到达;
- 目标台架/量产件带旋钮、触摸板等 pointer 类设备(部分座舱控制器以
  `SCREEN_EVENT_POINTER` 上报);
- 开发调试需要在 QNX 台架接 USB 鼠标操作 UI。

触发后的实施要点备查:`SCREEN_EVENT_POINTER` → `TouchEvent{deviceType=MOUSE}`
(POSITION/BUTTONS/滚轮增量,注意与 MTOUCH 双报去重);
`SCREEN_EVENT_KEYBOARD` → `KEY_DOWN`/`CHARACTER`(key sym / unicode 属性翻译);
`isUnhandledEventType` 与实际映射表重新对齐。

### P3-2 其他(记录在案,暂不处理)

- `SCREEN_EVENT_PROPERTY` 会截断当轮 drain(剩余事件留到下一帧):事件不丢,
  只延迟一帧,单窗口场景影响可忽略;
- 休眠恢复 skip 帧丢输入:已有注释承认是权宜方案,等待真实需求驱动(如恢复期间
  改为丢弃"恢复前缓冲"而非按帧数丢弃);
- 窗口偏移无 DPI/缩放概念:与"单窗口、车机固定屏"的现状匹配,多窗口/分辨率
  适配出现时一并处理(见 `OPTIMIZATION_DIRECTIONS.md` 第 15 条)。

---

## 4. 主流引擎输入架构对照

### 4.1 共同范式

Godot、Unity、UE 三家的输入栈尽管实现差异很大,但范式完全一致:

```text
OS 事件(回调 / 消息泵)
    → 平台层翻译为平台无关事件对象(InputEvent / InputEvent 缓冲 / Slate 事件)
    → 引擎内队列或状态缓冲
    → 在确定的帧阶段、由主线程 flush / 消费        ← 消费面永远是帧边界
    → 命中测试 / 焦点 / 路由(隧道或冒泡)
    → 更新输入状态(action/axis/pressed 集合)
    → 游戏逻辑在 update 读取
```

关键点:**"事件驱动"只发生在 OS → 引擎缓冲这一段;引擎消费输入永远是主线程、
帧边界、批量的**。没有一家在 OS 回调线程里直接改 UI/游戏状态。MorrowUI 的
"帧首 poll + 帧内派发"与这个范式等价——poll 只是 flush 的形式之一。

### 4.2 Godot 4

```text
DisplayServer::process_events()      平台层泵事件(X11/Wayland/Win32/…)
    → 翻译为 InputEvent*              (InputEventKey/MouseMotion/ScreenTouch…)
    → Input 单例缓冲                  (buffered events)
    → Main::iteration() 开头 Input::flush_buffered_events()
    → SceneTree 逐节点 _input / _gui_input(视口命中 + 焦点)
```

- **输入缓冲**:帧处理过程中到达的 OS 事件被缓冲到下一帧 flush,防止重入和
  半帧状态(`Input.flush_buffered_events()` / `is_input_flushed()`)。
- **运动聚合**:`Input.use_accumulated_input` 把一帧内多条鼠标移动合并为一条,
  减少事件量、保证单帧单处理(默认开启)。
- **触摸↔鼠标双向仿真**:`emulate_mouse_from_touch`(默认开)让**桌面触摸板**
  场景的 UI 代码看到一致指针语义;`emulate_touch_from_mouse` 反向仿真。
  注意其定位是桌面平台(有 hover 语义),与车机触摸屏不同源——MorrowUI
  不因此需要在 QNX 上补仿真。
- **空闲**:低处理器模式(`OS.low_processor_usage_mode`,桌面工具类应用常用)
  在帧间 sleep 固定微秒数,但每轮仍泵事件;Godot 的平台泵本身是事件阻塞的
  (如 Win32 GetMessage),事件到达即醒。
- **时间戳**:`InputEvent` 不暴露事件级时间戳(类文档无该字段,社区对高分辨率
  输入时间戳有明确需求缺口)。Godot 的时间模型是帧对齐的——对桌面鼠标场景成立,
  对触摸速度类手势是已知短板。

对照 MorrowUI:消费模型一致;Godot 多出的两件事是**聚合**与**触摸→鼠标仿真**
(桌面场景专属),以及平台泵的真事件阻塞(对应 R1)。

### 4.3 Unity

```text
平台原生层泵消息,更新设备状态
    → 产生 InputEvent(带原生时间戳)入队
    → InputSystem.Update()(按 Update Mode 在确定的玩家循环阶段执行)
    → 事件应用到设备状态机(双缓冲:previous/current state)
    → Action 解析(callbacks)与 UI(EventSystem / UI Toolkit 面板事件队列)
```

- **事件队列 + 设备状态机**:新 Input System 把原始事件(StateEvent /
  DeltaStateEvent)按模式(Dynamic / Fixed / Manual / BeforeRender)flush 到
  设备双缓冲状态;动作与轮询读状态而不是直接读事件。事件比帧密时合并,稀疏时
  状态保持——这是"事件流 → 帧对齐状态"的完整实现。
- **Update Mode 可选**:引擎驱动(Dynamic/Fixed)、手动 `InputSystem.Update()`
  (自定帧循环场景,与 MorrowUI "引擎统一 poll" 的形态同源)。
- **时间戳**:每个事件携带原生时间戳;XR 场景额外有 BeforeRender 二次刷新。
- **空闲**:无按需渲染门,`targetFrameRate` / vSync 节流;Player Loop 严格分阶段。
- **平台能力差异的正常化先例**:Unity 在移动端同样不承诺物理键鼠,hover 语义
  只在有指针的平台上成立——引擎能力与平台承诺分离,MorrowUI 的 WGL/QNX 差异
  属同一性质。

对照 MorrowUI:MorrowUI 没有独立"设备状态层",`InputEventsManager` 直接暴露本帧
事件向量 + 若干查询(`GetTouchIDCount` 等),粒度更细、更轻——对保留式 Widget
树够用;若未来出现"帧中段查询输入状态"的需求,再考虑引入设备状态快照。

### 4.4 Unreal Engine 5

```text
游戏线程每 tick FSlateApplication::PumpMessages()
    → 平台消息翻译(FWindowsApplication::ProcessMessage)
    → FSlateApplication::OnMouseButtonDown 等
    → 路由:preview 事件自根向下(隧道)→ 目标事件沿路径向上(冒泡)
      键盘 → 焦点 widget;指针 → hittest grid 命中路径
    → PlayerController::ProcessInput(TG_PrePhysics)
      action/axis 绑定,一帧内多次 axis 采样按帧聚合
```

- **路由模型**:Slate 的 preview(隧道)+ bubble 与 Web/Qt 同族,比 MorrowUI
  "命中即派发 + capture"多一层显式隧道阶段;MorrowUI 的 hover ENTER/LEAVE +
  pointer capture + 键盘焦点已覆盖 Slate 路由的核心语义,缺的是"父级拦截"
  (preview)——当前无业务需求,不引入。
- **聚合**:一帧内多次按键/轴事件在 `PlayerInput` 内合并、轴值求平均,
  保证逻辑一帧一语义。
- **线程**:默认消息泵与 Slate 路由都在游戏线程;UE5 有实验性异步输入
  (采集与路由解耦、路由仍限游戏线程),尚未成为默认——侧面印证
  "输入采集异步化"在大型引擎里也只是演进项而非基线。
- **空闲**:游戏模式无按需渲染门,t.MaxFPS / vSync 节流;失去焦点时消息泵
  阻塞在 OS。

对照 MorrowUI:MorrowUI 的 `findTopmostInteractiveWidget` 递归命中 + clip 传播
与 Slate hittest grid 是同一问题的两种实现(树递归 vs 网格加速);当前规模
树递归成本可接受,widget 数量上万时再考虑 hittest 缓存(按需抽象原则)。

### 4.5 QNX 生态参照:Qt QPA(QNX 平台插件)

Qt 在 QNX 上的官方平台插件与 MorrowUI 面对同一套 screen API,采用的是
**独立事件线程** 模型:

```text
QQnxScreenEventThread(独立线程)
    → screen_get_event(ctx, ev, -1)   无限阻塞(QNX 官方文档推荐姿势)
    → 翻译为 Qt 平台事件,QWindowSystemInterface 投递
    → 经 dispatch 唤醒 GUI 线程(pulse / SCREEN_EVENT_USER 机制)
GUI 线程
    → 事件循环被唤醒后批量处理队列
```

要点:采集线程阻塞在 `-1` 超时上,退出和唤醒靠 `screen_register_event` +
`SCREEN_EVENT_USER` 注入用户事件解除阻塞;**消费仍在 GUI 主线程、帧边界**。
QNX Screen 官方文档同样把"专用事件线程 + 无限阻塞"列为推荐用法。

这是本文 R1 方案 B 的直接依据。对触摸-only 车机,该形态的价值不是"引擎
先进性",而是两个可测的产品指标:**触摸按下→首帧响应**(空闲画面从
≤16.6ms+drain 降到事件到达即处理)与**静止画面功耗**(主线程零节拍)。

### 4.6 对比表

| 维度 | MorrowUI(QNX 侧现状) | Godot 4 | Unity(新 Input System) | UE5 |
| --- | --- | --- | --- | --- |
| OS 采集 | 主线程 poll,1ms 超时 drain | 平台泵(DisplayServer) | 原生层泵 → 事件队列 | 游戏线程 PumpMessages |
| 平台无关抽象 | `TouchEvent` 向量(`IInputProvider`) | `InputEvent*` 类族 | `InputEvent` + 设备布局 | Slate 事件 + `FInput` |
| flush 时机 | `beginFrame`(帧首)✅ | iteration 开头 flush | 按 Update Mode 的循环阶段 | tick 首,Slate 路由 → TG_PrePhysics |
| 时间戳 | ❌ 全帧共享 poll 时刻 | ❌ 未暴露(帧聚合模型) | ✅ 原生时间戳 | ⚠️ 帧对齐为主 |
| 运动聚合 | 无(事件本来就按帧批) | ✅ mouse motion 累积 | ✅ 设备状态机合并 | ✅ axis 帧内平均 |
| 触摸↔鼠标仿真 | ❌(车机无 hover 语义,不需要) | ✅(桌面场景) | ✅ | 平台相关 |
| 鼠标/键盘(QNX) | ❌ 不映射(平台现实,需求驱动) | ✅ | ✅ | ✅ |
| hover/capture/焦点 | ✅(引擎层,由 WGL 承载) | ✅ | ✅ | ✅(preview+bubble) |
| 空闲/唤醒 | ❌ sleep 60Hz 节拍,无 wake | 低处理器模式 sleep,事件到即醒 | 帧率节流,无按需门 | 消息泵阻塞,t.MaxFPS 节流 |
| 输入线程 | 主线程 ✅ | 主线程(泵在平台层) | 主线程 flush | 游戏线程(异步输入为实验特性) |

结论:MorrowUI 在"消费模型、线程归属、flush 时机"三个根本维度上与三家一致;
表中缺口分两类:**时间戳与唤醒**是值得补的平台实现项;**QNX 鼠标键盘与触摸
仿真**按车机触摸为主的产品定位属"引擎能力存在、平台不承诺",与 Godot/Unity
在移动端的处理一致,不作为缺陷跟踪。

---

## 5. 逐维度评估:当前设计的选择是否合理

### 5.1 轮询模型(合理,不建议改推送)

"每帧 poll 一次"不是偷懒,而是与保留式帧模型严格对齐的选择:

- Widget 更新、布局、合批都是帧同步的;输入在帧首一次性 flush 保证
  "一帧一输入快照",派发期间树状态稳定;
- 推送式(事件到达即派发)会把重入问题引入 Widget 树——Godot 缓冲、Unity
  队列存在的全部理由,就是把这些事件推迟到帧边界;
- 三家引擎的"事件驱动"只到引擎缓冲为止;MorrowUI 的 poll 是同一语义的低配实现。

若产品对"静止画面触摸响应"或"空闲功耗"提出硬指标,演进形态是**采集线程 +
无锁队列 + 帧首 drain**(Qt QPA 形态,即 R1 方案 B),消费面不变。

### 5.2 帧边界 flush(已具备)

MorrowUI 每帧 poll 一次天然满足;skip 帧丢弃输入是唯一例外(见 P3-2),
语义为"不渲染的帧不消费输入",与按需渲染一致,可接受。

### 5.3 输入抽象的完整性(方向正确,平台差异收敛在翻译层)

`TouchEvent` 是"触摸为主、鼠标为辅"的统一语义:`deviceType` 区分鼠标/触摸,
`MOUSE` 分支已有 hover/wheel/capture。引擎语义面不缺;QNX 侧按触摸-only
产品定位只映射 MTOUCH,翻译层是否扩展由 R2 的触发条件决定。平台差异消灭在
翻译层、上层只见统一事件的做法与 Godot/Unity 一致。

### 5.4 时间戳与聚合(轻量取舍,补时间戳即可)

Godot/Unity/UE 都保留事件级时间戳并做帧内聚合。MorrowUI 按帧批处理已隐含
"帧粒度聚合",一帧多事件不重复派发,聚合需求大部分被帧模型吸收;
剩余缺口只是时间戳本身(R3),不必引入设备状态机。触摸-only 场景
(双指缩放、滑动速度)是该缺口的主要受益方。

### 5.5 空闲与唤醒(最大差距,必补)

按需渲染是 MorrowUI 的核心卖点之一,而 QNX 侧 `waitForEvents` 的 sleep 实现
使这个卖点在目标平台上打折:worker 任务不能即时触发渲染;触摸延迟、空闲功耗、
等待节拍三者被绑死(见 §3 P1)。WGL 侧 `glfwWaitEventsTimeout`/
`glfwPostEmptyEvent` 已证明该契约的价值,QNX 缺的是把契约接到 OS 原语上(R1)。
注意收益要拆开:condvar 形态只解决 worker 唤醒;触摸即唤醒与零节拍功耗需要
事件线程形态(§6 R1 方案 B)。

### 5.6 命中测试与路由(规模内合理)

树递归命中 + clip 传播 + capture + hover 合成,语义覆盖 Slate/Unity UIToolkit
的核心;preview(隧道)路由在 UI 引擎里对应"父级优先拦截",当前控件集无此需求。
hittest 网格化属于量级驱动优化,列入观察不列入实施。

---

## 6. 建议(R1-R5)

按收益/成本排序;R1 建议尽快,其余按需求节奏。R2/R5 已按产品定位降级为
需求驱动项,不排期。

### R1:实现 QNX `waitForEvents` / `wakeEventLoop`(P1)

**做什么**:QNXPlatform 覆盖两个虚函数,把 Engine 空闲等待和
`RenderingThread::requestRender` 的唤醒契约接到 OS 原语。按触摸-only 的
产品定位,两个方案交付的不是同一组东西,收益要拆开算:

| 收益点 | 方案 A(condvar) | 方案 B(事件线程) |
| --- | --- | --- |
| worker 任务即时触发渲染(关闭与 WGL 的契约缺口) | ✅ | ✅ |
| 触摸事件即时唤醒(空闲画面触摸响应 ms 级) | ❌ 等待节拍不变,触摸延迟与现状相同 | ✅ 事件到达即醒 |
| 静止画面功耗(消除 60Hz 空闲节拍) | ❌ 节拍必须为触摸延迟保留 | ✅ 主线程可无限阻塞,近零节拍 |

**方案 A(condvar,推荐作为第一步)**:`waitForEvents` 改为
`condition_variable::wait_for(timeout)`,`wakeEventLoop` 改为 `notify_one()`,
约 20 行。关闭 worker 唤醒缺口,与 WGL 契约对齐。关键认识:把
`sleep_for` 换成 `wait_for` 不改变空闲节拍——触摸延迟要求把等待上限钉在
~16.6ms,方案 A 无法同时优化触摸延迟与功耗,它交付的只有第一项。

**方案 B(QNX 官方推荐形态,Qt QPA 同款,触摸-only 场景的目标状态)**:
独立 screen 事件线程阻塞 `screen_get_event(-1)`,翻译后经 SPSC 队列投递,
主线程帧首 drain;输入到达或 worker 任务经 condvar 唤醒主线程;线程退出用
`screen_register_event` + `SCREEN_EVENT_USER` 解除阻塞。
`QNXInputProvider::poll` 从"直接排空 screen"改为"排空本地队列",
消费面(帧首、主线程、批量)不变。触摸按下→首帧响应从 ≤16.6ms+drain
降到事件到达即处理;静止画面主线程近零唤醒。
代价:输入线程生命周期、与主线程 drain 的交接协议、skipEvents/休眠恢复
路径重验。

**实施建议**:先 A 后 B。A 独立可交付,先验证唤醒契约;B 在 A 的契约上
只换等待源与事件来源。若产品对触摸响应/功耗没有硬指标,B 可长期不启动。

**验收**:A——worker post → 渲染帧延迟从 ≤16.6ms 降至毫秒级;
B——另加空闲画面触摸按下→首帧响应、静止画面 CPU 占空比(目标:从 ~6%
单核到近零)。

**风险**:heartbeat 与 objectSnapshot 命令轮询依赖循环节拍(方案 B 需为其
保留低频兜底唤醒或改为唤醒驱动);skipEvents 休眠恢复路径与事件线程的
组合需重验。

### R2:`SCREEN_EVENT_POINTER` / `SCREEN_EVENT_KEYBOARD` 映射(暂缓,需求驱动)

按"QNX = 车机触摸屏,无鼠标/键盘硬件"的产品定位,本项**不排期**;触发
条件见 §3 P3-1,触发后的实施要点已在彼处备查。

当前唯一动作:`QNXInputProvider.cpp` 的 `isUnhandledEventType` 处加两行注释,
写明"POINTER/KEYBOARD 在触摸-only 目标上已知且有意丢弃,映射为需求驱动项
(见 road_map/QNX_EVENT_SYSTEM_COMPARISON.md §3 P3-1)"。

### R3:事件级时间戳(P2)

**做什么**:每个事件读 `SCREEN_PROPERTY_TIMESTAMP`,写入 `TouchEvent.touchTime`;
无时间戳的类型回退到单调时钟。

**为什么(误差机理)**:当前 `poll()` 把 drain 开始时刻作为整批事件的
`touchTime`(`QNXInputProvider.cpp:72`)。触摸事件的真实时刻是触摸控制器采样
手指的时刻,事件在 screen 队列里排队、到引擎帧首才被取出——因此每个事件带
0~一个帧周期的未知滞后,且同批事件之间无法区分先后。

时间敏感度要按判定类型拆开,不能一概而论:

- **阈值类(点击、双击、长按)**:只关心"两个时刻的间隔是否超过阈值",
  现有误差(≤1-2 帧,约 10-30ms)相对 300-500ms 阈值占比 1-3%,感知边缘,
  **现状可接受**。引擎当前唯一消费者即属此类:
  `Interaction::checkLongPress`(`Interaction.cpp:58,150`)用
  `now - m_pressStartTime >= longPressDelay` 判长按,`m_pressStartTime` 来自
  共享时间戳——长按只会固定偏晚最多约一个帧周期,500ms 阈值下 ≤3%。
  **R3 不是为这类而做。**
- **速度类(一切带 Δx/Δt 的量)**:滑动速度、列表惯性、双指缩放/旋转速率。
  共享时间戳把 Δt 量化到帧周期(0 / 16.6 / 33.2ms),误差性质完全不同:

  1. **批内 Δt=0(必现,与相位无关)**:触摸屏约 8ms 上报一次、引擎 60fps
     时,多数 frame drain 收到 2 个 MOVE,二者 `touchTime` 相同——事件级速度
     公式除零,样本只能丢弃。高速滑动时事件最密,丢的恰是最关键的样本。
  2. **相位漂移(最坏 ±一个采样周期)**:帧对帧估计只能用"帧末事件位置差 ÷
     帧周期",但两次帧末采样的真实时刻差不等于帧周期——帧节拍与触摸采样节拍
     在真实设备上从不同步(面板频率非精确整数倍、vsync 抖动、按需渲染下帧间隔
     不均匀),偏差在 0~±8ms 间漂移。数值例:真实速度 2 px/ms 的匀速滑动,
     采样相位偏早时(Δt_true=13ms,位移 26px)估出 26/16.6≈1.57 px/ms
     (低估 22%);相位偏晚时(Δt_true=22ms,位移 44px)估出 44/16.6≈2.65
     px/ms(高估 33%)——同一物理速度,两次测量相差 1.7 倍。
  3. **v² 放大**:惯性滚动距离 ∝ v²,速度估差 1.7 倍 → 滚动距离差 ~2.9 倍。
     用户感受:"同样的快滑,列表有时飞出去、有时滑一半就停"。这是相位相关的
     系统性误差,不是随机噪声,调阈值救不回来。

事件级时间戳把可用的速度估计从"帧对帧近似"升级为"事件窗口回归"(样本数
翻倍以上、无相位依赖)——这正是 Android `VelocityTracker` 的做法。

**行业佐证**:Android `VelocityTracker`(所有 Android fling/缩放手势的底层)
对每个事件用 `MotionEvent.getEventTime()` 做时间窗内加权最小二乘;Qt
`QInputEvent::timestamp()`、Unity Input System 事件均携带原生时间戳。桌面
鼠标系引擎(Godot 帧聚合、UE 帧对齐)不暴露事件时间戳,是因为桌面指针交互
对速度精度需求低——反过来说明**触摸场景才需要事件级时间**。

**成本**:每事件多一次 `screen_get_event_property_iv` 调用——现有代码对每个
事件已经用同样方式读 `SCREEN_PROPERTY_TOUCH_ID`、`SCREEN_PROPERTY_POSITION`
两次,模式不变;速度计算只用 Δt,screen 时钟与引擎单调钟的域差无关紧要
(同一事件流内来源一致即可)。约 5 行改动。

**验收**:匀速滑动回放下,事件级速度估计与输入相位无关(同一速度多次测量
方差收敛);双指事件的相对时序与硬件一致;长按触发时刻不回归。

### R4:drain 超时改 0(P2)

**做什么**:`kScreenEventWaitTimeNs` 改为 0(纯排空);阻塞语义交 R1 的
`waitForEvents`/事件线程承担。

**为什么**:消除连续渲染模式每帧 ≤1ms 的帧时间污染;语义也更正确——
"poll" 只排空,"wait" 才等待。

**验收**:连续渲染模式 60fps 帧时间分布(P99)对比;输入密集场景无丢事件。

### R5:触摸→鼠标仿真开关(暂缓,需求驱动)

hover 语义的前提是"指针悬浮",车机触摸屏不存在该交互,本项**不排期**。
Godot `emulate_mouse_from_touch` 的定位是桌面触摸板场景(桌面 UI 有 hover
语义),与车机场景不同源。仅当产品出现 kiosk/POS 类触摸桌面形态时重估。

### 明确不做的事

- **不把输入消费移出主线程**:任何方案下事件消费都在帧边界、主线程、批量;
  R1 方案 B 的独立采集线程只是"采集源前置"(Qt QPA 同款),不改变消费模型;
- **不在 QNX 上映射 POINTER/KEYBOARD、不做触摸→鼠标仿真**:车机触摸为主、
  无 hover 语义,映射与仿真均为投机工作,违反需求驱动原则(触发条件见
  §3 P3-1);
- **不改引擎内事件分层**(`Observable`/`EngineEvents`/`MainThreadDispatcher`
  已由 `EVENT_DRIVEN_ARCHITECTURE_ASSESSMENT.md` 定案,本文不推翻);
- **不引入设备状态机 / action 绑定层**:当前 `InputEventsManager` 的事件向量 +
  查询足够,Unity 式状态机是 ECS 规模的方案,违反按需抽象原则;
- **不引入 preview(隧道)路由**:无父级拦截需求,Widget 树的直接遍历
  (`findTopmostInteractiveWidget`)保持简单。

---

## 7. 参考资料

- QNX Screen 官方文档:
  [Screen Event Handling(推荐专用线程 + 无限阻塞)](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.screen/topic/manual/cscreen_event-handling.html)、
  [screen_get_event() 参考](https://qnx.com/developers/docs/7.1/com.qnx.doc.screen/topic/screen_get_event.html)、
  [Screen 事件类型(含 SCREEN_EVENT_IDLE)](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.screen/topic/group__screen__events_1Screen_Event_Types.html)
- Qt QNX QPA 实现:
  [qqnxscreeneventthread.cpp(独立事件线程 + SCREEN_EVENT_USER 唤醒)](https://github.com/mburakov/qt5/blob/master/qtbase/src/plugins/platforms/qnx/qqnxscreeneventthread.cpp)
- Godot 4:
  [Input 类文档(flush_buffered_events / use_accumulated_input / 仿真开关)](https://docs.godotengine.org/en/stable/classes/class_input.html)、
  [Main::iteration 输入 flush 流程解读](https://gamefromscratch.com/godot-engine-tutorial-part-3-program-lifecycle-and-input-handling)、
  [输入累积行为讨论](https://github.com/godotengine/godot/issues/55037)
- Unity:
  [Input System Settings(Update Mode: Dynamic/Fixed/Manual)](https://docs.unity3d.com/Packages/com.unity.inputsystem@1.4/manual/Settings.html)、
  [创建与排队事件(下一帧处理)](https://docs.unity3d.com/Packages/com.unity.inputsystem@1.20/manual/create-events.html)、
  [Input Events 与设备状态缓冲](https://docs.unity.cn/Packages/com.unity.inputsystem@1.0/manual/Events.html)、
  [InputSystem 源码仓库](https://github.com/Unity-Technologies/InputSystem)
- Unreal Engine:
  [Understanding Unreal's Input Stack(输入栈全流程)](https://www.google.com/goto?url=CAESZgHrOzAVyGx6BwwI-0y14VwdsX8tq8__y6p7KUjtrW68d_1BCkoTn_DrOax-IMWuwiTaPPU0ez0Os1W1SMGOljhOHQHBy70cpTOClhQ6gkpwyDMYFzf43_3f-hWNYAQa7veugvoGNQ)、
  [Slate 输入拦截与路由(hittest grid / preprocessors)](https://dev.epicgames.com/community/learning/knowledge-base/LJjB/unreal-engine-slate-intercepting-input-events-with-preprocessors)、
  [UE 5.4 Game Input for Windows 实验插件说明](https://forums.unrealengine.com/t/tutorial-game-input-for-windows-experimental-release-notes/1763696)
- 事件时间戳与速度估计先例:
  [Android VelocityTracker(基于逐事件 eventTime 的速度估计)](https://developer.android.com/reference/android/view/VelocityTracker)、
  [AOSP VelocityTracker.cpp 实现(时间窗内加权最小二乘)](https://android.googlesource.com/platform/frameworks/native/+/refs/heads/main/libs/input/VelocityTracker.cpp)、
  [Qt QInputEvent::timestamp()](https://doc.qt.io/qt-6/qinputevent.html)、
  [Godot InputEvent(无时间戳字段,帧对齐模型)](https://docs.godotengine.org/en/stable/classes/class_inputevent.html)
