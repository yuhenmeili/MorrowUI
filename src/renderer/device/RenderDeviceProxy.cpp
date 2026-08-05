//
// Filament-style: ring-buffer command queue + flat CommandBuffer payload encoding.
//
// Architecture improvements vs. the old double-buffer design:
//
//  1. RING BUFFER (N=3, async)
//     endFrame() signals the render thread and immediately advances to the
//     next write slot.  It blocks only when ALL slots are occupied (render
//     thread is >2 frames behind).  The old design blocked on EVERY frame.
//
//  2. ZERO-ALLOCATION COMMAND ENCODING
//     Trivial payloads are embedded inline in a pre-allocated flat buffer
//     via CommandBuffer::push<T>().  Non-trivial payloads (shared_ptr /
//     vector / string) use CommandBuffer::pushNT<T>() which calls
//     placement-new and registers a destructor for CommandBuffer::clear().
//     Either way there is no per-command heap allocation for trivial types.
//
//  3. static_cast INSTEAD OF dynamic_cast
//     All hot-path casts are static_cast because we own and control the
//     entire type hierarchy.  This eliminates RTTI lookup overhead.
//

#include "RenderDeviceProxy.h"

#include <cstring>

#include "Log.h"
#include "PixelFormat.h"
#include "RenderDevice.h"
#include "RenderDeviceProxyBase.h"

namespace morrow {
// ---------------------------------------------------------------------------
// Command-type enum
// ---------------------------------------------------------------------------
enum CommandType {
    Cmd_Unused = 10000,
    Cmd_MakeCurrent,
    Cmd_Present,
    Cmd_Clear,
    Cmd_BindPipelineState,
    Cmd_UseGPUProgram,
    Cmd_CreateGPUProgram,
    Cmd_DeleteGPUProgram,
    Cmd_CreateTexture2D,
    Cmd_DeleteTexture2D,
    Cmd_UseTexture2D,
    Cmd_UpdateTexture2D,
    Cmd_SetClearColor,
    Cmd_SetViewPort,
    Cmd_DumpFrameBuffer,
    Cmd_CreateVBO,
    Cmd_UpdateVBO,
    Cmd_DeleteVBO,
    Cmd_DrawVBO,
    Cmd_SetGPUProgramAsInt,
    Cmd_SetGPUProgramAsFloat,
    Cmd_SetGPUProgramAsVec2,
    Cmd_SetGPUProgramAsVec3,
    Cmd_SetGPUProgramAsVec4,
    Cmd_SetGPUProgramAsMat4,
    Cmd_SetGPUProgramAsIntArray,
    Cmd_SetGPUProgramAsFloatArray,
    Cmd_SetGPUProgramAsMat4Array,
    Cmd_CreateUBO,
    Cmd_UpdateUBO,
    Cmd_BindUBO,
    Cmd_CreateSSBO,
    Cmd_UpdateSSBO,
    Cmd_CheckSSBOSupport,
    Cmd_DebugDriver,
    Cmd_UpdateSubTexture2D,
    Cmd_EndFrame,
    Cmd_CreateRenderTarget,
    Cmd_DeleteRenderTarget,
    Cmd_BindRenderTarget,
    Cmd_UnbindRenderTarget,
    Cmd_ClearDepth,
    Cmd_Count
};

// ---------------------------------------------------------------------------
// Payload structs
//
// TRIVIAL  – contain only scalars / raw pointers / plain structs.
//            Embedded inline via CommandBuffer::push<T>().
// NON-TRIVIAL – contain std::string / std::vector / std::shared_ptr.
//               Embedded via CommandBuffer::pushNT<T>() (placement-new +
//               registered destructor).
// ---------------------------------------------------------------------------

// --- Trivial ---
struct InitContextPayload {
    void* window = nullptr;
};

struct BindPipelineStatePayload {
    GraphicsPipelineState state;
};

struct UseGPUProgramPayload {
    HwGPUProgram program{0};
};

struct DeleteGPUProgramPayload {
    HwGPUProgram program{0};
};

struct CreateTexture2DPayload {
    HwTexture2D texture{0};
    ImageType imageType = ImageType::IMAGE;
};

struct DeleteTexture2DPayload {
    HwTexture2D texture{0};
};

struct UseTexture2DPayload {
    HwTexture2D texture{0};
    uint32_t index = 0;
};

struct SetClearColorPayload {
    Vector4 color;
};

struct SetViewPortPayload {
    Vector4 v;
};

struct DumpFrameBufferPayload {
    int32_t x, y, displayWidth, displayHeight, rectX, rectY, rectWidth, rectHeight, comp;
};

struct CreateVBOPayload {
    HwVBO vbo{0};
};

struct DeleteVBOPayload {
    HwVBO vbo{0};
};

struct DrawVBOPayload {
    HwVBO vbo{0};
    int32_t instanceCount = 0;
};

// GPUProgramParam removed — all uniform setting now uses HwGPUProgram + name string
struct SetGPUProgramParamIntPayload {
    HwGPUProgram program{0};
    std::string uniformName;
    int32_t value = 0;
};

struct SetGPUProgramParamFloatPayload {
    HwGPUProgram program{0};
    std::string uniformName;
    float value = 0;
};

struct SetGPUProgramParamVec2Payload {
    HwGPUProgram program{0};
    std::string uniformName;
    float x = 0, y = 0;
};

struct SetGPUProgramParamVec3Payload {
    HwGPUProgram program{0};
    std::string uniformName;
    float x = 0, y = 0, z = 0;
};

struct SetGPUProgramParamVec4Payload {
    HwGPUProgram program{0};
    std::string uniformName;
    float x = 0, y = 0, z = 0, w = 0;
};

struct SetGPUProgramParamMat4Payload {
    HwGPUProgram program{0};
    std::string uniformName;
    Matrix4 mat;
};

struct CreateUBOPayload {
    HwUBO ubo{0};
};

struct CreateSSBOPayload {
    HwSSBO ssbo{0};
};

struct CheckSSBOSupportPayload {
    bool* result = nullptr;
};

// --- Non-trivial (have std::string / std::vector / std::shared_ptr members) ---
struct CreateGPUProgramPayload {
    HwGPUProgram program{0};
    std::string programFileName, vertexShader, fragmentShader;
};

struct UpdateTexture2DPayload {
    HwTexture2D texture{0};
    TextureData data;
    std::shared_ptr<std::vector<uint8_t>> pixelStorage;
};

struct UpdateSubTexture2DPayload {
    HwTexture2D texture{0};
    TextureData data;
    int32_t x = 0, y = 0, width = 0, height = 0;
    std::shared_ptr<std::vector<uint8_t>> pixelStorage;
};

struct UpdateVBOPayload {
    HwGPUProgram program{0};
    HwVBO vbo{0};
    std::shared_ptr<VBOData> vboData;
};

struct BindUBOPayload {
    HwGPUProgram program{0};
    HwUBO ubo{0};
    std::string blockName;
    uint32_t bindingPoint = 0;
};

struct UpdateUBOPayload {
    HwUBO ubo{0};
    std::shared_ptr<UBOData> uboData;
};

struct UpdateSSBOPayload {
    HwSSBO ssbo{0};
    std::shared_ptr<SSBOData> ssboData;
    uint32_t bindingPoint = 0;
};

// --- FBO / depth state payloads ---
struct CreateRenderTargetPayload {
    HwRenderTarget rt{0};
    HwTexture2D colorTexture{0};
    int32_t w = 0, h = 0;
};

struct BindRenderTargetPayload {
    HwRenderTarget rt{0};
};

struct DeleteRenderTargetPayload {
    HwRenderTarget rt{0};
};

struct SetGPUProgramParamIntArrayPayload {
    HwGPUProgram program{0};
    std::string uniformName;
    std::vector<int32_t> values;
    int32_t size = 0, step = 0;
};

struct SetGPUProgramParamFloatArrayPayload {
    HwGPUProgram program{0};
    std::string uniformName;
    std::vector<float> values;
    int32_t size = 0, step = 0;
};

struct SetGPUProgramParamMat4ArrayPayload {
    HwGPUProgram program{0};
    std::string uniformName;
    std::vector<Matrix4> values;
};

// ---------------------------------------------------------------------------
// Helpers – shorthand so encoding sites are one-liners
// ---------------------------------------------------------------------------
#define CMD_BUF m_commandBuffers[m_writeIdx]

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

RenderDeviceProxy::RenderDeviceProxy(PlatformSharedPtr platform, bool returnResImmediately) : RenderDeviceProxyBase(std::move(platform), returnResImmediately) {
    m_vboRecyclePool = std::make_unique<VBODataRecyclePool>();
    m_uboRecyclePool = std::make_unique<UBODataRecyclePool>();
    m_ssboRecyclePool = std::make_unique<SSBODataRecyclePool>();
    m_pixelDataRecyclePool = std::make_unique<PixelDataRecyclePool>();

    // Prime the free-slots semaphore so the main thread can be (kRingSize-1)
    // frames ahead without blocking.
    for (uint32_t i = 0; i < kRingSize - 1; ++i)
        m_freeSlotsSem.signal();
}

RenderDeviceProxy::~RenderDeviceProxy() = default;

// ---------------------------------------------------------------------------
// Context / window management
// ---------------------------------------------------------------------------

void RenderDeviceProxy::makeCurrent(void* window) {
    if (!m_threaded) {
        m_realDevice->makeCurrent(window);
        return;
    }
    auto* pl = CMD_BUF.push<InitContextPayload>(Cmd_MakeCurrent);
    pl->window = window;
}

void RenderDeviceProxy::present(void* window) {
    if (!m_threaded) {
        m_realDevice->present(window);
        return;
    }
    auto* pl = CMD_BUF.push<InitContextPayload>(Cmd_Present);
    pl->window = window;
}

void RenderDeviceProxy::debugDriver() {
    if (!m_threaded) {
        m_realDevice->debugDriver();
        return;
    }
    CMD_BUF.push(Cmd_DebugDriver);
}

void RenderDeviceProxy::clear() {
    if (!m_threaded) {
        m_realDevice->clear();
        return;
    }
    CMD_BUF.push(Cmd_Clear);
}

void RenderDeviceProxy::bindPipelineState(const GraphicsPipelineState& state) {
    if (!m_threaded) {
        m_realDevice->bindPipelineState(state);
        return;
    }
    auto* pl = CMD_BUF.push<BindPipelineStatePayload>(Cmd_BindPipelineState);
    pl->state = state;
}

// ---------------------------------------------------------------------------
// GPU program
// ---------------------------------------------------------------------------

void RenderDeviceProxy::useGPUProgram(HwGPUProgram program) {
    if (!m_threaded) {
        m_realDevice->useGPUProgram(program);
        return;
    }
    auto* pl = CMD_BUF.push<UseGPUProgramPayload>(Cmd_UseGPUProgram);
    pl->program = program;
}

HwGPUProgram RenderDeviceProxy::createGPUProgram(const std::string& programFileName, const std::string& vertexShader, const std::string& fragmentShader) {
    HwGPUProgram handle = m_realDevice->allocateGPUProgram();
    if (!m_threaded) {
        m_realDevice->commitGPUProgram(handle, programFileName, vertexShader, fragmentShader);
    } else {
        auto* pl = CMD_BUF.pushNT<CreateGPUProgramPayload>(Cmd_CreateGPUProgram);
        pl->program = handle;
        pl->programFileName = programFileName;
        pl->vertexShader = vertexShader;
        pl->fragmentShader = fragmentShader;
    }
    return handle;
}

void RenderDeviceProxy::deletGPUProgram(HwGPUProgram program) {
    if (!m_threaded) {
        m_realDevice->deletGPUProgram(program);
        return;
    }
    auto* pl = CMD_BUF.push<DeleteGPUProgramPayload>(Cmd_DeleteGPUProgram);
    pl->program = program;
}

// ---------------------------------------------------------------------------
// Texture2D
// ---------------------------------------------------------------------------

HwTexture2D RenderDeviceProxy::createTexture2D(ImageType imageType) {
    HwTexture2D handle = m_realDevice->allocateTexture2D();
    if (!m_threaded) {
        m_realDevice->commitTexture2D(handle, imageType);
        return handle;
    }
    auto* pl = CMD_BUF.push<CreateTexture2DPayload>(Cmd_CreateTexture2D);
    pl->texture = handle;
    pl->imageType = imageType;
    return handle;
}

void RenderDeviceProxy::deleteTexture2D(HwTexture2D texture) {
    if (!m_threaded) {
        m_realDevice->deleteTexture2D(texture);
        return;
    }
    auto* pl = CMD_BUF.push<DeleteTexture2DPayload>(Cmd_DeleteTexture2D);
    pl->texture = texture;
}

void RenderDeviceProxy::useTexture2D(HwTexture2D texture, uint32_t index) {
    if (!m_threaded) {
        m_realDevice->useTexture2D(texture, index);
        return;
    }
    auto* pl = CMD_BUF.push<UseTexture2DPayload>(Cmd_UseTexture2D);
    pl->texture = texture;
    pl->index = index;
}

bool RenderDeviceProxy::isTextureFormatSupported(PixelDataFormat textureFormat) {
    if (!m_threaded)
        return m_realDevice->isTextureFormatSupported(textureFormat);
    return true;
}

void RenderDeviceProxy::updateTexture2D(HwTexture2D texture, const TextureData& data) {
    if (!m_threaded) {
        m_realDevice->updateTexture2D(texture, data);
        if (data.releaseCallback)
            data.releaseCallback();
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateTexture2DPayload>(Cmd_UpdateTexture2D);
    pl->texture = texture;
    pl->data = data;
    if (!data.pixelOwner && data.pixels && data.bytes > 0 && data.imageType != ImageType::OES) {
        auto buf = m_pixelDataRecyclePool->acquire();
        buf->assign(static_cast<const uint8_t*>(data.pixels), static_cast<const uint8_t*>(data.pixels) + data.bytes);
        pl->data.pixels = buf->data();
        pl->pixelStorage = std::move(buf);
    }
    // 若 data.releaseCallback 非空，一并入队，渲染线程 GL 完成后回调。
    // 零拷贝路径：data.pixelOwner 持有内存，pl->data.pixels 已指向正确位置，无需额外操作。
}

void RenderDeviceProxy::updateSubTexture2D(HwTexture2D texture, const TextureData& data, int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData) {
    if (!m_threaded) {
        m_realDevice->updateSubTexture2D(texture, data, x, y, width, height, sourceData);
        if (data.releaseCallback)
            data.releaseCallback();
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateSubTexture2DPayload>(Cmd_UpdateSubTexture2D);
    pl->texture = texture;
    pl->data = data;
    pl->x = x;
    pl->y = y;
    pl->width = width;
    pl->height = height;
    const size_t byteSize = static_cast<size_t>(PixelFormat::textureSizeInBytes(data.format, GL_UNSIGNED_BYTE, width, height));
    if (sourceData && byteSize > 0) {
        auto buf = m_pixelDataRecyclePool->acquire();
        buf->assign(sourceData, sourceData + byteSize);
        pl->pixelStorage = std::move(buf);
    }
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void RenderDeviceProxy::setClearColor(float r, float g, float b, float alpha) {
    if (!m_threaded) {
        m_realDevice->setClearColor(r, g, b, alpha);
        return;
    }
    auto* pl = CMD_BUF.push<SetClearColorPayload>(Cmd_SetClearColor);
    pl->color = Vector4(r, g, b, alpha);
}

void RenderDeviceProxy::setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!m_threaded) {
        m_realDevice->setViewPort(x, y, width, height);
        return;
    }
    auto* pl = CMD_BUF.push<SetViewPortPayload>(Cmd_SetViewPort);
    pl->v = Vector4(float(x), float(y), float(width), float(height));
}

void RenderDeviceProxy::dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight, int32_t comp) {
    if (!m_threaded) {
        m_realDevice->dumpFrameBuffer(x, y, displayWidth, displayHeight, rectX, rectY, rectWidth, rectHeight, comp);
        return;
    }
    auto* pl = CMD_BUF.push<DumpFrameBufferPayload>(Cmd_DumpFrameBuffer);
    pl->x = x;
    pl->y = y;
    pl->displayWidth = displayWidth;
    pl->displayHeight = displayHeight;
    pl->rectX = rectX;
    pl->rectY = rectY;
    pl->rectWidth = rectWidth;
    pl->rectHeight = rectHeight;
    pl->comp = comp;
}

bool RenderDeviceProxy::checkSSBOSupport() {
    if (!m_threaded)
        return m_realDevice->checkSSBOSupport();

    bool result = false;
    auto* pl = CMD_BUF.push<CheckSSBOSupportPayload>(Cmd_CheckSSBOSupport);
    pl->result = &result;
    submitCurrentBufferAndAdvance();
    return result;
}

// ---------------------------------------------------------------------------
// VBO
// ---------------------------------------------------------------------------

HwVBO RenderDeviceProxy::createVBO() {
    HwVBO handle = m_realDevice->allocateVBO();
    if (!m_threaded) {
        m_realDevice->commitVBO(handle);
        return handle;
    }
    auto* pl = CMD_BUF.push<CreateVBOPayload>(Cmd_CreateVBO);
    pl->vbo = handle;
    return handle;
}

void RenderDeviceProxy::updateVBO(HwGPUProgram program, HwVBO vbo, VBODataSharedPtr vboData) {
    if (!m_threaded) {
        if (!program.isValid() || !vbo.isValid() || !vboData) {
            LOG_E("RenderDeviceProxy::updateVBO received an invalid argument");
            return;
        }
        m_realDevice->updateVBO(program, vbo, vboData);
        m_currentFrameRecyclables.push_back(std::move(vboData));
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateVBOPayload>(Cmd_UpdateVBO);
    pl->program = program;
    pl->vbo = vbo;
    pl->vboData = std::move(vboData);
}

void RenderDeviceProxy::deleteVBO(HwVBO vbo) {
    if (!m_threaded) {
        if (!vbo.isValid()) {
            LOG_E("RenderDeviceProxy::deleteVBO received an invalid VBO");
            return;
        }
        m_realDevice->deleteVBO(vbo);
        return;
    }
    auto* pl = CMD_BUF.push<DeleteVBOPayload>(Cmd_DeleteVBO);
    pl->vbo = vbo;
}

void RenderDeviceProxy::drawVBO(HwVBO vbo, int32_t instanceCount) {
    if (!m_threaded) {
        if (!vbo.isValid()) {
            LOG_E("RenderDeviceProxy::drawVBO received an invalid VBO");
            return;
        }
        m_realDevice->drawVBO(vbo, instanceCount);
        return;
    }
    auto* pl = CMD_BUF.push<DrawVBOPayload>(Cmd_DrawVBO);
    pl->vbo = vbo;
    pl->instanceCount = instanceCount;
}

// ---------------------------------------------------------------------------
// GPU program uniform params (name-based only; GPUProgramParam removed)
// ---------------------------------------------------------------------------

void RenderDeviceProxy::setGPUProgramParamAsInt(HwGPUProgram program, const std::string& uniformName, int32_t value) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsInt(program, uniformName, value);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamIntPayload>(Cmd_SetGPUProgramAsInt);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->value = value;
}

void RenderDeviceProxy::setGPUProgramParamAsFloat(HwGPUProgram program, const std::string& uniformName, float value) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsFloat(program, uniformName, value);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamFloatPayload>(Cmd_SetGPUProgramAsFloat);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->value = value;
}

void RenderDeviceProxy::setGPUProgramParamAsVec2(HwGPUProgram program, const std::string& uniformName, float x, float y) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsVec2(program, uniformName, x, y);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamVec2Payload>(Cmd_SetGPUProgramAsVec2);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->x = x;
    pl->y = y;
}

void RenderDeviceProxy::setGPUProgramParamAsVec3(HwGPUProgram program, const std::string& uniformName, float x, float y, float z) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsVec3(program, uniformName, x, y, z);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamVec3Payload>(Cmd_SetGPUProgramAsVec3);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->x = x;
    pl->y = y;
    pl->z = z;
}

void RenderDeviceProxy::setGPUProgramParamAsVec4(HwGPUProgram program, const std::string& uniformName, float x, float y, float z, float w) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsVec4(program, uniformName, x, y, z, w);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamVec4Payload>(Cmd_SetGPUProgramAsVec4);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->x = x;
    pl->y = y;
    pl->z = z;
    pl->w = w;
}

void RenderDeviceProxy::setGPUProgramParamAsMat4(HwGPUProgram program, const std::string& uniformName, const Matrix4& mat) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsMat4(program, uniformName, mat);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamMat4Payload>(Cmd_SetGPUProgramAsMat4);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->mat = mat;
}

void RenderDeviceProxy::setGPUProgramParamAsIntArray(HwGPUProgram program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsIntArray(program, uniformName, values, size, step);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamIntArrayPayload>(Cmd_SetGPUProgramAsIntArray);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->size = size;
    pl->step = step;
    const int32_t n = (size > 0 && step > 0) ? size * step : 0;
    if (values && n > 0)
        pl->values.assign(values, values + n);
}

void RenderDeviceProxy::setGPUProgramParamAsFloatArray(HwGPUProgram program, const std::string& uniformName, const float* values, int32_t size, int32_t step) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsFloatArray(program, uniformName, values, size, step);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamFloatArrayPayload>(Cmd_SetGPUProgramAsFloatArray);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->size = size;
    pl->step = step;
    const int32_t n = (size > 0 && step > 0) ? size * step : 0;
    if (values && n > 0)
        pl->values.assign(values, values + n);
}

void RenderDeviceProxy::setGPUProgramParamAsMat4Array(HwGPUProgram program, const std::string& uniformName, const std::vector<Matrix4>& values) {
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsMat4Array(program, uniformName, values);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamMat4ArrayPayload>(Cmd_SetGPUProgramAsMat4Array);
    pl->program = program;
    pl->uniformName = uniformName;
    pl->values = values;
}

// ---------------------------------------------------------------------------
// UBO
// ---------------------------------------------------------------------------

HwUBO RenderDeviceProxy::createUBO() {
    HwUBO handle = m_realDevice->allocateUBO();
    if (!m_threaded) {
        m_realDevice->commitUBO(handle);
        return handle;
    }
    auto* pl = CMD_BUF.push<CreateUBOPayload>(Cmd_CreateUBO);
    pl->ubo = handle;
    return handle;
}

void RenderDeviceProxy::updateUBO(HwUBO ubo, std::shared_ptr<UBOData> uboData) {
    if (!m_threaded) {
        m_realDevice->updateUBO(ubo, uboData);
        m_currentFrameUBORecyclables.push_back(std::move(uboData));
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateUBOPayload>(Cmd_UpdateUBO);
    pl->ubo = ubo;
    pl->uboData = std::move(uboData);
}

void RenderDeviceProxy::bindUBO(HwGPUProgram program, HwUBO ubo, const std::string& blockName, uint32_t bindingPoint) {
    if (!m_threaded) {
        m_realDevice->bindUBO(program, ubo, blockName, bindingPoint);
        return;
    }
    auto* pl = CMD_BUF.pushNT<BindUBOPayload>(Cmd_BindUBO);
    pl->program = program;
    pl->ubo = ubo;
    pl->blockName = blockName;
    pl->bindingPoint = bindingPoint;
}

// ---------------------------------------------------------------------------
// SSBO
// ---------------------------------------------------------------------------

HwSSBO RenderDeviceProxy::createSSBO() {
    HwSSBO handle = m_realDevice->allocateSSBO();
    if (!m_threaded) {
        m_realDevice->commitSSBO(handle);
        return handle;
    }
    auto* pl = CMD_BUF.push<CreateSSBOPayload>(Cmd_CreateSSBO);
    pl->ssbo = handle;
    return handle;
}

void RenderDeviceProxy::updateSSBO(HwSSBO ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) {
    if (!m_threaded) {
        m_realDevice->updateSSBO(ssbo, ssboData, bindingPoint);
        m_currentFrameSSBORecyclables.push_back(std::move(ssboData));
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateSSBOPayload>(Cmd_UpdateSSBO);
    pl->ssbo = ssbo;
    pl->ssboData = std::move(ssboData);
    pl->bindingPoint = bindingPoint;
}

// ---------------------------------------------------------------------------
// Fence (passthrough – always executed on render thread for multithreaded)
// ---------------------------------------------------------------------------

void* RenderDeviceProxy::insertFence() {
    // Fences are only meaningful on the GL thread; proxy always delegates.
    return m_realDevice->insertFence();
}

bool RenderDeviceProxy::waitFence(void* fence, uint64_t timeoutNs) {
    return m_realDevice->waitFence(fence, timeoutNs);
}

void RenderDeviceProxy::deleteFence(void* fence) {
    m_realDevice->deleteFence(fence);
}

// ---------------------------------------------------------------------------
// FBO (RenderTarget)
// ---------------------------------------------------------------------------

HwRenderTarget RenderDeviceProxy::createRenderTarget(int32_t w, int32_t h, HwTexture2D colorTexture) {
    const HwRenderTarget rtHandle = m_realDevice->allocateRenderTarget();

    if (!m_threaded) {
        m_realDevice->commitRenderTarget(rtHandle, w, h, colorTexture);
        return rtHandle;
    }

    auto* pl = CMD_BUF.push<CreateRenderTargetPayload>(Cmd_CreateRenderTarget);
    pl->rt = rtHandle;
    pl->colorTexture = colorTexture;
    pl->w = w;
    pl->h = h;
    return rtHandle;
}

void RenderDeviceProxy::deleteRenderTarget(HwRenderTarget rt) {
    if (!m_threaded) {
        m_realDevice->deleteRenderTarget(rt);
        return;
    }
    auto* pl = CMD_BUF.push<DeleteRenderTargetPayload>(Cmd_DeleteRenderTarget);
    pl->rt = rt;
}

void RenderDeviceProxy::bindRenderTarget(HwRenderTarget rt) {
    if (!m_threaded) {
        m_realDevice->bindRenderTarget(rt);
        return;
    }
    auto* pl = CMD_BUF.push<BindRenderTargetPayload>(Cmd_BindRenderTarget);
    pl->rt = rt;
}

void RenderDeviceProxy::unbindRenderTarget() {
    if (!m_threaded) {
        m_realDevice->unbindRenderTarget();
        return;
    }
    CMD_BUF.push(Cmd_UnbindRenderTarget);
}

// ---------------------------------------------------------------------------
// Depth / rasteriser state
// ---------------------------------------------------------------------------

void RenderDeviceProxy::clearDepth() {
    if (!m_threaded) {
        m_realDevice->clearDepth();
        return;
    }
    CMD_BUF.push(Cmd_ClearDepth);
}

// ---------------------------------------------------------------------------
// Frame control
// ---------------------------------------------------------------------------

void RenderDeviceProxy::submitCurrentBufferAndAdvance() {
    m_frameReadySem.signal();
    m_freeSlotsSem.waitForSignal();

    const uint32_t nextIdx = (m_writeIdx + 1) % kRingSize;
    m_commandBuffers[nextIdx].clear();
    m_writeIdx = nextIdx;
}

void RenderDeviceProxy::endFrame() {
    if (!m_threaded) {
        m_pendingFrames.push({m_realDevice->insertFence(), std::move(m_currentFrameRecyclables), std::move(m_currentFrameUBORecyclables), std::move(m_currentFrameSSBORecyclables)});
        tryRecycle();
        return;
    }

    // Write the end-of-frame sentinel.
    CMD_BUF.push(Cmd_EndFrame);
    submitCurrentBufferAndAdvance();
}

void RenderDeviceProxy::tryRecycle() {
    while (!m_pendingFrames.empty()) {
        PendingFrame& frame = m_pendingFrames.front();
        if (m_realDevice->waitFence(frame.fence, 0)) {
            if (m_vboRecyclePool && !frame.vboRecyclables.empty())
                m_vboRecyclePool->release(std::move(frame.vboRecyclables));
            if (m_uboRecyclePool && !frame.uboRecyclables.empty())
                m_uboRecyclePool->release(std::move(frame.uboRecyclables));
            if (m_ssboRecyclePool && !frame.ssboRecyclables.empty())
                m_ssboRecyclePool->release(std::move(frame.ssboRecyclables));
            m_realDevice->deleteFence(frame.fence);
            m_pendingFrames.pop();
        } else {
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Render thread entry point
// ---------------------------------------------------------------------------

void RenderDeviceProxy::runCommand() {
    LOG_I("start multithreaded render (ring-buffer, kRingSize={})", kRingSize);

    uint32_t readIdx = 0;
    while (!isQuit()) {
        // Wait until the main thread has submitted a frame.
        m_frameReadySem.waitForSignal();
        if (isQuit())
            break;

        executeFrame(m_commandBuffers[readIdx]);

        // 像素缓冲立即归还：glTexImage2D/glTexSubImage2D 均为同步调用，
        // 驱动已完成数据拷贝，无需等待 GPU Fence，比 VBO 回收更快。
        if (m_pixelDataRecyclePool && !m_framePixelRecyclables.empty())
            m_pixelDataRecyclePool->release(std::move(m_framePixelRecyclables));
        m_framePixelRecyclables.clear();

        // Insert fence and book-keep recyclables collected during executeFrame.
        m_pendingFrames.push({m_realDevice->insertFence(), std::move(m_frameRecyclables), std::move(m_frameUBORecyclables), std::move(m_frameSSBORecyclables)});
        tryRecycle();

        // Advance read cursor and release a ring slot to the main thread.
        readIdx = (readIdx + 1) % kRingSize;
        m_freeSlotsSem.signal();
    }
}

void RenderDeviceProxy::signalThreadToExit() {
    // Wake render thread so it can observe isQuit().
    m_frameReadySem.signal();
    // Unblock main thread if it is waiting for a free ring slot.
    m_freeSlotsSem.signal();
}

// ---------------------------------------------------------------------------
// Command execution (render-thread only)
// ---------------------------------------------------------------------------

void RenderDeviceProxy::executeFrame(CommandBuffer& buf) {
    m_frameRecyclables.clear();
    m_frameUBORecyclables.clear();
    m_frameSSBORecyclables.clear();
    m_framePixelRecyclables.clear();

    uint8_t* ptr = buf.mutable_data();
    const uint8_t* end = ptr + buf.size();

    while (ptr < end) {
        auto* h = reinterpret_cast<CommandBuffer::CmdHeader*>(ptr);
        void* p = ptr + sizeof(CommandBuffer::CmdHeader);

        if (h->type == Cmd_EndFrame)
            break;

        switch (h->type) {
            case Cmd_MakeCurrent: {
                auto* pl = static_cast<InitContextPayload*>(p);
                m_realDevice->makeCurrent(pl->window);
                break;
            }
            case Cmd_Present: {
                auto* pl = static_cast<InitContextPayload*>(p);
                m_realDevice->present(pl->window);
                break;
            }
            case Cmd_Clear:
                m_realDevice->clear();
                break;
            case Cmd_BindPipelineState: {
                auto* pl = static_cast<BindPipelineStatePayload*>(p);
                m_realDevice->bindPipelineState(pl->state);
                break;
            }
            case Cmd_UseGPUProgram: {
                auto* pl = static_cast<UseGPUProgramPayload*>(p);
                if (pl->program.isValid())
                    m_realDevice->useGPUProgram(pl->program);
                break;
            }
            case Cmd_CreateGPUProgram: {
                auto* pl = static_cast<CreateGPUProgramPayload*>(p);
                if (pl->program.isValid())
                    m_realDevice->commitGPUProgram(pl->program, pl->programFileName, pl->vertexShader, pl->fragmentShader);
                break;
            }
            case Cmd_DeleteGPUProgram: {
                auto* pl = static_cast<DeleteGPUProgramPayload*>(p);
                if (pl->program.isValid())
                    m_realDevice->deletGPUProgram(pl->program);
                break;
            }
            case Cmd_CreateTexture2D: {
                auto* pl = static_cast<CreateTexture2DPayload*>(p);
                if (pl->texture.isValid())
                    m_realDevice->commitTexture2D(pl->texture, pl->imageType);
                break;
            }
            case Cmd_DeleteTexture2D: {
                auto* pl = static_cast<DeleteTexture2DPayload*>(p);
                if (pl->texture.isValid())
                    m_realDevice->deleteTexture2D(pl->texture);
                break;
            }
            case Cmd_UseTexture2D: {
                auto* pl = static_cast<UseTexture2DPayload*>(p);
                if (pl->texture.isValid())
                    m_realDevice->useTexture2D(pl->texture, pl->index);
                break;
            }
            case Cmd_UpdateTexture2D: {
                auto* pl = static_cast<UpdateTexture2DPayload*>(p);
                if (pl->texture.isValid()) {
                    m_realDevice->updateTexture2D(pl->texture, pl->data);
                    if (pl->data.releaseCallback)
                        pl->data.releaseCallback();
                    if (pl->pixelStorage)
                        m_framePixelRecyclables.push_back(std::move(pl->pixelStorage));
                }
                break;
            }
            case Cmd_UpdateSubTexture2D: {
                auto* pl = static_cast<UpdateSubTexture2DPayload*>(p);
                if (pl->texture.isValid() && pl->pixelStorage) {
                    m_realDevice->updateSubTexture2D(pl->texture, pl->data, pl->x, pl->y, pl->width, pl->height, pl->pixelStorage->data());
                    if (pl->data.releaseCallback)
                        pl->data.releaseCallback();
                    m_framePixelRecyclables.push_back(std::move(pl->pixelStorage));
                }
                break;
            }
            case Cmd_SetClearColor: {
                auto* pl = static_cast<SetClearColorPayload*>(p);
                m_realDevice->setClearColor(pl->color.x, pl->color.y, pl->color.z, pl->color.w);
                break;
            }
            case Cmd_SetViewPort: {
                auto* pl = static_cast<SetViewPortPayload*>(p);
                m_realDevice->setViewPort(static_cast<int32_t>(pl->v.x), static_cast<int32_t>(pl->v.y), static_cast<int32_t>(pl->v.z), static_cast<int32_t>(pl->v.w));
                break;
            }
            case Cmd_DumpFrameBuffer: {
                auto* pl = static_cast<DumpFrameBufferPayload*>(p);
                m_realDevice->dumpFrameBuffer(pl->x, pl->y, pl->displayWidth, pl->displayHeight, pl->rectX, pl->rectY, pl->rectWidth, pl->rectHeight, pl->comp);
                break;
            }
            case Cmd_CreateVBO: {
                auto* pl = static_cast<CreateVBOPayload*>(p);
                if (pl->vbo.isValid())
                    m_realDevice->commitVBO(pl->vbo);
                break;
            }
            case Cmd_UpdateVBO: {
                auto* pl = static_cast<UpdateVBOPayload*>(p);
                if (pl->program.isValid() && pl->vbo.isValid() && pl->vboData) {
                    m_realDevice->updateVBO(pl->program, pl->vbo, pl->vboData);
                    m_frameRecyclables.push_back(pl->vboData);
                }
                break;
            }
            case Cmd_DeleteVBO: {
                auto* pl = static_cast<DeleteVBOPayload*>(p);
                if (pl->vbo.isValid())
                    m_realDevice->deleteVBO(pl->vbo);
                break;
            }
            case Cmd_DrawVBO: {
                auto* pl = static_cast<DrawVBOPayload*>(p);
                if (pl->vbo.isValid())
                    m_realDevice->drawVBO(pl->vbo, pl->instanceCount);
                break;
            }
            case Cmd_SetGPUProgramAsInt: {
                auto* pl = static_cast<SetGPUProgramParamIntPayload*>(p);
                if (pl->program.isValid())
                    m_realDevice->setGPUProgramParamAsInt(pl->program, pl->uniformName, pl->value);
                break;
            }
            case Cmd_SetGPUProgramAsFloat: {
                auto* pl = static_cast<SetGPUProgramParamFloatPayload*>(p);
                if (pl->program.isValid())
                    m_realDevice->setGPUProgramParamAsFloat(pl->program, pl->uniformName, pl->value);
                break;
            }
            case Cmd_SetGPUProgramAsVec2: {
                auto* pl = static_cast<SetGPUProgramParamVec2Payload*>(p);
                if (pl->program.isValid())
                    m_realDevice->setGPUProgramParamAsVec2(pl->program, pl->uniformName, pl->x, pl->y);
                break;
            }
            case Cmd_SetGPUProgramAsVec3: {
                auto* pl = static_cast<SetGPUProgramParamVec3Payload*>(p);
                if (pl->program.isValid())
                    m_realDevice->setGPUProgramParamAsVec3(pl->program, pl->uniformName, pl->x, pl->y, pl->z);
                break;
            }
            case Cmd_SetGPUProgramAsVec4: {
                auto* pl = static_cast<SetGPUProgramParamVec4Payload*>(p);
                if (pl->program.isValid())
                    m_realDevice->setGPUProgramParamAsVec4(pl->program, pl->uniformName, pl->x, pl->y, pl->z, pl->w);
                break;
            }
            case Cmd_SetGPUProgramAsMat4: {
                auto* pl = static_cast<SetGPUProgramParamMat4Payload*>(p);
                if (pl->program.isValid())
                    m_realDevice->setGPUProgramParamAsMat4(pl->program, pl->uniformName, pl->mat);
                break;
            }
            case Cmd_SetGPUProgramAsIntArray: {
                auto* pl = static_cast<SetGPUProgramParamIntArrayPayload*>(p);
                if (pl->program.isValid() && pl->size > 0 && !pl->values.empty())
                    m_realDevice->setGPUProgramParamAsIntArray(pl->program, pl->uniformName, pl->values.data(), pl->size, pl->step);
                break;
            }
            case Cmd_SetGPUProgramAsFloatArray: {
                auto* pl = static_cast<SetGPUProgramParamFloatArrayPayload*>(p);
                if (pl->program.isValid() && pl->size > 0 && !pl->values.empty())
                    m_realDevice->setGPUProgramParamAsFloatArray(pl->program, pl->uniformName, pl->values.data(), pl->size, pl->step);
                break;
            }
            case Cmd_SetGPUProgramAsMat4Array: {
                auto* pl = static_cast<SetGPUProgramParamMat4ArrayPayload*>(p);
                if (pl->program.isValid() && !pl->values.empty())
                    m_realDevice->setGPUProgramParamAsMat4Array(pl->program, pl->uniformName, pl->values);
                break;
            }
            case Cmd_CreateUBO: {
                auto* pl = static_cast<CreateUBOPayload*>(p);
                if (pl->ubo.isValid())
                    m_realDevice->commitUBO(pl->ubo);
                break;
            }
            case Cmd_UpdateUBO: {
                auto* pl = static_cast<UpdateUBOPayload*>(p);
                if (pl->ubo.isValid() && pl->uboData) {
                    m_realDevice->updateUBO(pl->ubo, pl->uboData);
                    m_frameUBORecyclables.push_back(pl->uboData);
                }
                break;
            }
            case Cmd_BindUBO: {
                auto* pl = static_cast<BindUBOPayload*>(p);
                if (pl->program.isValid() && pl->ubo.isValid())
                    m_realDevice->bindUBO(pl->program, pl->ubo, pl->blockName, pl->bindingPoint);
                break;
            }
            case Cmd_CreateSSBO: {
                auto* pl = static_cast<CreateSSBOPayload*>(p);
                if (pl->ssbo.isValid())
                    m_realDevice->commitSSBO(pl->ssbo);
                break;
            }
            case Cmd_UpdateSSBO: {
                auto* pl = static_cast<UpdateSSBOPayload*>(p);
                if (pl->ssbo.isValid() && pl->ssboData) {
                    m_realDevice->updateSSBO(pl->ssbo, pl->ssboData, pl->bindingPoint);
                    m_frameSSBORecyclables.push_back(pl->ssboData);
                }
                break;
            }
            case Cmd_CheckSSBOSupport: {
                auto* pl = static_cast<CheckSSBOSupportPayload*>(p);
                if (pl->result) {
                    *pl->result = m_realDevice->checkSSBOSupport();
                }
                break;
            }
            case Cmd_DebugDriver:
                m_realDevice->debugDriver();
                break;
            case Cmd_CreateRenderTarget: {
                auto* pl = static_cast<CreateRenderTargetPayload*>(p);
                if (pl->rt.isValid() && pl->colorTexture.isValid()) {
                    m_realDevice->commitRenderTarget(pl->rt, pl->w, pl->h, pl->colorTexture);
                }
                break;
            }
            case Cmd_DeleteRenderTarget: {
                auto* pl = static_cast<DeleteRenderTargetPayload*>(p);
                if (pl->rt.isValid())
                    m_realDevice->deleteRenderTarget(pl->rt);
                break;
            }
            case Cmd_BindRenderTarget: {
                auto* pl = static_cast<BindRenderTargetPayload*>(p);
                if (pl->rt.isValid())
                    m_realDevice->bindRenderTarget(pl->rt);
                break;
            }
            case Cmd_UnbindRenderTarget:
                m_realDevice->unbindRenderTarget();
                break;
            case Cmd_ClearDepth:
                m_realDevice->clearDepth();
                break;
            default:
                break;
        }

        ptr += sizeof(CommandBuffer::CmdHeader) + CommandBuffer::align8(h->payloadSize);
    }
}

#undef CMD_BUF
}  // namespace morrow
