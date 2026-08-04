# ResourceHandle 统一重构方案

> **状态**: 方案已确认，待实施  
> **日期**: 2026-07-15  
> **目标**: 将引擎前端所有 GPU 资源引用统一为整数 `ResourceHandle`，类似 Filament 的 `Handle<Hw*>` 体系。后端维护 GPU 资源注册表，前端对象仅持有 Handle（整数 ID），避免持有各种裸指针 / shared_ptr 到后端对象。

---

## 1. 动机

### 1.1 当前问题

| 问题 | 说明 |
|---|---|
| **混合所有权模型** | `Texture::m_texture2DPtr` 是裸指针，`Material::m_shader` 是 `GPUProgramHandle*`，`VertexArray::m_vbo` 是 `VBO*`，缺乏一致性 |
| **裸指针悬挂风险** | 多线程渲染路径中，前端持有 `Texture2D*`、`UBO*`、`VBO*` 等指向渲染线程对象的裸指针，一旦后端先于前端析构，即为悬挂指针 |
| **Handle 体系不完整** | 已有 `GpuHandle<T>` / `XXXHandle` 包装类，但其实质仍是指针的薄包装（`getReal()` 返回裸指针），未解决所有权问题 |
| **隐式生命周期依赖** | `OffscreenRenderTarget` 析构时调用 `RENDERINGTHREAD->deleteRenderTarget()`，这种跨线程手动删除模式分散在各处，容易遗漏 |
| **无法做资源追踪/调试** | 没有全局资源 ID，难以实现资源泄漏检测、GPU 内存统计、帧捕获工具集成 |

### 1.2 目标

1. 所有前端对象 **只持有整数 `ResourceHandle`**，不持有任何指向后端对象的指针
2. 后端维护 **`ResourceRegistry`**，负责 Handle ↔ GPU 对象的映射和生命周期
3. 前端对象析构时，Handle 自动通知后端销毁对应 GPU 资源
4. 统一 6 类 GPU 资源的句柄体系：**VBO、UBO、SSBO、Texture2D、GPUProgram、RenderTarget**

---

## 2. 现有 GPU 资源类型全景

### 2.1 资源类型清单

| # | 资源类型 | 当前前端持有方式 | 持有该资源的对象 | 后端实现 |
|---|---|---|---|---|
| 1 | **Texture2D** | `Texture2D*` 裸指针 | `Texture::m_texture2DPtr`、`OffscreenRenderTarget::m_colorTexture` | `GlTexture2D` |
| 2 | **VBO** | `VBO*` 裸指针 | `VertexArray::m_vbo`、`VertexArray::m_instanceVBO` | `GlVBO` |
| 3 | **UBO** | `UBO*` 裸指针 | `UniformBuffer::m_globalUBO`、`Material::m_scene3DMaterialUbo`、`FrameState::scene3DFrameUBO`、`Scene3DPassContext::frameUBO` | `GlUBO` |
| 4 | **SSBO** | `SSBO*` 裸指针 | `ShaderStorageBuffer::m_ssbo` | `GlSSBO` |
| 5 | **GPUProgram** | `GPUProgramHandle*` 指针包装 | `Material::m_shader`、`Material::m_batchShader`、`VertexArray::m_uploadedProgram` | `GlProgram` |
| 6 | **RenderTarget** | `RenderTarget*` 裸指针 | `OffscreenRenderTarget::m_renderTarget` | GL FBO（内嵌于 `GLRenderDevice`） |

### 2.2 中间层 "Handle" 包装（已有但需重构）

```cpp
// 当前: Handle 实质是指针包装，不是真正的整数 ID
class Texture2DHandle : public Texture2D {
    Texture2D* m_realTexture = nullptr;  // 仍然持有后端裸指针
};
class VBOHandle : public VBO { VBO* m_realVbo = nullptr; };
class UBOHandle : public UBO { UBO* m_realUbo = nullptr; };
// ... 等等
```

这些 Handle 包装类仅在多线程路径中作为命令队列的 payload 使用——前端创建 `XXXHandle`，编码命令发到渲染线程，渲染线程通过 `getReal()` 解引用执行。**本质上仍然是绕过所有权管理的指针传递**。

### 2.3 相关的数据传输对象（DTO）

以下对象是从 CPU 到 GPU 的**数据载体**，不是 GPU 资源本身，**不需要** Handle 化：

- `TextureData` — 纹理上传像素数据
- `VBOData` / `VBODataSharedPtr` — 顶点/索引数据
- `UBOData` / `SSBOData` — Uniform/Storage block 数据
- `BatchCompatibilityKey`、`RenderItem` 等 — 纯 CPU 合批数据结构

---

## 3. 设计方案

### 3.1 ResourceHandle 定义

```cpp
// 通用整数 Handle，类似 filament::Handle<HwTexture>
// 0 表示无效 / null handle
//
// 预留 generation 字段 — 初期固定为 1，后续可启用 ABA 防护：
//   打开 MORROW_HANDLE_GENERATION_CHECK 宏后，ResourceRegistry 在
//   每次 ID 复用时递增 generation，Handle 访问时校验 generation 匹配。
template <typename Tag>
struct ResourceHandle {
    uint32_t id = 0;

#if MORROW_HANDLE_GENERATION_CHECK
    uint16_t generation = 1;
#endif

    bool isValid() const { return id != 0; }
    explicit operator bool() const { return id != 0; }
    bool operator==(ResourceHandle other) const { return id == other.id; }
    bool operator!=(ResourceHandle other) const { return id != other.id; }

    struct Hash {
        size_t operator()(ResourceHandle h) const { return std::hash<uint32_t>{}(h.id); }
    };
};

// 各资源类型的 Handle 别名
using HwTexture2D    = ResourceHandle<struct Texture2DTag>;
using HwVBO          = ResourceHandle<struct VBOTag>;
using HwUBO          = ResourceHandle<struct UBOTag>;
using HwSSBO         = ResourceHandle<struct SSBOTag>;
using HwGPUProgram   = ResourceHandle<struct GPUProgramTag>;
using HwRenderTarget = ResourceHandle<struct RenderTargetTag>;
```

### 3.2 后端 ResourceRegistry

```cpp
class ResourceRegistry {
public:
    // ── 前端调用（线程安全，非阻塞，立即返回 Handle）──
    // 从原子自由列表预分配 ID，真正的 GPU 对象由渲染线程稍后创建
    HwTexture2D    allocateTexture2D();
    HwVBO          allocateVBO();
    HwUBO          allocateUBO();
    HwSSBO         allocateSSBO();
    HwGPUProgram   allocateGPUProgram();
    HwRenderTarget allocateRenderTarget();

    // ── 渲染线程调用（单线程，在 execute 阶段）──
    void commitTexture2D(HwTexture2D handle, GlTexture2D* obj);
    void commitVBO(HwVBO handle, GlVBO* obj);
    // ... 同理

    // ── 渲染线程调用：销毁 GPU 对象并回收 ID ──
    void destroyTexture2D(HwTexture2D handle);
    // ... 同理

    // ── 渲染线程调用：查询后端对象 ──
    GlTexture2D*   getTexture2D(HwTexture2D handle);
    GlVBO*         getVBO(HwVBO handle);
    // ... 同理

private:
    // 每种资源类型一个稠密数组，用 handle.id 索引
    std::vector<GlTexture2D*>  m_textures;
    std::vector<GlVBO*>        m_vbos;
    std::vector<GlUBO*>        m_ubos;
    std::vector<GlSSBO*>       m_ssbos;
    std::vector<GlProgram*>    m_programs;

    // 原子 ID 分配器（前端线程安全访问）
    std::atomic<uint32_t> m_nextTextureId{1};
    std::atomic<uint32_t> m_nextVboId{1};
    // ... 或使用 LockFreeFreeList 复用已释放的 ID
};
```

**关键设计决策：**

1. **后端（`GLRenderDevice`）持有 `ResourceRegistry`**。资源创建/销毁发生在渲染线程，线程安全天然保证。
2. **两阶段创建，前端不阻塞**：
   - **阶段 1（前端线程）**：`allocateTexture2D()` 从原子计数器预分配 Handle ID，立即返回 `HwTexture2D{id}`。真正的 GPU 对象此时不存在。
   - **阶段 2（渲染线程）**：执行 `Cmd_CreateTexture2D` 时，创建 `GlTexture2D` 并调用 `commitTexture2D(handle, obj)` 存入注册表。
   - **保序保证**：后续对此 Handle 的 `useTexture2D` / `updateTexture2D` 命令在队列中严格排在创建命令之后，渲染线程按序执行，因此使用时 GPU 资源一定已就绪。
3. **Handle ID 分配**：通过 `std::atomic<uint32_t>` 递增计数器（简单无复用）或无锁自由列表（复用已释放 ID）。`id=0` 始终表示无效。
4. **删除为异步**：前端销毁 Handle 时，将删除命令发送到渲染线程的命令队列，由渲染线程执行 `ResourceRegistry::destroy*()`。这与当前 `CommandBuffer` 编码体系完全兼容。

### 3.3 前端对象变更

每个需要持有 GPU 资源的前端对象，将裸指针/包装指针替换为 `ResourceHandle`：

| 前端对象 | 当前成员 | 重构后成员 |
|---|---|---|
| `Texture` | `Texture2D* m_texture2DPtr` | `HwTexture2D m_handle` |
| `VertexArray` | `VBO* m_vbo`、`VBO* m_instanceVBO` | `HwVBO m_vboHandle`、`HwVBO m_instanceVboHandle` |
| `VertexArray` | `GPUProgram* m_uploadedProgram` | `HwGPUProgram m_uploadedProgramHandle` |
| `UniformBuffer` | `UBO* m_globalUBO` | `HwUBO m_globalUBOHandle` |
| `ShaderStorageBuffer` | `SSBO* m_ssbo` | `HwSSBO m_ssboHandle` |
| `Material` | `GPUProgramHandle* m_shader` | `HwGPUProgram m_shaderHandle` |
| `Material` | `GPUProgramHandle* m_batchShader` | `HwGPUProgram m_batchShaderHandle` |
| `Material` | `UBO* m_scene3DMaterialUbo` | `HwUBO m_materialUboHandle` |
| `OffscreenRenderTarget` | `RenderTarget* m_renderTarget` | `HwRenderTarget m_rtHandle` |
| `OffscreenRenderTarget` | `Texture2D* m_colorTexture` | `HwTexture2D m_colorTexHandle` |
| `FrameState` | `UBO* scene3DFrameUBO` | `HwUBO scene3DFrameUBO` |
| `Scene3DPassContext` | `UBO* frameUBO` | `HwUBO frameUBO` |

### 3.4 API 变更：RenderDevice 接口

```cpp
// 变更前：返回裸指针 + GPUProgramParam* 作为独立句柄
class GPUBufferDevice {
    virtual VBO*  createVBO() = 0;
    virtual void  updateVBO(GPUProgram* program, VBO* vbo, ...) = 0;
    virtual void  deleteVBO(VBO* vbo) = 0;
    virtual void  drawVBO(VBO* vbo, ...) = 0;
};

// 变更后：所有接口使用 Handle
class GPUBufferDevice {
    virtual HwVBO createVBO() = 0;
    virtual void  updateVBO(HwGPUProgram program, HwVBO vbo, ...) = 0;
    virtual void  deleteVBO(HwVBO vbo) = 0;
    virtual void  drawVBO(HwVBO vbo, ...) = 0;
};
```

同理 `GPUTextureDevice`、`GPUShaderDevice`、`GPURenderPassDevice` 的所有接口中 `XXX*` → `HwXXX`。

**特别注意 `GPUShaderDevice`：** 重构后 `GPUProgramParam` 不作为独立资源。所有 uniform/attribute 设置统一通过 `HwGPUProgram` + 名称字符串完成，不再提供 `GPUProgramParam*` 相关的接口重载。

### 3.5 需要保留的现有基础设施

以下系统**不需要大改**，只需将内部指针替换为 Handle：

| 组件 | 说明 |
|---|---|
| `CommandBuffer` | 零分配命令编码，payload 中的 `XXX*` 变为 `HwXXX`（仍是 4 字节） |
| `RenderDeviceProxy` | 多线程代理，命令 payload 和 execute 适配 Handle |
| `GLRenderDevice` | 唯一真正的 GL 调用者，内部配合 `ResourceRegistry` 解析 Handle |
| `BatchManager` / `BatchBuilder` | 合批系统，`RenderBatch` 中的 `shared_ptr<VertexArray>` 不变（VertexArray 内部改用 Handle） |
| `RenderingThread` | 渲染线程框架不变 |

---

## 4. 不需要 Handle 化的对象

以下对象是 **纯 CPU 数据结构** 或 **组合型前端对象**，不直接持有 GPU 资源，因此**无需修改**：

| 对象 | 原因 |
|---|---|
| `Mesh` | 纯 CPU 顶点/索引数据容器，没有 GPU 引用 |
| `MeshFilter` | 持有 `MeshSharedPtr`，Mesh 是纯 CPU 数据 |
| `Transform` / `Transform3D` | 纯数学变换 |
| `Widget` / `UIWidget` | UI 树节点 |
| `Component` / `ComponentManager` | 组件框架 |
| `Camera` 系列 | 数学投影 |
| `Material` 的纹理引用 (`m_textureMap`) | 持有 `TextureSharedPtr`，Texture 内部改用 Handle，外部 API 不变 |
| `TextureInfo` / `TextureData` / `VBOData` / `UBOData` | 纯数据传输对象 |

---

## 5. 迁移策略

### 5.1 阶段划分

```
Phase 1: 基础设施（约 2-3 天）
├── 定义 ResourceHandle<Tag> 模板
├── 实现 ResourceRegistry（后端资源注册表）
├── 修改 GpuTypes.h，废弃旧的 GpuHandle<T> 体系
└── 添加单元测试：Handle 分配/回收/查询

Phase 2: 后端适配（约 2-3 天）
├── GLRenderDevice 内部集成 ResourceRegistry
├── 修改 4 个 Device 子接口的虚函数签名（XXX* → HwXXX）
├── 修改 RenderDeviceProxy 的命令编码/执行以适配 Handle
└── 移除旧的 XXXHandle 包装类（Texture2DHandle 等）

Phase 3: 前端对象逐个迁移（约 3-5 天）
├── Texture       → HwTexture2D
├── VertexArray   → HwVBO + HwGPUProgram
├── UniformBuffer → HwUBO
├── ShaderStorageBuffer → HwSSBO
├── Material      → HwGPUProgram + HwUBO
├── OffscreenRenderTarget → HwRenderTarget + HwTexture2D
├── FrameState / Scene3DPassContext → HwUBO
└── 清理析构函数中的手动 deleteXXX 调用

Phase 4: 回归验证（约 1-2 天）
├── 编译所有 Sample（19 个 Demo）
├── 运行单元测试（BatchBuilder 等）
└── 运行时验证：资源泄漏检测 / GPU 内存统计
```

### 5.2 风险与缓解

| 风险 | 缓解措施 |
|---|---|
| **编译期错误**：接口签名变更影响面大 | 使用 `using` 别名逐步替换；Phase 2 先改接口，再逐个改实现 |
| **Handle 生命周期错乱** | 在 `ResourceRegistry` 中添加 debug 模式的 use-after-free 检测（类似 fillet 的 `assert_invariant`） |
| **命令队列中 Handle 已失效** | 命令 payload 中的 Handle 由发送方保证引用计数；渲染线程执行时 Handle 一定有效（命令队列天然保序） |
| **RenderBatch 中的 VertexArray** | `RenderBatch` 持有 `shared_ptr<VertexArray>`，VertexArray 的 Handle 在 VB 生命周期内有效；保持不变 |

---

## 6. API 对照示例

### 6.1 Texture 使用（变更前/后）

```cpp
// ── 变更前 ──
Texture::render(FrameStateSharedPtr frameState) {
    // ...
    RENDERINGTHREAD->useTexture2D(m_texture2DPtr, index);  // 裸指针
}

Texture::~Texture() {
    if (m_texture2DPtr) {
        RENDERINGTHREAD->deleteTexture2D(m_texture2DPtr);  // 手动删除
    }
}

// ── 变更后 ──
Texture::render(FrameStateSharedPtr frameState) {
    if (m_handle.isValid()) {
        RENDERINGTHREAD->useTexture2D(m_handle, index);   // 整数 Handle
    }
}

Texture::~Texture() {
    if (m_handle.isValid()) {
        RENDERINGTHREAD->deleteTexture2D(m_handle);       // 异步销毁
    }
}
```

### 6.2 Material 使用（变更前/后）

```cpp
// ── 变更前 ──
class Material {
    GPUProgramHandle* m_shader = nullptr;
    GPUProgramHandle* m_batchShader = nullptr;
    UBO* m_scene3DMaterialUbo = nullptr;
};

// ── 变更后 ──
class Material {
    HwGPUProgram m_shaderHandle{0};
    HwGPUProgram m_batchShaderHandle{0};
    HwUBO m_materialUboHandle{0};
};
```

### 6.3 资源创建流程（变更后）

```cpp
// ── 前端调用（非阻塞） ──
// Step 1: ResourceRegistry 从原子自由列表预分配 Handle ID，立即返回
HwTexture2D handle = RENDERINGTHREAD->createTexture2D(imageType);
// handle.id 此时已有效（如 42），前端可以继续执行后续逻辑

// Step 2: 编码异步创建命令到 CommandBuffer
// → 渲染线程稍后执行
// → GLRenderDevice::executeCreateTexture2D(handle, imageType)
// → 创建 GlTexture2D，存入 registry.m_textures[42]
// → handle 变为"就绪"状态

// ── 线程安全保证 ──
// 后续对同一 handle 的 useTexture2D / updateTexture2D 命令
// 在命令队列中严格排在 create 之后，渲染线程按序执行，
// 因此使用时 GPU 资源一定已就绪。
```

**Handle ID 预分配机制：**

```cpp
class ResourceRegistry {
    // 原子自由列表 — 前端线程和渲染线程均可安全访问
    std::atomic<uint32_t> m_nextId{1};          // 简单递增（无复用）
    // 或
    LockFreeFreeList m_freeList;                // 复用已释放的 ID

    // 前端调用（线程安全，不阻塞）
    HwTexture2D allocateTexture2D() {
        return HwTexture2D{ m_nextId.fetch_add(1) };
    }

    // 渲染线程调用（单线程，在 execute 阶段）
    void commitTexture2D(HwTexture2D handle, GlTexture2D* obj) {
        ensureCapacity(m_textures, handle.id);
        m_textures[handle.id] = obj;
    }
};
```

**与当前架构的对比：**

```
当前：  createTexture2D() → new Texture2DHandle()  → 返回占位指针（m_realTexture=nullptr）
       渲染线程执行 Cmd_CreateTexture2D → 填充 m_realTexture

重构后：createTexture2D() → allocateTexture2D()   → 返回 HwTexture2D{id}
       渲染线程执行 Cmd_CreateTexture2D → commitTexture2D(handle, obj)
```

本质逻辑不变——都是立即返回一个"凭证"，真正的 GPU 资源异步创建。区别仅在于凭证从指针变成了整数。

---

## 7. 额外收益

1. **GPU 资源统计**：遍历 `ResourceRegistry` 的数组即可得到每种资源的数量和大小
2. **帧调试工具**：Handle 是稳定的整数 ID，可以直接映射到 RenderDoc / 自定义调试 UI
3. **序列化友好**：整数 Handle 可以安全地在日志、崩溃报告中输出
4. **未来扩展**：可以轻松添加 `ResourceHandle` 的引用计数或 generational check（参考 Filament 的 `Handle` 实现）
5. **消除 `friend class`**：当前 `GLRenderDevice` 需要 friend 访问 `Texture2D` 等内部类，Handle 化后不再需要

---

## 8. 已决问题

1. **Handle 是否需要 generation 检查**（防止 ABA 复用问题）？
   
   > **决策：初期不需要。** 当前引擎资源生命周期受控（创建/销毁均在命令队列中严格保序），不存在 reuse-before-free 窗口。在 `ResourceHandle` 中预留 `generation` 字段（当前固定为 1），`ResourceRegistry` 中预留 slot generational check 的宏开关，后续需要时打开即可。

2. **`GPUProgramParam` 句柄化**：
   
   > **决策：不作为独立资源。** 重构后 `GPUProgramParam` 不再有独立的 Handle。所有 uniform/attribute 参数操作通过 `HwGPUProgram` + `const std::string& name` 完成。`GPUShaderDevice` 中移除 `GPUProgramParam*` 相关接口，统一使用按名称设置的重载。

3. **`RenderBatch` 中的 `shared_ptr<VertexArray>` 和 `shared_ptr<ShaderStorageBuffer>`**：
   
   > **决策：保持 `shared_ptr` 不变。** Batch 系统操作的是前端逻辑对象（`VertexArray`、`ShaderStorageBuffer`），GPU Handle 是其内部细节。Batch 层不需要感知 Handle。

4. **兼容性过渡期**：
   
   > **决策：不需要过渡期。** 直接全局替换 `XXX*` → `HwXXX`，编译时发现所有未适配的代码。修改范围可控（约 15 个文件），且集中在 `src/renderer/` 目录。

---

## 9. 文件变更清单

### 新增文件
- `src/renderer/ResourceHandle.h` — `ResourceHandle<Tag>` 模板定义
- `src/renderer/ResourceRegistry.h` — 后端资源注册表
- `src/renderer/ResourceRegistry.cpp`

### 修改文件

| 文件 | 变更内容 |
|---|---|
| `src/renderer/GpuTypes.h` | 移除旧的 `GpuHandle<T>` 体系，保留 DTO 结构 |
| `src/renderer/GPUBufferDevice.h` | 接口签名：`XXX*` → `HwXXX` |
| `src/renderer/GPUTextureDevice.h` | 接口签名：`XXX*` → `HwXXX` |
| `src/renderer/GPUShaderDevice.h` | 接口签名：`XXX*` → `HwXXX` |
| `src/renderer/GPURenderPassDevice.h` | 接口签名：`XXX*` → `HwXXX` |
| `src/renderer/RenderDevice.h` | 继承链不变，接口自动更新 |
| `src/renderer/RenderDeviceProxyBase.h` | 移除 `XXXHandle` 包装类，`GPUProgramHandle` → 改造为 `HwGPUProgram` 适配层 |
| `src/renderer/RenderDeviceProxy.h` | 接口签名更新 |
| `src/renderer/RenderDeviceProxy.cpp` | 命令 payload、编码、执行全部适配 Handle |
| `src/renderer/GLRenderDevice.h` | 集成 `ResourceRegistry`，接口签名更新 |
| `src/renderer/GLRenderDevice.cpp` | 内部使用 `ResourceRegistry` 管理所有 GL 对象 |
| `src/renderer/Texture.h` / `.cpp` | `Texture2D*` → `HwTexture2D` |
| `src/renderer/Material.h` / `.cpp` | `GPUProgramHandle*` → `HwGPUProgram`，`UBO*` → `HwUBO` |
| `src/renderer/VertexArray.h` / `.cpp` | `VBO*` → `HwVBO`，`GPUProgram*` → `HwGPUProgram` |
| `src/renderer/UniformBuffer.h` / `.cpp` | `UBO*` → `HwUBO` |
| `src/renderer/ShaderStorageBuffer.h` / `.cpp` | `SSBO*` → `HwSSBO` |
| `src/renderer/OffscreenRenderTarget.h` / `.cpp` | `RenderTarget*` → `HwRenderTarget`，`Texture2D*` → `HwTexture2D` |
| `src/renderer/FrameState.h` | `UBO* scene3DFrameUBO` → `HwUBO` |
| `src/renderer/Scene3DPassContext.h` | `UBO* frameUBO` → `HwUBO` |
| `src/renderer/BatchDataDefine.h` | 不直接依赖 GPU Handle，保持不变 |
| `src/core/GlobalObject.h` | 可能需要适配新的设备接口 typedef |
| 所有 Sample 文件 | 通常无需修改（通过高层 API 间接使用资源） |

---

## 10. 总结

这次重构的核心思想是 **"前端对象不持有后端指针，只持有整数 ID"**。引擎目前已经有一个雏形的 `GpuHandle<T>` 体系和多线程命令编码架构，这为重构提供了良好的基础。

重构后：
- 前端资源的创建/销毁变为异步命令，线程安全由命令队列保证
- 所有 GPU 资源有全局统一的 ID 空间，便于调试和统计
- 消除了混合所有权带来的悬挂指针风险
- API 更加一致：所有 Device 接口统一使用 `HwXXX` 而非 `XXX*`
