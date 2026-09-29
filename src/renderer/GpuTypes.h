// GpuTypes.h — GPU 资源 DTO 与管线状态（引擎内部，实现代码专用）。
// 公共面的纹理数据入口是 Texture 自身 API；本头不对外。
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "morrow/DriverEnums.h"
#include "morrow/GlobalDefine.h"
#include "morrow/ResourceHandle.h"
#include "Matrix4.h"

namespace morrow {
using namespace Math;

// VBO 上传数据：顶点布局 + 顶点字节流 + 索引（GLTF 公共数据模型使用）。
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

// Fixed-function state that must be established immediately before a draw.
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

    // 持有 CPU 侧图像数据的生存期（GLTF 场景跨线程交接 / 上传命令执行前）。
    std::shared_ptr<void> cpuPixelOwner;

    // -----------------------------------------------------------------------
    // OES 外部 buffer 的 GPU 使用完成回调。
    //  时机：对应渲染帧的 GPU Fence 完成后调用（渲染线程调用，须线程安全）。
    // -----------------------------------------------------------------------
    std::function<void()> gpuUseCompleteCallback;
};

}  // namespace morrow
