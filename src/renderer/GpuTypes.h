// GpuTypes.h
//
// GPU 资源句柄（Handle）与 CPU↔GPU 数据传输结构（DTO）的统一定义。
//
// 职责划分：
//   GpuTypes.h    — 资源句柄 + 数据结构（无设备操作，可被 UI / Texture / VertexArray 等轻量包含）
//   RenderDevice.h — 纯虚设备接口（包含本文件；仅需调用 GPU 操作的模块才引入）
//
// 依赖：BatchDataDefine.h, DriverEnums.h, Matrix4.h — 均无重量级头文件。

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

#include "BatchDataDefine.h"
#include "DriverEnums.h"
#include "GlobalDefine.h"
#include "Matrix4.h"
#include "ResourceHandle.h"

namespace morrow {
using namespace Math;

// ---------------------------------------------------------------------------
// 前向声明
// ---------------------------------------------------------------------------
class RenderDevice;
class GLRenderDevice;

// ---------------------------------------------------------------------------
// CPU↔GPU 数据传输对象（DTO）
// ---------------------------------------------------------------------------

// UBO 上传数据：任意 uniform block 原始字节
struct UBOData {
    std::vector<uint8_t> data;
    uint32_t size = 0;
};

// SSBO 上传数据：可变长度的 shader storage block 数据。
class SSBOData {
public:
    std::vector<uint8_t> data;
    uint32_t size = 0;
};

// VBO 上传数据：顶点布局 + 顶点字节流 + 索引。
struct VBOData {
    std::vector<VertexAttribute> attributes;
    std::vector<uint8_t> vertexData;
    std::vector<uint32_t> indices;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    PrimitiveType drawMode{};

    void* getAttributeData(VertexAttributeType type) {
        for (const auto& attr : attributes) {
            if (attr.type == type)
                return vertexData.data() + attr.offset;
        }
        return nullptr;
    }
};

using VBODataSharedPtr = std::shared_ptr<VBOData>;

// 纹理上传数据。
struct TextureData {
    ImageType imageType = ImageType::IMAGE;
    int32_t width = 0;
    int32_t height = 0;
    PixelDataFormat format = PixelDataFormat::RGBA;
    int32_t bytes = 0;
    bool compressedTexture = false;
    SamplerMinFilter minFilterType = SamplerMinFilter::LINEAR;
    SamplerMagFilter magFilterType = SamplerMagFilter::LINEAR;
    void* pixels = nullptr;

    // -----------------------------------------------------------------------
    // 零拷贝所有权（可选）
    //   设置后，多线程渲染路径直接在 payload 中共享此 shared_ptr，
    //   不再 memcpy 全部像素；pixels 仍指向同一块内存。
    //   未设置时退化为 PixelDataRecyclePool 复用缓冲区的拷贝路径。
    // -----------------------------------------------------------------------
    std::shared_ptr<void> pixelOwner;

    // -----------------------------------------------------------------------
    // 缓冲释放回调（推荐用于：视频流帧 / OES 外部纹理 / 应用内存池 buffer）
    //
    //  时机：渲染线程在 glTexImage2D / glTexSubImage2D 同步完成后立即调用，
    //        此时 GL 驱动已完成像素数据拷贝，像素内存可安全复用或释放。
    //
    //  线程：由渲染线程调用，实现必须线程安全。推荐方式：
    //    ① 原子引用计数 / semaphore.signal()  → 通知池可复用该 buffer
    //    ② 无锁队列 push(buffer)             → 生产者-消费者归还
    //    ③ shared_ptr 池：自定义 deleter 归还（无需显式 callback）
    //
    //  重要：不要在回调中阻塞渲染线程（会导致帧率下降）。
    //        不要在回调中修改 TextureData 本身（该对象属于命令缓冲区）。
    // -----------------------------------------------------------------------
    std::function<void()> releaseCallback;
};
} // namespace morrow
