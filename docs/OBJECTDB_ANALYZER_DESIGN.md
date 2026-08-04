# MorrowUI ObjectDB 分析器设计方案

> 状态：轻量版已实施
> 目标：为 MorrowUI 引入类似 Godot ObjectDB 的运行时对象登记、快照和差异分析能力，并支持通过命令触发 snapshot。

当前实现严格限定为 CPU 对象生命周期快照，不包含 `ResourceRegistryDiagnostics`、
`RenderMemoryDiagnostics`、recycle pool、pending frame/fence 或 GPU handle 统计。
这些渲染指标如后续重新引入，应属于独立的 RenderDiagnostics，而不是 ObjectRegistry。

## 1. 结论摘要

MorrowUI **可以引入 ObjectDB 分析器**，但不建议直接照搬 Godot 的 `Object`/`ObjectDB`
体系。

Godot 的 ObjectDB 适合它自身的对象模型：

- 大多数引擎对象继承统一的 `Object` 基类；
- 每个对象拥有稳定的 `ObjectID`；
- 对象创建时注册到全局 ObjectDB；
- 析构时从 ObjectDB 移除；
- ObjectDB 可以枚举当前存活对象、按类型统计并生成快照；
- `Node` 额外维护场景树关系，可以识别 orphan nodes；
- `RefCounted` 另外提供 native refs、ObjectDB refs、total refs 和 cycle 信息。

MorrowUI 当前主要使用：

- `std::shared_ptr<Widget>`；
- `std::shared_ptr<Component>`；
- `std::shared_ptr<Texture>`、`Material`、`VertexArray` 等资源；
- `Widget` 只有 `enable_shared_from_this`，没有统一的非模板对象基类；
- `Component` 也没有统一的运行时类型注册中心；
- Widget 的父子关系由 `m_parent` 和 `m_children` 维护；
- `Texture`、VBO、UBO、SSBO 的 GPU handle 由 `ResourceRegistry` 管理。

当前采用：

> **轻量级 `ObjectRegistry` + 类型统计 + 生命周期快照 + Widget 树快照**

而不是第一阶段引入完整的 Godot 风格反射系统。

---

## 2. Godot ObjectDB 是怎么工作的

### 2.1 统一对象基类

Godot 的 `Object` 是大多数引擎对象的共同基类。对象可以通过统一接口获得：

- class name；
- instance id；
- property/method/signal 元信息；
-对象有效性检查；
-对象数据库查询能力。

对象类型通常通过 `GDCLASS` 等宏声明，并通过 `ClassDB` 注册运行时类型。
这使得 ObjectDB 不需要了解每个具体派生类的头文件，只需要保存 `Object *`
以及统一的类型和实例信息。

### 2.2 创建和销毁时注册

Object 的构造/初始化阶段会登记到 ObjectDB，析构阶段移除登记。
ObjectDB 的核心职责不是“拥有”对象，而是维护一个**非拥有型的全局索引**：

```text
ObjectID -> Object*
```

因此 ObjectDB 不会改变对象的所有权，不会因为登记而延长对象生命周期。
它只用于调试、查找和统计。

这个原则对 MorrowUI 很重要：ObjectDB 不能保存 `shared_ptr`，否则会把诊断工具
变成新的保活根，直接掩盖真实泄漏。

### 2.3 ObjectID

Godot 为每个对象分配稳定的实例 ID。ID 的作用是：

- 快照中标识对象；
- 记录对象之间的引用；
- 在对象地址变化时仍能保持快照可比性；
- 远程调试器查询具体对象；
- 判断某个对象是否仍然有效。

MorrowUI 当前 Widget 已有 UUID，但其他对象没有统一 ID。因此建议新增独立的：

```cpp
using DebugObjectId = uint64_t;
```

该 ID 不取代业务 UUID，也不暴露为业务 API。

### 2.4 对象枚举和 orphan nodes

Godot 可以枚举 ObjectDB 中所有对象，再结合 `Node` 的父子关系判断：

```text
ObjectDB 中存在
但不在 SceneTree 根节点下
```

这类 Node 会被报告为 orphan node。

需要注意：orphan node 不一定就是泄漏。对象可能暂时处于：

- 异步加载中；
- 尚未挂载到场景树；
- 从一个父节点移除、等待重新挂载；
- 测试代码临时持有。

只有在多个快照中持续存在并且数量增长，才应该视为泄漏信号。

### 2.5 ObjectDB Profiler 和 Snapshot

Godot 4.6 的 ObjectDB Profiler 支持：

- 获取当前 ObjectDB snapshot；
- 查看按类型汇总的对象；
- 查看对象列表；
- 查看节点树；
- 查看 orphan nodes；
- 比较两个 snapshot；
- 对 `RefCounted` 显示 native refs、ObjectDB refs、total refs 和 cycle。

Godot 文档明确说明，ObjectDB Profiler 主要覆盖 ObjectDB 中的引擎对象，
**不会覆盖所有引擎内存或外部库内存**。因此它不能代替 Windows heap profiler、
GPU/显存分析器或驱动工具。

---

## 3. MorrowUI 当前模型与 Godot 的差异

| 能力 | Godot | MorrowUI 当前状态 |
|---|---|---|
| 统一对象基类 | `Object` | 没有统一基类 |
| 运行时类型名 | `ClassDB` | Widget 有 `m_widgetType`，其他类型不统一 |
| 全局对象索引 | ObjectDB | 没有 |
| 稳定实例 ID | ObjectID | Widget 有 UUID，其他对象没有 |
| 对象所有权 | Object 手动释放、RefCounted 引用计数 | 主要是 `shared_ptr` |
| 场景树 | Node/SceneTree | Widget `m_parent/m_children` |
| orphan 检测 | 原生支持 | 可以针对 Widget 实现 |
| GPU 资源索引 | 由渲染系统管理 | 已有 `ResourceRegistry` |
| 运行时快照 | ObjectDB Profiler | 当前只有内存 heartbeat |
| 远程调试命令 | EngineDebugger/Editor 通道 | 暂无统一命令通道 |

最重要的差异是所有权模型：

```text
MorrowUI 的对象是否存活，主要由 shared_ptr 引用计数决定；
ObjectRegistry 只能观察 weak_ptr 或裸指针，绝不能保存 shared_ptr。
```

因此 MorrowUI 的 ObjectDB 第一阶段应该定位为：

> **live object census（存活对象盘点）和生命周期诊断器**

而不是完整的引用图分析器。

---

## 4. 推荐总体架构

```text
对象创建/销毁
      │
      ▼
ObjectRegistry（非拥有型索引）
      │
      ├── ObjectRecord
      ├── TypeRegistry
      ├── WidgetTreeProvider
      └── SnapshotService
                    │
                    ├── JSON snapshot 文件
                    ├── diff 文件
                    └── 命令响应

命令入口
      │
      ├── CLI 启动参数（启动时 snapshot）
      ├── 文件 mailbox（开发机最简单）
      ├── Windows named pipe
      ├── QNX/Unix domain socket
      └── 后续远程 Debugger 协议
```

### 4.1 ObjectRegistry

建议文件：

```text
src/debug/ObjectRegistry.h
src/debug/ObjectRegistry.cpp
```

核心接口建议：

```cpp
class ObjectRegistry {
public:
    static ObjectRegistry& getInstance();

    DebugObjectId registerObject(
        void* object,
        std::string_view typeName,
        ObjectCategory category,
        ObjectDebugInfoProvider provider = {});

    void unregisterObject(void* object);

    ObjectRecord getRecord(DebugObjectId id) const;
    std::vector<ObjectRecord> enumerate() const;

    ObjectSnapshot snapshot(const SnapshotOptions& options = {}) const;
    bool writeSnapshot(const std::filesystem::path& path,
                       const SnapshotOptions& options = {}) const;
};
```

`registerObject()` 返回的 ID 应该在对象整个生命周期内稳定。
`unregisterObject()` 必须在对象析构开始阶段执行，避免快照中出现已经无效的裸指针。

### 4.2 ObjectRecord

第一阶段不要记录裸指针地址作为主要关联键。建议记录：

```cpp
struct ObjectRecord {
    DebugObjectId id = 0;
    std::string typeName;
    ObjectCategory category;
    void* address = nullptr;              // 仅进程内即时诊断使用，不写入持久快照
    uint64_t createdAtFrame = 0;
    double createdAtSeconds = 0.0;
    std::string debugName;
    std::string ownerId;
    std::string parentId;
    uint32_t flags = 0;
    size_t estimatedBytes = 0;
};
```

持久化 snapshot 中不写 `address`，因为地址：

- 每次运行不同；
- ASLR 下不稳定；
- 不能作为快照 diff 的对象身份；
- 对外暴露可能带来不必要的安全问题。

### 4.3 对象分类

推荐第一阶段使用固定分类，不马上做复杂反射：

```cpp
enum class ObjectCategory : uint8_t {
    Widget,
    Component,
    UiResource,
    GpuResource,
    Font,
    Manager,
    RenderObject,
    Other
};
```

固定分类的好处：

- 快照容易阅读；
- 能直接和当前 ImageDemo 内存问题对应；
- 不依赖 RTTI 名称稳定性；
- 后续可扩展 typeName。

---

## 5. 如何接入 MorrowUI 对象

### 5.1 Widget：优先接入

Widget 是最适合优先接入的对象类型，因为它具有：

- UUID；
- widget name；
- widget type；
- parent/children；
- ComponentManager；
- visible 状态；
- stage 关系。

建议在 Widget 中增加：

```cpp
DebugObjectId m_debugObjectId = 0;
```

构造函数中登记：

```cpp
Widget::Widget() {
    m_debugObjectId = ObjectRegistry::getInstance().registerWidget(this, ...);
    m_componentManager = std::make_unique<ComponentManager>();
}
```

析构函数中注销：

```cpp
Widget::~Widget() {
    ObjectRegistry::getInstance().unregisterObject(m_debugObjectId);
}
```

但是当前 `Widget` 的析构函数是头文件中的 `= default`，并且派生类大量使用
默认析构函数。正式实施时应先把 Widget 析构函数移到 `.cpp`，确保注销逻辑在
基类析构阶段稳定执行。

### 5.2 Component：第二优先级

Component 创建集中在：

```cpp
ComponentManager::addComponent<T>()
```

因此可以在这个模板中统一登记 Component，而无需修改每个具体组件：

```cpp
auto component = std::make_shared<T>(...);
ObjectRegistry::registerComponent(component.get(), typeName<T>(), ...);
```

销毁路径需要特别注意当前实现：

- `removeComponent<T>()` 会从 manager 容器中移除；
- 但外部可能仍持有 `shared_ptr<Component>`；
- `onDestroy()` 并不等于 C++ 析构；
- 真正析构时间由最后一个 `shared_ptr` 决定。

因此 Component 必须在真正的 C++ 析构中注销，而不能在 `removeComponent()`
调用时立即注销，否则 snapshot 会漏掉仍然存活的外部引用。

推荐将 `Component` 析构函数从 `= default` 改为 `.cpp` 实现，并在其中注销。

### 5.3 Texture、Material、VertexArray、Font

这些对象建议分两层统计：

1. **CPU对象层**
   - `Texture`
   - `Material`
   - `VertexArray`
   - `ShaderStorageBuffer`
   - `DynamicFont`
CPU对象不建议一开始全部手工改构造/析构。可以先从高风险对象接入：

```text
Texture
VertexArray
ShaderStorageBuffer
DynamicFont
UniformBuffer
```

而 `Material` 主要是共享引用关系，第一阶段只统计对象数量和 owner 信息，
不尝试重建全部 `shared_ptr` 引用图。

### 5.4 Manager 和 Singleton

`GlobalObject`、`FontManager`、`TextureManager`、`RenderingThread` 等管理对象
可以作为 `Manager` 分类注册，但它们数量有限，不是主要泄漏对象。

建议在第一阶段只登记：

- `GlobalObject` 的各个 manager；
- `RenderDeviceProxy`；
- `GLRenderDevice`；
- `ResourceRegistry` 的资源统计。

不要把每一个临时容器、锁、命令 header 都当成 ObjectDB 对象，否则会使快照
失去诊断重点。

---

## 6. Snapshot 数据格式

建议使用 JSON，便于：

- 命令行生成；
- Python/PowerShell 分析；
- CI 做基线比较；
- 后续接 Web 或 Debugger UI；
- 不绑定具体调试器。

示例：

```json
{
  "schema": 1,
  "engine": "MorrowUI",
  "timestamp": "2026-08-03T17:30:00",
  "frame": 950834,
  "process": {
    "privateBytes": 0,
    "workingSet": 0,
    "commitBytes": 0
  },
  "summary": {
    "total": 42,
    "widgets": 12,
    "components": 31,
    "textures": 2,
    "fonts": 1
  },
  "objects": [
    {
      "id": 104,
      "type": "MRImage",
      "category": "Widget",
      "name": "",
      "uuid": "stable-widget-uuid",
      "parentId": 1,
      "createdAtFrame": 12,
      "estimatedBytes": 0,
      "flags": ["in_tree", "visible"]
    }
  ],
  "widgetTree": {
    "rootId": 1,
    "orphans": []
  },
  "orphans": []
}
```

进程内存字段应由平台抽象提供：

```cpp
struct ProcessMemoryStats {
    uint64_t privateBytes = 0;
    uint64_t workingSetBytes = 0;
    uint64_t commitBytes = 0;
};
```

Windows 可使用 `GetProcessMemoryInfo()`，Linux 可读取 `/proc/self/status`
或 `/proc/self/statm`，QNX 使用对应系统 API。读取失败时写 0，并记录
`available=false`，不要伪造数据。

---

## 7. Snapshot Diff 设计

### 7.1 不使用地址做 diff key

对象 diff key 应优先级如下：

1. ObjectDB debug ID；
2. Widget UUID；
3. 资源逻辑 ID；
4. 仅在进程内临时分析时使用地址。

其中 debug ID 适合单次运行内的 snapshot diff。跨进程运行的 snapshot 不应假设
debug ID 相同。

### 7.2 Diff 结果

建议输出：

```json
{
  "from": "before.json",
  "to": "after.json",
  "summary": {
    "added": 4,
    "removed": 1,
    "retained": 38,
    "bytesDelta": 33554432
  },
  "byType": [
    {
      "type": "ShaderStorageBuffer",
      "added": 0,
      "removed": 0,
      "countDelta": 0,
      "estimatedBytesDelta": 0
    }
  ],
  "orphanedWidgets": [],
  "warnings": [
    "GPU live handles are stable but process private bytes increased."
  ]
}
```

### 7.3 泄漏判定

不能把“snapshot B 中仍存在”直接判定为泄漏。建议采用多次快照：

```text
S0：启动完成
S1：开始执行操作
S2：操作结束
S3：等待 10 秒后
S4：重复操作后
```

只有满足以下条件才提升为泄漏候选：

- 同类型对象数量持续增长；
- 创建帧早于最近一次操作；
- parent/owner 已断开；
- 多个 GC/回收周期后仍然存在；
- CPU object 与 GPU live handle 或 pool 数据相互印证。

---

## 8. Orphan Widget 检测

MorrowUI 的 orphan 定义建议为：

```text
Widget 仍被 ObjectRegistry 登记
且 m_parent == nullptr
且不是当前 Window 根节点
且没有处于允许的 transient 状态
```

建议 `ObjectRecord` 增加：

```cpp
bool isInStage = false;
DebugObjectId parentId = 0;
uint64_t lastAttachedFrame = 0;
uint64_t lastDetachedFrame = 0;
```

注意 `Widget` 当前 `addChild()` 会先调用：

```cpp
widget->removeFromStage();
```

因此对象在重挂载操作中可能短暂无 parent。建议增加 transient grace period：

```text
detach 后 2～3 帧内不报告 orphan
```

或者在 snapshot 时只报告：

```text
detachedFrame + graceFrames < currentFrame
```

这样可以减少正常重挂载造成的误报。

---

## 9. 通过命令触发 Snapshot

### 9.1 目标命令

设计上的完整命令格式：

```text
snapshot
snapshot path=logs/objects-001.json
snapshot diff=logs/objects-001.json
objectdb summary
objectdb list category=Widget
objectdb list type=ShaderStorageBuffer
objectdb orphans
objectdb dump id=104
objectdb help
```

当前已实施最小命令：

```text
snapshot path=<输出 JSON 路径>
snapshot <输出 JSON 路径>
```

`ImageDemo` 启动方式：

```text
# 进程退出前生成一次快照
ImageDemo --frames 5 --object-snapshot build/object-snapshot.json
# 实际输出：build/object-snapshot-frame-5.json

# 运行期间每约 2 秒检查一次命令文件
ImageDemo --object-snapshot-command build/object-snapshot.cmd
```

运行时触发示例：

```powershell
'snapshot path=build/runtime-object-snapshot.json' |
    Set-Content -Encoding UTF8 build/object-snapshot.cmd
```

输出文件会在扩展名前追加当前帧号，例如
`build/runtime-object-snapshot-frame-1520.json`。如果同帧文件已存在，还会继续追加
`-1`、`-2` 等序号，避免覆盖已有快照。

命令文件消费后会被删除。为避免写入和读取竞争，正式工具应先写临时文件，再使用
同目录原子 rename 发布。

命令执行结果建议返回：

```text
OK snapshot written: logs/objectdb-20260803-173000.json
```

错误示例：

```text
ERROR invalid snapshot path
ERROR object id not found
ERROR ObjectRegistry is not initialized
```

### 9.2 推荐命令通道分层

#### 第一阶段：文件 mailbox

实现成本最低，适合 ImageDemo 长时间测试：

```text
logs/commands/in/
logs/commands/out/
```

外部工具写入：

```text
logs/commands/in/snapshot.cmd
```

Engine heartbeat 每 100～250 ms 检查一次，执行后将文件移动到：

```text
logs/commands/done/
```

输出结果写入：

```text
logs/commands/out/snapshot.result
```

优点：

- Windows、Linux、QNX 都容易实现；
- 不需要开放端口；
- 可通过 PowerShell 触发；
- 适合离线测试和现场复现。

缺点：

- 文件系统轮询；
- 命令原子性需要通过临时文件 + rename 保证；
- 不适合作为最终高频远程协议。

#### 第二阶段：Windows Named Pipe / Unix Domain Socket

为开发机提供实时命令：

```text
Windows: \\.\pipe\morrowui-debug-<pid>
Linux/QNX: /tmp/morrowui-debug-<pid>.sock
```

调试服务线程只负责：

- 接收文本命令；
- 放入 `DebugCommandBus`；
- 返回 command id。

真正的 snapshot 操作必须在 Engine 主线程或指定安全点执行，避免在任意线程
遍历 Widget、Component 和资源对象。

#### 第三阶段：Debugger UI 协议

后续可以增加类似 Godot EngineDebugger 的 TCP/本地协议，但不建议第一版就做。
协议应复用同一套命令和 JSON snapshot，避免出现两套分析逻辑。

### 9.3 启动参数

运行时命令之外，建议支持：

```text
--objectdb-snapshot=<path>
--objectdb-snapshot-after=<seconds>
--objectdb-command-dir=<path>
--objectdb-disabled
```

启动参数适合：

- 自动化回归；
- CI；
- 复现测试；
- 无法交互操作的目标设备。

---

## 10. Snapshot 安全点与线程模型

MorrowUI 当前可能存在主线程和渲染线程。建议遵循以下规则：

### 主线程采集

主线程负责：

- Widget/Component 对象枚举；
- parent/children 关系；
- orphan 判断；
- CPU 资源对象信息；
- FrameState 信息。

### 渲染线程采集

渲染线程负责：

- `ResourceRegistry` live/slot；
- GPU fence/pending；
- recycle pool；
- 上传计数。

### 合并方式

不要让主线程直接遍历渲染线程的容器。建议：

```text
主线程请求 snapshot
    ↓
读取主线程对象 registry
    ↓
请求 RenderMemoryDiagnostics 原子/锁保护快照
    ↓
合并为 ObjectSnapshot
    ↓
写 JSON
```

当前已经实现的 `RenderMemoryDiagnostics` 可以作为 GPU 部分的第一版 provider。

若未来需要严格一致性，可在 `endFrame()` 建立 frame barrier，再生成带有：

```text
mainFrame
renderFrame
snapshotFrame
```

三个时间字段的快照，而不是强行假设所有字段同一时刻采集。

---

## 11. 与当前 ImageDemo 内存问题的结合

ObjectDB 分析器能够回答：

```text
Widget/Component/Texture/Font CPU对象是否在增长？
哪些对象在重复操作后仍然存在？
哪些 Widget 已脱离 Window 树？
哪些对象的创建帧很早但一直没有销毁？
```

但它不能单独回答：

```text
WGL/DWM presentation surface 是否增长？
OpenGL driver backing store 是否增长？
显卡共享内存是否增长？
Windows allocator arena 是否保留高水位？
```

因此 ObjectDB 必须和以下数据一起保存：

- `[MemoryDiag]`；
- Windows Private Bytes；
- Working Set；
- Commit Size；
- GPU dedicated/shared memory；
- 跨显示器前后的 framebuffer size/content scale。

推荐现场操作流程：

```text
1. 启动 ImageDemo
2. objectdb snapshot path=before.json
3. 等待稳定
4. 跨显示器拖动窗口并松手
5. 等待 10 秒
6. objectdb snapshot path=after.json
7. objectdb diff before.json after.json
8. 对比 MemoryDiag 和进程/GPU 统计
```

如果 `after.json` 中对象数量和 GPU live handle 都稳定，但 Private Bytes 增长，
应优先转向 WGL/DWM/驱动分析，而不是继续搜索 Widget 泄漏。

---

## 12. 分阶段实施计划

### Phase 0：方案设计

**状态：已完成。**

- 明确 Godot ObjectDB 的适用范围；
- 识别 MorrowUI 对象模型差异；
- 确定非拥有型 registry；
- 确定 JSON snapshot 和命令格式；
- 确定文件 mailbox 为第一命令通道。

### Phase 1：最小对象登记

**状态：已完成。**

已新增：

```text
src/debug/ObjectRegistry.h/.cpp
```

当前接入：

- Widget；
- Component；
- Texture；
- DynamicFont；

输出：

- 总数；
- 按类型统计；
- 对象 ID、类型、名称、创建帧；
- orphan Widget；
- JSON snapshot。

实现特点：

- Registry 仅保存对象状态的 `weak_ptr`，不持有业务对象；
- `MORROW_ENABLE_OBJECT_DIAGNOSTICS` CMake 选项可整体裁剪；
- Widget 类型在 snapshot 前从现有 `m_widgetType` 同步；
- Component 类型通过编译器签名提取为稳定可读名称；
- 不引入统一 Object 基类或 ClassDB；
- 不改变 `shared_ptr` 所有权。

### Phase 2：命令触发

**状态：最小版已完成。**

- `Engine::writeObjectSnapshot(path)` 可由应用直接调用；
- `EngineOptions::objectSnapshotPath` 支持退出前快照；
- `EngineOptions::objectSnapshotCommandPath` 支持低频命令文件触发；
- `ImageDemo` 暴露对应的命令行参数。

尚未实施：

- `objectdb summary/list/dump` 交互命令；
- snapshot diff；
- Named Pipe/Domain Socket；
- 远程 Debugger UI。

### Phase 3：引用和 owner 信息

增加：

- Widget parent/child；
- Component -> Widget；
- Texture -> Material/Widget；
- ShaderStorageBuffer -> RenderBatch；
- CPU object -> GPU handle。

这里必须避免保存 owning `shared_ptr`。关系边应使用：

```text
DebugObjectId
weak_ptr
或受控的非拥有裸指针
```

### Phase 4：Debugger/远程工具

最后再考虑：

- Named Pipe；
- Unix Domain Socket；
- TCP；
- Web UI；
- 两个 snapshot 的图形化 diff。

---

## 13. 不建议直接照搬的部分

### 13.1 不要为了 ObjectDB 让所有对象继承新基类

这会导致：

- 大量头文件和 ABI 改动；
- `shared_ptr` 体系与基类析构顺序复杂化；
- 资源、GPU handle、平台对象被迫使用同一生命周期模型；
- 第一阶段收益不高。

### 13.2 不要让 registry 保存 shared_ptr

错误：

```cpp
std::unordered_map<DebugObjectId, std::shared_ptr<void>>
```

这会让 ObjectDB 直接持有对象，造成：

- 泄漏检测失真；
- registry 自己延长生命周期；
- shutdown 顺序复杂；
- 跨线程析构风险。

正确方式是保存：

```text
raw pointer + 生命周期注销
或 weak_ptr + 非拥有元数据
```

### 13.3 不要第一阶段实现完整引用图

`shared_ptr` 控制块、aliasing constructor、容器内部引用和 lambda capture
都使完整引用图成本很高。

第一阶段先判断：

```text
对象数量是否增长
对象类型是否集中
对象是否脱离 Widget 树
GPU live handle 是否同步增长
```

这已经可以覆盖大多数 MorrowUI 泄漏定位需求。

---

## 14. 预期收益和限制

### 能解决的问题

- Widget/Component 是否不断创建；
- 某类对象是否只增不减；
- Widget 是否从树上移除后仍然存活；
- 哪个操作前后对象数量发生变化；
- CPU对象数量与 GPU handle 是否一致；
- 多次操作 snapshot 的差异。

### 不能单独解决的问题

- OpenGL driver 内部 backing store；
- WGL/DWM swapchain/presentation surface；
- 显卡 dedicated/shared memory；
- malloc allocator arena；
- 第三方库内部对象；
- 未接入 registry 的临时栈对象。

这些限制与 Godot ObjectDB Profiler 自身的定位一致：它是对象生命周期分析器，
不是完整进程内存分析器。

---

## 15. 参考资料

1. Godot ObjectDB Profiler 文档：  
   <https://docs.godotengine.org/en/stable/tutorials/scripting/debug/objectdb_profiler.html>
2. Godot Object class 架构文档：  
   <https://docs.godotengine.org/en/stable/engine_details/architecture/object_class.html>
3. Godot Node orphan node API 文档：  
   <https://docs.godotengine.org/en/stable/classes/class_node.html>
4. Godot Node/ObjectDB 相关源码浏览：  
   <https://blog.weghos.com/godot/Godot/scene/main/node.cpp.html>
