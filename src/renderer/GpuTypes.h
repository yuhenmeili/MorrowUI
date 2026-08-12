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

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

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

    /// 重置所有字段，用于从对象池取出后复用
    void reset() {
        attributes.clear();
        vertexData.clear();
        indices.clear();
        vertexCount = 0;
        indexCount = 0;
        drawMode = PrimitiveType::TRIANGLES;
    }
};

using VBODataSharedPtr = std::shared_ptr<VBOData>;

// Fixed-function state that must be established immediately before a draw.
// Render-pass state such as framebuffer, viewport and clear values deliberately
// stays outside this structure.
struct GraphicsPipelineState {
    HwGPUProgram program{0};
    bool blendEnabled = true;
    BlendFactor srcRgbBlendFactor = BlendFactor::SRC_ALPHA;
    BlendFactor dstRgbBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
    BlendFactor dstAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = false;
    CullFaceMode cullFaceMode = CullFaceMode::NONE;

    bool isEqual(const GraphicsPipelineState& other) const {
        return blendEnabled == other.blendEnabled && srcRgbBlendFactor == other.srcRgbBlendFactor && dstRgbBlendFactor == other.dstRgbBlendFactor &&
               srcAlphaBlendFactor == other.srcAlphaBlendFactor && dstAlphaBlendFactor == other.dstAlphaBlendFactor && depthTestEnabled == other.depthTestEnabled &&
               depthWriteEnabled == other.depthWriteEnabled && cullFaceMode == other.cullFaceMode;
    }
};

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

    // Keeps CPU-side image data alive while a parsed GLTF scene is handed
    // from the loader thread to the render/main thread. Ordinary IMAGE
    // uploads are still copied by RenderDeviceProxy when needed; this is
    // Keeps ordinary CPU texture data alive until the upload command has
    // executed. This is intentionally separate from the OES GPU-fence
    // callback below.
    std::shared_ptr<void> cpuPixelOwner;

    // -----------------------------------------------------------------------
    // OES 外部 buffer 的 GPU 使用完成回调。
    //
    //  时机：对应渲染帧的 GPU Fence 完成后调用。
    //
    //  线程：由渲染线程调用，实现必须线程安全。
    //  重要：不要在回调中阻塞渲染线程。
    //        不要在回调中修改 TextureData 本身（该对象属于命令缓冲区）。
    // -----------------------------------------------------------------------
    std::function<void()> gpuUseCompleteCallback;
};
}  // namespace morrow
