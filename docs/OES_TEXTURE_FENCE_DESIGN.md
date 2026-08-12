# OES 外部纹理 Buffer 生命周期设计

## 1. 文档状态

- 状态：已实施初版，待 OES 硬件验证
- 目标文件：`src/renderer/device/RenderDeviceProxy.cpp`
- 本文档只描述设计方案，当前不修改源码。
- 适用场景：多线程渲染模式下，通过 OES / `EGLImage` 使用应用侧外部视频 buffer。

## 2. 问题背景

在多线程模式下，`RenderDeviceProxy::updateTexture2D()` 是异步接口。调用方提交更新命令后立即返回，实际的 OES 纹理绑定和后续绘制由渲染线程执行。

典型场景是应用维护三个 OES buffer：

```text
应用写入 buffer A
    -> updateTexture2D(OES, A)
应用写入 buffer B
    -> updateTexture2D(OES, B)
应用写入 buffer C
    -> updateTexture2D(OES, C)
```

调用 `updateTexture2D()` 返回，只能说明命令已经排入客户端命令缓冲，不能说明：

1. 渲染线程已经执行了该更新；
2. OES 对应的 `EGLImage` 已经完成绑定；
3. GPU 已经完成后续绘制对该外部 buffer 的读取。

如果应用把返回视为“buffer 已经使用完毕”，并立即覆盖同一个 OES buffer，GPU 可能仍在读取该内存，从而产生撕裂、画面异常或数据竞争。

## 3. 根本原因

普通 CPU 像素纹理和 OES 外部纹理的内存语义不同。

### 3.1 普通纹理

普通纹理通过 `glTexImage2D()` 或 `glTexSubImage2D()` 上传像素数据。当前设计可以在多线程代理层将裸指针数据复制到对象池，命令执行时使用对象池副本。

对于这条路径，应用原始 buffer 不再被 GPU 直接引用。上传命令执行所需的数据由命令 payload 持有，对象池负责后续复用，不需要让应用 buffer 等待 GPU Fence。

### 3.2 OES 外部纹理

OES 路径不是将像素数据复制到普通 `GL_TEXTURE_2D` 存储，而是通过外部地址创建或绑定 `EGLImage`，并将其连接到 `GL_TEXTURE_EXTERNAL_OES`：

```text
外部 buffer
    -> eglCreateImageKHR
    -> glEGLImageTargetTexture2DOES
    -> 后续 draw 使用 samplerExternalOES 读取
```

因此，OES buffer 的生命周期必须覆盖 GPU 对当前帧的最后一次读取。渲染线程完成 `updateTexture2D()` 只能作为“命令已执行”的标志，不能作为 buffer 可复用的标志。

## 4. 设计目标

1. OES buffer 只有在 GPU 确认完成使用后才能归还应用侧对象池。
2. 普通纹理继续使用现有的对象池拷贝路径。
3. 去掉 `TextureData::pixelOwner`，不再通过通用像素所有权字段表达外部 buffer 生命周期。
4. 不让普通纹理更新引入额外的 GPU Fence 等待或生命周期管理。
5. 保持 `updateTexture2D()` 的异步特性，避免每次 OES 更新都阻塞调用方。
6. 三 buffer 等有限 buffer 池耗尽时，允许生产者自然产生背压，不能覆盖仍被 GPU 使用的 buffer。

## 5. 总体方案

将纹理更新分为两条明确路径：

| 类型 | 数据传递方式 | Buffer 释放时机 |
| --- | --- | --- |
| 普通纹理 | 多线程代理层复制到对象池 buffer，再随命令提交 | `glTexImage2D` / `glTexSubImage2D` 使用完对应副本后归还对象池 |
| OES 外部纹理 | 不复制外部视频 buffer，只提交外部 buffer 描述和完成回调 | 对应渲染帧的 GPU Fence 完成后回调 |

关键原则：

> 只有 OES 外部纹理需要将外部 buffer 生命周期绑定到 GPU Fence；普通纹理不需要使用外部 buffer Fence。

## 6. `TextureData` 设计调整

### 6.1 移除 `pixelOwner`

删除以下通用所有权字段：

```cpp
std::shared_ptr<void> pixelOwner;
```

原因：

1. 普通纹理走复制路径，不需要通过 `TextureData` 持有应用原始内存；
2. OES buffer 的生命周期不是简单的对象引用生命周期，而是“直到对应 GPU Fence 完成”；
3. 用一个通用 `shared_ptr<void>` 同时表示普通上传数据和 OES 外部 buffer，容易让调用方误解释放时机；
4. OES buffer 应由明确的外部帧生命周期回调管理。

### 6.2 使用 OES GPU 完成回调

OES 外部纹理使用专用回调字段：

```cpp
// 对 OES 外部纹理：
// 对应渲染帧的 GPU Fence 完成后调用。
std::function<void()> gpuUseCompleteCallback;
```

`gpuUseCompleteCallback` 的语义是：

```text
它不表示 updateTexture2D 命令执行完成。
它表示 GPU 已经完成当前帧对该外部 buffer 的使用。
```

该回调只用于 OES 外部纹理。普通纹理不设置该回调，继续使用对象池拷贝路径。

## 7. 普通纹理路径

普通纹理在多线程模式下统一走对象池拷贝：

```cpp
if (!data.pixels || data.bytes <= 0 || data.imageType == ImageType::OES) {
    // OES 走外部 buffer 路径
} else {
    auto buffer = m_pixelDataRecyclePool->acquire();
    buffer->assign(
        static_cast<const uint8_t*>(data.pixels),
        static_cast<const uint8_t*>(data.pixels) + data.bytes
    );

    payload->data.pixels = buffer->data();
    payload->pixelStorage = std::move(buffer);
}
```

普通纹理路径的要求：

1. `payload->data.pixels` 必须指向对象池副本；
2. `payload->pixelStorage` 必须存活到渲染线程完成该上传命令；
3. 不需要持有应用原始指针；
4. 不需要为应用原始 buffer 创建 GPU Fence；
5. 不需要对普通纹理调用 OES 专用的完成回调。

普通纹理通常是一次性加载或低频更新，复制成本在当前场景下可以接受。该路径优先保证生命周期清晰和实现简单。

## 8. OES 路径

OES 路径不复制视频 buffer：

```text
应用视频 buffer
    -> updateTexture2D(OES)
    -> 命令 payload 保存 OES 地址和 gpuUseCompleteCallback
    -> 渲染线程执行 OES EGLImage 绑定
    -> 当前帧绘制完成后插入 GPU Fence
    -> Fence 完成
    -> 调用 gpuUseCompleteCallback
    -> buffer 归还应用侧对象池
```

OES payload 至少需要保存：

```cpp
struct UpdateTexture2DPayload {
    HwTexture2D texture{0};
    TextureData data;
    std::shared_ptr<std::vector<uint8_t>> pixelStorage;
};
```

其中：

- `pixelStorage` 只用于普通纹理拷贝；
- OES 不使用 `pixelStorage`；
- OES 使用 `data.pixels` 作为外部地址；
- OES 使用 `data.gpuUseCompleteCallback` 表达 GPU 使用完成通知；
- OES 不使用 `pixelOwner`。

执行 `Cmd_UpdateTexture2D` 时：

```cpp
m_realDevice->updateTexture2D(pl->texture, pl->data);
```

如果是 OES，不能在这里立即调用 `gpuUseCompleteCallback()`。应该将回调移动到当前渲染帧的待回收列表：

```cpp
if (pl->data.imageType == ImageType::OES && pl->data.gpuUseCompleteCallback) {
    m_frameOESRecycle.callbacks.push_back(std::move(pl->data.gpuUseCompleteCallback));
} else if (pl->data.gpuUseCompleteCallback) {
    // 普通纹理不应走外部 buffer 回调；如保留兼容行为，需单独确认其语义。
    pl->data.gpuUseCompleteCallback();
}
```

更严格的实现可以只允许 OES 设置该回调，并对普通纹理忽略或记录错误。

## 9. Fence 与待回收队列

当前渲染线程在执行完一帧后会插入 Fence，并将本帧资源放入 `PendingFrame`。OES 外部 buffer 应加入同一套队列。

建议扩展：

```cpp
struct PendingFrame {
    void* fence = nullptr;

    std::vector<VBODataSharedPtr> vboRecyclables;
    std::vector<std::shared_ptr<UBOData>> uboRecyclables;
    std::vector<std::shared_ptr<SSBOData>> ssboRecyclables;
    OESFrameRecycle oesFrameRecycle;
};
```

其中 `OESFrameRecycle` 是专用的 OES frame recycle 类型，用于保存本帧提交的 OES 外部纹理完成回调。例如：

```cpp
struct OESFrameRecycle {
    std::vector<std::function<void()>> callbacks;
};
```

渲染线程成员：

```cpp
OESFrameRecycle m_frameOESRecycle;
```

帧执行开始时清空，执行 OES 更新命令时收集 `gpuUseCompleteCallback`，帧结束时：

```cpp
m_pendingFrames.push({
    m_realDevice->insertFence(),
    std::move(m_frameRecyclables),
    std::move(m_frameUBORecyclables),
    std::move(m_frameSSBORecyclables),
    std::move(m_frameOESRecycle)
});
```

只有 Fence 完成后才执行：

```cpp
for (auto& callback : frame.oesFrameRecycle.callbacks) {
    if (callback)
        callback();
}
```

回调执行完成后，才允许对应 OES buffer 回到应用侧可用队列。

## 10. 三 Buffer 示例

应用侧可以将 buffer 状态抽象为：

```cpp
enum class BufferState {
    Free,
    Writing,
    Submitted
};
```

典型流程：

```text
1. 从 Free 队列获取 buffer A。
2. 应用写入 buffer A。
3. 调用 updateTexture2D(OES, A)。
4. buffer A 标记为 Submitted。
5. 渲染线程将 A 的 `gpuUseCompleteCallback` 放入当前帧 Fence 回收列表。
6. Fence 完成后执行 `gpuUseCompleteCallback`。
7. 应用收到回调后将 buffer A 回到 Free 队列。
```

当 A、B、C 都处于 `Submitted` 状态时，应用必须等待任意一个 Fence 完成。此时的等待是正确的生产者背压，不能通过覆盖已有 buffer 来避免等待。

## 11. 回调与帧归属

一个 OES buffer 的回收回调应归属于包含该 OES 更新命令的渲染帧。

推荐规则：

1. 一个 OES buffer 每次提交只能对应一个未完成的 GPU 使用周期；
2. `gpuUseCompleteCallback` 由引擎在对应 Fence 完成后调用；
3. OES 更新命令必须出现在对应绘制命令之前；
4. 当前帧结束时插入的 Fence 必须位于该帧所有 OES 使用命令之后；
5. 每个 `Texture` 对象每帧最多触发一次 OES `updateTexture2D`，因此本设计不处理同一 Texture 同帧多次提交的问题；
6. OES buffer 的 Free、Writing、Submitted 状态和复用策略由应用负责，引擎不管理应用侧 buffer 队列；
7. 应用应在收到 `gpuUseCompleteCallback` 后自行将 buffer 标记为可复用。

## 12. EGLImage 缓存注意事项

当前实现使用外部地址作为 `m_eglImageMap` 的 key。该做法要求外部地址在对应 EGLImage 生命周期内保持稳定。

该设计的使用约束包括：

1. 同一个外部 buffer 在其生命周期内必须保持稳定的虚拟地址；
2. EGLImage 缓存项有效期间，该地址不能被释放后重新分配给不同的底层存储；
3. 应用必须保证用于轮换的多个 OES buffer 使用不同且稳定的地址；
4. 外部 buffer 在 GPU Fence 完成前不能被覆盖、释放或重新绑定；
5. 销毁纹理时，仍需在渲染线程销毁已创建的 EGLImage。

本设计继续以 `TextureData::pixels` 裸地址作为 EGLImage 缓存 key，不增加额外的 buffer 身份字段。该取舍保持现有接口简单，但地址稳定性属于应用与外部 buffer 管理方的契约。

## 13. 不采用的方案

### 13.1 `updateTexture2D()` 内部阻塞等待

不建议在 `RenderDeviceProxy::updateTexture2D()` 中等待渲染线程或 GPU：

1. 会破坏多线程接口的异步特性；
2. 视频流每帧更新都会引入主线程阻塞；
3. 无法充分利用三 buffer 提供的流水线能力；
4. GPU 负载高时可能造成应用线程抖动。

### 13.2 仅等待渲染线程执行更新命令

不充分。渲染线程执行 `eglCreateImageKHR()` 或 `glEGLImageTargetTexture2DOES()` 后，GPU 仍可能在后续 draw 中读取外部 buffer。必须等待 GPU Fence。

### 13.3 对 OES buffer 做普通像素拷贝

不符合 OES 视频流的 zero-copy 目标，且会增加每帧 CPU 拷贝和额外内存带宽。普通纹理采用对象池拷贝，OES 视频流保留外部 buffer 直连。

## 14. 异常与关闭流程

RenderDeviceProxy 销毁前，需要确保 OES 待回收队列不会遗失：

1. 停止接受新的 OES 更新；
2. 提交或清理剩余命令缓冲；
3. 等待所有相关 Fence 完成，或者在平台允许的情况下执行明确的上下文同步；
4. 执行仍未回调的 OES `gpuUseCompleteCallback`；
5. 最后销毁 EGLImage 和纹理资源。

如果无法保证 GPU 已经停止访问外部 buffer，不能直接执行回调让应用复用或释放 buffer。

## 15. 验证计划

实现后至少需要验证以下场景：

1. 三个 OES buffer 按帧轮换更新；
2. 人为增加 GPU 工作量，使渲染线程落后于生产线程；
3. 生产者在收到 `gpuUseCompleteCallback` 前尝试复用 buffer，应被阻塞或拒绝；
4. 不应出现同一个 buffer 同时处于 `Writing` 和 `Submitted`；
5. OES 纹理更新完成后，画面不出现撕裂或随机帧；
6. 普通纹理仍然通过对象池复制，原始应用 buffer 可以在 `updateTexture2D()` 返回后立即复用；
7. 单线程模式保持现有同步行为；
8. RenderDeviceProxy 销毁时，未完成 OES buffer 能够被正确回收；
9. 应用保证每个 OES buffer 地址稳定，且 EGLImage 缓存有效期间不会将相同地址用于不同底层存储；
10. OES 更新、绘制、Fence 和回收的顺序可通过日志或测试钩子确认。

## 16. 已确认的设计决策

1. OES 完成回调使用 `gpuUseCompleteCallback`，不再使用含义不明确的 `releaseCallback`。
2. OES buffer 的 Free、Writing、Submitted 状态、应用侧对象池以及 buffer 复用策略全部由应用负责。引擎只在对应 GPU Fence 完成后调用 `gpuUseCompleteCallback`，不管理应用侧 buffer 队列。
3. 每个 `Texture` 对象每帧最多触发一次 OES `updateTexture2D`，不存在同一 Texture 同帧多次更新同一 buffer 的场景，不需要为此增加额外处理。
4. `PendingFrame` 使用专用的 OES frame recycle 类型保存本帧 OES 完成回调，不直接使用无语义区分的通用回调数组。
5. 不新增 OES 专用命令类型，继续复用现有 `Cmd_UpdateTexture2D`，由 `TextureData::imageType` 区分普通纹理和 OES 路径。
6. EGLImage 缓存和更新逻辑继续使用 `TextureData::pixels` 裸地址作为 key，不增加 `bufferID`；应用负责保证地址与底层外部 buffer 存储的对应关系稳定。
7. RenderDeviceProxy 关闭流程可以可靠地等待所有未完成 Fence；等待完成后再调用剩余的 `gpuUseCompleteCallback`，最后销毁 OES EGLImage 和纹理资源。

## 17. 结论

本设计将外部 OES 视频 buffer 与 GPU Fence 绑定，普通纹理继续走对象池拷贝路径：

```text
普通纹理：
应用 buffer -> 对象池拷贝 -> 命令执行 -> 对象池复用

OES 纹理：
应用外部 buffer -> EGLImage -> 当前帧绘制 -> GPU Fence 完成 -> 回调归还
```

这样可以保留 OES 视频流的零拷贝特性，同时避免应用在 GPU 仍读取外部 buffer 时提前复用它。
