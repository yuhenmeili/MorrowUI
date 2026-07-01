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
#include "RenderDevice.h"
#include "RenderDeviceProxyBase.h"
#include "PixelFormat.h"
#include <cstring>

namespace morrow {
// ---------------------------------------------------------------------------
// Command-type enum
// ---------------------------------------------------------------------------
enum CommandType {
    Cmd_Unused = 10000,
    Cmd_MakeCurrent,
    Cmd_Present,
    Cmd_Clear,
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
    Cmd_EnableBlend,
    Cmd_DisableBlend,
    Cmd_SetGPUProgramAsInt,
    Cmd_SetGPUProgramAsFloat,
    Cmd_SetGPUProgramAsVec2,
    Cmd_SetGPUProgramAsVec3,
    Cmd_SetGPUProgramAsVec4,
    Cmd_SetGPUProgramAsMat4,
    Cmd_SetGPUProgramAsIntArray,
    Cmd_SetGPUProgramAsFloatArray,
    Cmd_SetGPUProgramAsMat4Array,
    Cmd_InitThreadGPUProgramParam,
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
    Cmd_SetDepthTest,
    Cmd_SetDepthWrite,
    Cmd_SetCullFace,
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

struct UseGPUProgramPayload {
    GPUProgramHandle* program = nullptr;
};

struct DeleteGPUProgramPayload {
    GPUProgramHandle* program = nullptr;
};

struct CreateTexture2DPayload {
    Texture2DHandle* texture = nullptr;
    ImageType imageType = ImageType::IMAGE;
};

struct DeleteTexture2DPayload {
    Texture2DHandle* texture = nullptr;
};

struct UseTexture2DPayload {
    Texture2DHandle* texture = nullptr;
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
    VBOHandle* vbo = nullptr;
};

struct DeleteVBOPayload {
    VBOHandle* vbo = nullptr;
};

struct DrawVBOPayload {
    VBOHandle* vbo = nullptr;
    int32_t instanceCount = 0;
};

struct SetGPUProgramParamIntPayload {
    GPUProgramParamHandle* param = nullptr;
    int32_t value = 0;
};

struct SetGPUProgramParamFloatPayload {
    GPUProgramParamHandle* param = nullptr;
    float value = 0;
};

struct SetGPUProgramParamVec2Payload {
    GPUProgramParamHandle* param = nullptr;
    float x = 0, y = 0;
};

struct SetGPUProgramParamVec3Payload {
    GPUProgramParamHandle* param = nullptr;
    float x = 0, y = 0, z = 0;
};

struct SetGPUProgramParamVec4Payload {
    GPUProgramParamHandle* param = nullptr;
    float x = 0, y = 0, z = 0, w = 0;
};

struct SetGPUProgramParamMat4Payload {
    GPUProgramParamHandle* param = nullptr;
    Matrix4 mat;
};

struct CreateUBOPayload {
    UBOHandle* ubo = nullptr;
};

struct CreateSSBOPayload {
    SSBOHandle* ssbo = nullptr;
};

struct CheckSSBOSupportPayload {
    bool* result = nullptr;
};

// --- Non-trivial (have std::string / std::vector / std::shared_ptr members) ---
struct CreateGPUProgramPayload {
    GPUProgramHandle* program = nullptr;
    std::string programFileName, vertexShader, fragmentShader;
};

struct UpdateTexture2DPayload {
    Texture2DHandle* texture = nullptr;
    TextureData data;
    // 零拷贝路径：data.pixelOwner 非空时，此字段为空，data.pixels 直接指向共享内存。
    // 拷贝路径：data.pixelOwner 为空时，从 PixelDataRecyclePool 取得缓冲并存于此；
    //          executeFrame 结束后立即归还池（glTexImage2D 为同步调用，无需等 fence）。
    std::shared_ptr<std::vector<uint8_t>> pixelStorage;
};

struct UpdateSubTexture2DPayload {
    Texture2DHandle* texture = nullptr;
    TextureData data;
    int32_t x = 0, y = 0, width = 0, height = 0;
    std::shared_ptr<std::vector<uint8_t>> pixelStorage; // 同上，总是拷贝路径
};

struct UpdateVBOPayload {
    GPUProgramHandle* program = nullptr;
    VBOHandle* vbo = nullptr;
    std::shared_ptr<VBOData> vboData;
};

struct BindUBOPayload {
    GPUProgramHandle* program = nullptr;
    UBOHandle* ubo = nullptr;
    std::string blockName;
    uint32_t bindingPoint = 0;
};

struct UpdateUBOPayload {
    UBOHandle* ubo = nullptr;
    std::shared_ptr<UBOData> uboData;
};

struct UpdateSSBOPayload {
    SSBOHandle* ssbo = nullptr;
    std::shared_ptr<SSBOData> ssboData;
    uint32_t bindingPoint = 0;
};

// --- FBO / depth state payloads ---
struct CreateRenderTargetPayload {
    RenderTargetHandle* rt       = nullptr;
    Texture2DHandle*    colorTex = nullptr;  // optional: filled by render thread
    int32_t w = 0, h = 0;
};

struct BindRenderTargetPayload {
    RenderTargetHandle* rt = nullptr;
};

struct DeleteRenderTargetPayload {
    RenderTargetHandle* rt = nullptr;
};

struct SetDepthTestPayload {
    bool enable = false;
};

struct SetDepthWritePayload {
    bool enable = false;
};

struct SetCullFacePayload {
    CullFaceMode mode = CullFaceMode::NONE;
};

struct SetGPUProgramParamIntArrayPayload {
    GPUProgramParamHandle* param = nullptr;
    std::vector<int32_t> values;
    int32_t size = 0, step = 0;
};

struct SetGPUProgramParamFloatArrayPayload {
    GPUProgramParamHandle* param = nullptr;
    std::vector<float> values;
    int32_t size = 0, step = 0;
};

struct SetGPUProgramParamMat4ArrayPayload {
    GPUProgramParamHandle* param = nullptr;
    std::vector<Matrix4> values;
};

struct InitThreadGPUProgramParamPayload {
    GPUProgramHandle* program = nullptr;
    GPUProgramParamHandle* param = nullptr;
    std::string name;
};

// ---------------------------------------------------------------------------
// Helpers – shorthand so encoding sites are one-liners
// ---------------------------------------------------------------------------
#define CMD_BUF  m_commandBuffers[m_writeIdx]

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

RenderDeviceProxy::RenderDeviceProxy(PlatformSharedPtr platform, bool returnResImmediately)
    : RenderDeviceProxyBase(std::move(platform), returnResImmediately) {
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

// ---------------------------------------------------------------------------
// GPU program
// ---------------------------------------------------------------------------

void RenderDeviceProxy::useGPUProgram(GPUProgram* program) {
    auto* tp = static_cast<GPUProgramHandle*>(program);
    if (!m_threaded) {
        m_realDevice->useGPUProgram(tp->m_realProgram);
        return;
    }
    auto* pl = CMD_BUF.push<UseGPUProgramPayload>(Cmd_UseGPUProgram);
    pl->program = tp;
}

GPUProgram* RenderDeviceProxy::createGPUProgram(const std::string& programFileName,
                                                const std::string& vertexShader,
                                                const std::string& fragmentShader) {
    auto* program = new GPUProgramHandle(this);
    if (!m_threaded) {
        program->m_realProgram = m_realDevice->createGPUProgram(programFileName, vertexShader, fragmentShader);
    } else {
        auto* pl = CMD_BUF.pushNT<CreateGPUProgramPayload>(Cmd_CreateGPUProgram);
        pl->program = program;
        pl->programFileName = programFileName;
        pl->vertexShader = vertexShader;
        pl->fragmentShader = fragmentShader;
    }
    program->setProgramFileName(programFileName);
    return program;
}

void RenderDeviceProxy::deletGPUProgram(GPUProgram* program) {
    auto* tp = static_cast<GPUProgramHandle*>(program);
    if (!m_threaded) {
        m_realDevice->deletGPUProgram(tp->m_realProgram);
        delete tp;
        return;
    }
    auto* pl = CMD_BUF.push<DeleteGPUProgramPayload>(Cmd_DeleteGPUProgram);
    pl->program = tp;
}

// ---------------------------------------------------------------------------
// Texture2D
// ---------------------------------------------------------------------------

Texture2D* RenderDeviceProxy::createTexture2D(ImageType imageType) {
    auto* texture = new Texture2DHandle();
    if (!m_threaded) {
        texture->m_realTexture = m_realDevice->createTexture2D(imageType);
        return texture;
    }
    auto* pl = CMD_BUF.push<CreateTexture2DPayload>(Cmd_CreateTexture2D);
    pl->texture = texture;
    pl->imageType = imageType;
    return texture;
}

void RenderDeviceProxy::deleteTexture2D(Texture2D* texture) {
    auto* tt = static_cast<Texture2DHandle*>(texture);
    if (!m_threaded) {
        m_realDevice->deleteTexture2D(tt->m_realTexture);
        delete tt;
        return;
    }
    auto* pl = CMD_BUF.push<DeleteTexture2DPayload>(Cmd_DeleteTexture2D);
    pl->texture = tt;
}

void RenderDeviceProxy::useTexture2D(Texture2D* texture, uint32_t index) {
    auto* tt = static_cast<Texture2DHandle*>(texture);
    if (!m_threaded) {
        m_realDevice->useTexture2D(tt->m_realTexture, index);
        return;
    }
    auto* pl = CMD_BUF.push<UseTexture2DPayload>(Cmd_UseTexture2D);
    pl->texture = tt;
    pl->index = index;
}

bool RenderDeviceProxy::isTextureFormatSupported(PixelDataFormat textureFormat) {
    if (!m_threaded) return m_realDevice->isTextureFormatSupported(textureFormat);
    return true; // sync query not supported in async mode; common formats assumed supported
}

void RenderDeviceProxy::updateTexture2D(Texture2D* texture, const TextureData& data) {
    auto* tt = static_cast<Texture2DHandle*>(texture);
    if (!m_threaded) {
        m_realDevice->updateTexture2D(tt->m_realTexture, data);
        // 单线程模式：glTexImage2D 同步完成，立即通知调用方 buffer 可复用。
        if (data.releaseCallback) data.releaseCallback();
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateTexture2DPayload>(Cmd_UpdateTexture2D);
    pl->texture = tt;
    pl->data = data;
    if (!data.pixelOwner
        && data.pixels
        && data.bytes > 0
        && data.imageType != ImageType::OES) {
        // 原始指针路径：从池中取复用缓冲，memcpy 一次，编码时即完成 pixels 重定向。
        // executeFrame 读完后立即归还池（glTexImage2D 为同步调用）。
        auto buf = m_pixelDataRecyclePool->acquire();
        buf->assign(static_cast<const uint8_t*>(data.pixels),
                    static_cast<const uint8_t*>(data.pixels) + data.bytes);
        pl->data.pixels = buf->data(); // 编码时重定向，executeFrame 无需再处理
        pl->pixelStorage = std::move(buf);
    }
    // 若 data.pixelOwner 非空，共享所有权一并拷贝（原子引用计数+1，零像素拷贝）
    // 若 data.releaseCallback 非空，一并入队，渲染线程 GL 完成后回调。
    // 零拷贝路径：data.pixelOwner 持有内存，pl->data.pixels 已指向正确位置，无需额外操作。
}

void RenderDeviceProxy::updateSubTexture2D(Texture2D* texture, const TextureData& data,
                                           int32_t x, int32_t y,
                                           int32_t width, int32_t height,
                                           const unsigned char* sourceData) {
    auto* tt = static_cast<Texture2DHandle*>(texture);
    if (!m_threaded) {
        m_realDevice->updateSubTexture2D(tt->m_realTexture, data, x, y, width, height, sourceData);
        if (data.releaseCallback) data.releaseCallback();
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateSubTexture2DPayload>(Cmd_UpdateSubTexture2D);
    pl->texture = tt;
    pl->data = data;
    pl->x = x;
    pl->y = y;
    pl->width = width;
    pl->height = height;
    const size_t byteSize = static_cast<size_t>(PixelFormat::textureSizeInBytes(data.format, GL_UNSIGNED_BYTE, width, height));
    if (sourceData && byteSize > 0) {
        // sub-texture 总是需要拷贝（无 shared_ptr 所有权），使用池复用缓冲。
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

void RenderDeviceProxy::dumpFrameBuffer(int32_t x, int32_t y,
                                        int32_t displayWidth, int32_t displayHeight,
                                        int32_t rectX, int32_t rectY,
                                        int32_t rectWidth, int32_t rectHeight, int32_t comp) {
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
    if (!m_threaded) return m_realDevice->checkSSBOSupport();

    bool result = false;
    auto* pl = CMD_BUF.push<CheckSSBOSupportPayload>(Cmd_CheckSSBOSupport);
    pl->result = &result;
    submitCurrentBufferAndAdvance();
    return result;
}

// ---------------------------------------------------------------------------
// VBO
// ---------------------------------------------------------------------------

VBO* RenderDeviceProxy::createVBO() {
    auto* vbo = new VBOHandle();
    if (!m_threaded) {
        vbo->m_realVbo = m_realDevice->createVBO();
        return vbo;
    }
    auto* pl = CMD_BUF.push<CreateVBOPayload>(Cmd_CreateVBO);
    pl->vbo = vbo;
    return vbo;
}

void RenderDeviceProxy::updateVBO(GPUProgram* program, VBO* vbo, VBODataSharedPtr vboData) {
    auto* tp = static_cast<GPUProgramHandle*>(program);
    auto* tv = static_cast<VBOHandle*>(vbo);
    if (!m_threaded) {
        m_realDevice->updateVBO(tp->m_realProgram, tv->m_realVbo, vboData);
        m_currentFrameRecyclables.push_back(std::move(vboData));
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateVBOPayload>(Cmd_UpdateVBO);
    pl->program = tp;
    pl->vbo = tv;
    pl->vboData = std::move(vboData);
}

void RenderDeviceProxy::deleteVBO(VBO* vbo) {
    auto* tv = static_cast<VBOHandle*>(vbo);
    if (!m_threaded) {
        m_realDevice->deleteVBO(tv->m_realVbo);
        delete tv;
        return;
    }
    auto* pl = CMD_BUF.push<DeleteVBOPayload>(Cmd_DeleteVBO);
    pl->vbo = tv;
}

void RenderDeviceProxy::drawVBO(VBO* vbo, int32_t instanceCount) {
    auto* tv = static_cast<VBOHandle*>(vbo);
    if (!m_threaded) {
        m_realDevice->drawVBO(tv->m_realVbo, instanceCount);
        return;
    }
    auto* pl = CMD_BUF.push<DrawVBOPayload>(Cmd_DrawVBO);
    pl->vbo = tv;
    pl->instanceCount = instanceCount;
}

// ---------------------------------------------------------------------------
// Blend
// ---------------------------------------------------------------------------

void RenderDeviceProxy::enableBlend() {
    if (!m_threaded) {
        m_realDevice->enableBlend();
        return;
    }
    CMD_BUF.push(Cmd_EnableBlend);
}

void RenderDeviceProxy::disableBlend() {
    if (!m_threaded) {
        m_realDevice->disableBlend();
        return;
    }
    CMD_BUF.push(Cmd_DisableBlend);
}

// ---------------------------------------------------------------------------
// GPU program uniform params
// ---------------------------------------------------------------------------

void RenderDeviceProxy::setGPUProgramParamAsInt(GPUProgramParam* param, int32_t value) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsInt(tp->m_realParam, value);
        return;
    }
    auto* pl = CMD_BUF.push<SetGPUProgramParamIntPayload>(Cmd_SetGPUProgramAsInt);
    pl->param = tp;
    pl->value = value;
}

void RenderDeviceProxy::setGPUProgramParamAsFloat(GPUProgramParam* param, float value) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsFloat(tp->m_realParam, value);
        return;
    }
    auto* pl = CMD_BUF.push<SetGPUProgramParamFloatPayload>(Cmd_SetGPUProgramAsFloat);
    pl->param = tp;
    pl->value = value;
}

void RenderDeviceProxy::setGPUProgramParamAsVec2(GPUProgramParam* param, float x, float y) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsVec2(tp->m_realParam, x, y);
        return;
    }
    auto* pl = CMD_BUF.push<SetGPUProgramParamVec2Payload>(Cmd_SetGPUProgramAsVec2);
    pl->param = tp;
    pl->x = x;
    pl->y = y;
}

void RenderDeviceProxy::setGPUProgramParamAsVec3(GPUProgramParam* param, float x, float y, float z) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsVec3(tp->m_realParam, x, y, z);
        return;
    }
    auto* pl = CMD_BUF.push<SetGPUProgramParamVec3Payload>(Cmd_SetGPUProgramAsVec3);
    pl->param = tp;
    pl->x = x;
    pl->y = y;
    pl->z = z;
}

void RenderDeviceProxy::setGPUProgramParamAsVec4(GPUProgramParam* param, float x, float y, float z, float w) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsVec4(tp->m_realParam, x, y, z, w);
        return;
    }
    auto* pl = CMD_BUF.push<SetGPUProgramParamVec4Payload>(Cmd_SetGPUProgramAsVec4);
    pl->param = tp;
    pl->x = x;
    pl->y = y;
    pl->z = z;
    pl->w = w;
}

void RenderDeviceProxy::setGPUProgramParamAsMat4(GPUProgramParam* param, const Matrix4& mat) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsMat4(tp->m_realParam, mat);
        return;
    }
    auto* pl = CMD_BUF.push<SetGPUProgramParamMat4Payload>(Cmd_SetGPUProgramAsMat4);
    pl->param = tp;
    pl->mat = mat;
}

void RenderDeviceProxy::setGPUProgramParamAsIntArray(GPUProgramParam* param,
                                                     const int32_t* values, int32_t size, int32_t step) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsIntArray(tp->m_realParam, values, size, step);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamIntArrayPayload>(Cmd_SetGPUProgramAsIntArray);
    pl->param = tp;
    pl->size = size;
    pl->step = step;
    const int32_t n = (size > 0 && step > 0) ? size * step : 0;
    if (values && n > 0) pl->values.assign(values, values + n);
}

void RenderDeviceProxy::setGPUProgramParamAsFloatArray(GPUProgramParam* param,
                                                       const float* values, int32_t size, int32_t step) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsFloatArray(tp->m_realParam, values, size, step);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamFloatArrayPayload>(Cmd_SetGPUProgramAsFloatArray);
    pl->param = tp;
    pl->size = size;
    pl->step = step;
    const int32_t n = (size > 0 && step > 0) ? size * step : 0;
    if (values && n > 0) pl->values.assign(values, values + n);
}

void RenderDeviceProxy::setGPUProgramParamAsMat4Array(GPUProgramParam* param,
                                                      const std::vector<Matrix4>& values) {
    auto* tp = static_cast<GPUProgramParamHandle*>(param);
    if (!m_threaded) {
        m_realDevice->setGPUProgramParamAsMat4Array(tp->m_realParam, values);
        return;
    }
    auto* pl = CMD_BUF.pushNT<SetGPUProgramParamMat4ArrayPayload>(Cmd_SetGPUProgramAsMat4Array);
    pl->param = tp;
    pl->values = values;
}

// --- String-name overloads (delegate to param overloads above) ---
void RenderDeviceProxy::setGPUProgramParamAsInt(GPUProgram* p, const std::string& u, int32_t v) { setGPUProgramParamAsInt(p->getUniformParam(u), v); }

void RenderDeviceProxy::setGPUProgramParamAsFloat(GPUProgram* p, const std::string& u, float v) { setGPUProgramParamAsFloat(p->getUniformParam(u), v); }

void RenderDeviceProxy::setGPUProgramParamAsVec2(GPUProgram* p, const std::string& u, float x, float y) { setGPUProgramParamAsVec2(p->getUniformParam(u), x, y); }

void RenderDeviceProxy::setGPUProgramParamAsVec3(GPUProgram* p, const std::string& u, float x, float y, float z) { setGPUProgramParamAsVec3(p->getUniformParam(u), x, y, z); }

void RenderDeviceProxy::setGPUProgramParamAsVec4(GPUProgram* p, const std::string& u, float x, float y, float z, float w) { setGPUProgramParamAsVec4(p->getUniformParam(u), x, y, z, w); }

void RenderDeviceProxy::setGPUProgramParamAsMat4(GPUProgram* p, const std::string& u, const Matrix4& m) { setGPUProgramParamAsMat4(p->getUniformParam(u), m); }

void RenderDeviceProxy::setGPUProgramParamAsIntArray(GPUProgram* p, const std::string& u, const int32_t* v, int32_t sz, int32_t st) {
    setGPUProgramParamAsIntArray(p->getUniformParam(u), v, sz, st);
}

void RenderDeviceProxy::setGPUProgramParamAsFloatArray(GPUProgram* p, const std::string& u, const float* v, int32_t sz, int32_t st) {
    setGPUProgramParamAsFloatArray(p->getUniformParam(u), v, sz, st);
}

// ---------------------------------------------------------------------------
// GPUProgramParamHandle initialisation
// ---------------------------------------------------------------------------

void RenderDeviceProxy::initThreadGPUProgramParam(GPUProgramHandle* program,
                                                  GPUProgramParamHandle* param,
                                                  const std::string& name) {
    if (!m_threaded) {
        param->m_realParam = m_realDevice->getGPUProgramParam(program->m_realProgram, name);
        return;
    }
    auto* pl = CMD_BUF.pushNT<InitThreadGPUProgramParamPayload>(Cmd_InitThreadGPUProgramParam);
    pl->program = program;
    pl->param = param;
    pl->name = name;
}

// ---------------------------------------------------------------------------
// UBO
// ---------------------------------------------------------------------------

UBO* RenderDeviceProxy::createUBO() {
    auto* u = new UBOHandle();
    if (!m_threaded) {
        u->m_realUbo = m_realDevice->createUBO();
        return u;
    }
    auto* pl = CMD_BUF.push<CreateUBOPayload>(Cmd_CreateUBO);
    pl->ubo = u;
    return u;
}

void RenderDeviceProxy::updateUBO(UBO* ubo, std::shared_ptr<UBOData> uboData) {
    auto* tu = static_cast<UBOHandle*>(ubo);
    if (!m_threaded) {
        m_realDevice->updateUBO(tu->m_realUbo, uboData);
        m_currentFrameUBORecyclables.push_back(std::move(uboData));
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateUBOPayload>(Cmd_UpdateUBO);
    pl->ubo = tu;
    pl->uboData = std::move(uboData);
}

void RenderDeviceProxy::bindUBO(GPUProgram* program, UBO* ubo,
                                const std::string& blockName, uint32_t bindingPoint) {
    auto* tp = static_cast<GPUProgramHandle*>(program);
    auto* tu = static_cast<UBOHandle*>(ubo);
    if (!m_threaded) {
        m_realDevice->bindUBO(tp->m_realProgram, tu->getReal(), blockName, bindingPoint);
        return;
    }
    auto* pl = CMD_BUF.pushNT<BindUBOPayload>(Cmd_BindUBO);
    pl->program = tp;
    pl->ubo = tu;
    pl->blockName = blockName;
    pl->bindingPoint = bindingPoint;
}

// ---------------------------------------------------------------------------
// SSBO
// ---------------------------------------------------------------------------

SSBO* RenderDeviceProxy::createSSBO() {
    auto* s = new SSBOHandle();
    if (!m_threaded) {
        s->m_realSSBO = m_realDevice->createSSBO();
        return s;
    }
    auto* pl = CMD_BUF.push<CreateSSBOPayload>(Cmd_CreateSSBO);
    pl->ssbo = s;
    return s;
}

void RenderDeviceProxy::updateSSBO(SSBO* ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) {
    auto* ts = static_cast<SSBOHandle*>(ssbo);
    if (!m_threaded) {
        m_realDevice->updateSSBO(ts->getReal(), ssboData, bindingPoint);
        m_currentFrameSSBORecyclables.push_back(std::move(ssboData));
        return;
    }
    auto* pl = CMD_BUF.pushNT<UpdateSSBOPayload>(Cmd_UpdateSSBO);
    pl->ssbo = ts;
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

RenderTarget* RenderDeviceProxy::createRenderTarget(int32_t w, int32_t h,
                                                     Texture2D** outColorTexture) {
    auto* rt = new RenderTargetHandle();
    rt->m_width  = w;
    rt->m_height = h;

    Texture2DHandle* colorTexHandle = nullptr;
    if (outColorTexture) {
        colorTexHandle = new Texture2DHandle();
        *outColorTexture = colorTexHandle;
    }

    if (!m_threaded) {
        Texture2D* rawTex = nullptr;
        rt->m_realRenderTarget = m_realDevice->createRenderTarget(w, h, colorTexHandle ? &rawTex : nullptr);
        if (colorTexHandle) colorTexHandle->m_realTexture = rawTex;
        return rt;
    }
    auto* pl = CMD_BUF.push<CreateRenderTargetPayload>(Cmd_CreateRenderTarget);
    pl->rt       = rt;
    pl->colorTex = colorTexHandle;
    pl->w        = w;
    pl->h        = h;
    return rt;
}

void RenderDeviceProxy::deleteRenderTarget(RenderTarget* rt) {
    auto* tr = static_cast<RenderTargetHandle*>(rt);
    if (!m_threaded) {
        m_realDevice->deleteRenderTarget(tr->m_realRenderTarget);
        delete tr;
        return;
    }
    auto* pl = CMD_BUF.push<DeleteRenderTargetPayload>(Cmd_DeleteRenderTarget);
    pl->rt = tr;
}

void RenderDeviceProxy::bindRenderTarget(RenderTarget* rt) {
    auto* tr = static_cast<RenderTargetHandle*>(rt);
    if (!m_threaded) {
        m_realDevice->bindRenderTarget(tr->m_realRenderTarget);
        return;
    }
    auto* pl = CMD_BUF.push<BindRenderTargetPayload>(Cmd_BindRenderTarget);
    pl->rt = tr;
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

void RenderDeviceProxy::setDepthTest(bool enable) {
    if (!m_threaded) { m_realDevice->setDepthTest(enable); return; }
    auto* pl = CMD_BUF.push<SetDepthTestPayload>(Cmd_SetDepthTest);
    pl->enable = enable;
}

void RenderDeviceProxy::setDepthWrite(bool enable) {
    if (!m_threaded) { m_realDevice->setDepthWrite(enable); return; }
    auto* pl = CMD_BUF.push<SetDepthWritePayload>(Cmd_SetDepthWrite);
    pl->enable = enable;
}

void RenderDeviceProxy::setCullFace(CullFaceMode mode) {
    if (!m_threaded) { m_realDevice->setCullFace(mode); return; }
    auto* pl = CMD_BUF.push<SetCullFacePayload>(Cmd_SetCullFace);
    pl->mode = mode;
}

void RenderDeviceProxy::clearDepth() {
    if (!m_threaded) { m_realDevice->clearDepth(); return; }
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
        m_pendingFrames.push({
            m_realDevice->insertFence(),
            std::move(m_currentFrameRecyclables),
            std::move(m_currentFrameUBORecyclables),
            std::move(m_currentFrameSSBORecyclables)
        });
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
        if (isQuit()) break;

        executeFrame(m_commandBuffers[readIdx]);

        // 像素缓冲立即归还：glTexImage2D/glTexSubImage2D 均为同步调用，
        // 驱动已完成数据拷贝，无需等待 GPU Fence，比 VBO 回收更快。
        if (m_pixelDataRecyclePool && !m_framePixelRecyclables.empty())
            m_pixelDataRecyclePool->release(std::move(m_framePixelRecyclables));
        m_framePixelRecyclables.clear();

        // Insert fence and book-keep recyclables collected during executeFrame.
        m_pendingFrames.push({
            m_realDevice->insertFence(),
            std::move(m_frameRecyclables),
            std::move(m_frameUBORecyclables),
            std::move(m_frameSSBORecyclables)
        });
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

        if (h->type == Cmd_EndFrame) break;

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
            case Cmd_UseGPUProgram: {
                auto* pl = static_cast<UseGPUProgramPayload*>(p);
                if (pl->program) m_realDevice->useGPUProgram(pl->program->m_realProgram);
                break;
            }
            case Cmd_CreateGPUProgram: {
                auto* pl = static_cast<CreateGPUProgramPayload*>(p);
                if (pl->program)
                    pl->program->m_realProgram = m_realDevice->createGPUProgram(
                        pl->programFileName, pl->vertexShader, pl->fragmentShader);
                break;
            }
            case Cmd_DeleteGPUProgram: {
                auto* pl = static_cast<DeleteGPUProgramPayload*>(p);
                if (pl->program) {
                    m_realDevice->deletGPUProgram(pl->program->m_realProgram);
                    delete pl->program;
                }
                break;
            }
            case Cmd_CreateTexture2D: {
                auto* pl = static_cast<CreateTexture2DPayload*>(p);
                if (pl->texture) pl->texture->m_realTexture = m_realDevice->createTexture2D(pl->imageType);
                break;
            }
            case Cmd_DeleteTexture2D: {
                auto* pl = static_cast<DeleteTexture2DPayload*>(p);
                if (pl->texture) {
                    m_realDevice->deleteTexture2D(pl->texture->m_realTexture);
                    delete pl->texture;
                }
                break;
            }
            case Cmd_UseTexture2D: {
                auto* pl = static_cast<UseTexture2DPayload*>(p);
                if (pl->texture) m_realDevice->useTexture2D(pl->texture->m_realTexture, pl->index);
                break;
            }
            case Cmd_UpdateTexture2D: {
                auto* pl = static_cast<UpdateTexture2DPayload*>(p);
                if (pl->texture) {
                    // pl->data.pixels 已在编码阶段正确设置（零拷贝或池缓冲均如此），无需再次重定向。
                    m_realDevice->updateTexture2D(pl->texture->m_realTexture, pl->data);
                    // glTexImage2D 同步完成，GL 驱动已拷贝像素，立即通知调用方 buffer 可复用。
                    // 回调先于 pixelStorage 归池，确保应用侧先收到信号再复用。
                    if (pl->data.releaseCallback) pl->data.releaseCallback();
                    // 拷贝路径：内部池缓冲归还（零拷贝路径此字段为空，shared_ptr 析构由 clear() 处理）。
                    if (pl->pixelStorage)
                        m_framePixelRecyclables.push_back(std::move(pl->pixelStorage));
                }
                break;
            }
            case Cmd_UpdateSubTexture2D: {
                auto* pl = static_cast<UpdateSubTexture2DPayload*>(p);
                if (pl->texture && pl->pixelStorage) {
                    m_realDevice->updateSubTexture2D(pl->texture->m_realTexture, pl->data,
                                                     pl->x, pl->y, pl->width, pl->height,
                                                     pl->pixelStorage->data());
                    // 同上：glTexSubImage2D 同步完成，先回调通知，再归还池缓冲。
                    if (pl->data.releaseCallback) pl->data.releaseCallback();
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
                m_realDevice->setViewPort(
                    static_cast<int32_t>(pl->v.x), static_cast<int32_t>(pl->v.y),
                    static_cast<int32_t>(pl->v.z), static_cast<int32_t>(pl->v.w));
                break;
            }
            case Cmd_DumpFrameBuffer: {
                auto* pl = static_cast<DumpFrameBufferPayload*>(p);
                m_realDevice->dumpFrameBuffer(pl->x, pl->y, pl->displayWidth, pl->displayHeight,
                                              pl->rectX, pl->rectY, pl->rectWidth, pl->rectHeight, pl->comp);
                break;
            }
            case Cmd_CreateVBO: {
                auto* pl = static_cast<CreateVBOPayload*>(p);
                if (pl->vbo) pl->vbo->m_realVbo = m_realDevice->createVBO();
                break;
            }
            case Cmd_UpdateVBO: {
                auto* pl = static_cast<UpdateVBOPayload*>(p);
                if (pl->program && pl->vbo && pl->vboData) {
                    m_realDevice->updateVBO(pl->program->m_realProgram, pl->vbo->m_realVbo, pl->vboData);
                    m_frameRecyclables.push_back(pl->vboData);
                }
                break;
            }
            case Cmd_DeleteVBO: {
                auto* pl = static_cast<DeleteVBOPayload*>(p);
                if (pl->vbo) {
                    m_realDevice->deleteVBO(pl->vbo->m_realVbo);
                    delete pl->vbo;
                }
                break;
            }
            case Cmd_DrawVBO: {
                auto* pl = static_cast<DrawVBOPayload*>(p);
                if (pl->vbo) m_realDevice->drawVBO(pl->vbo->m_realVbo, pl->instanceCount);
                break;
            }
            case Cmd_EnableBlend: m_realDevice->enableBlend();
                break;
            case Cmd_DisableBlend: m_realDevice->disableBlend();
                break;
            case Cmd_SetGPUProgramAsInt: {
                auto* pl = static_cast<SetGPUProgramParamIntPayload*>(p);
                if (pl->param) m_realDevice->setGPUProgramParamAsInt(pl->param->m_realParam, pl->value);
                break;
            }
            case Cmd_SetGPUProgramAsFloat: {
                auto* pl = static_cast<SetGPUProgramParamFloatPayload*>(p);
                if (pl->param) m_realDevice->setGPUProgramParamAsFloat(pl->param->m_realParam, pl->value);
                break;
            }
            case Cmd_SetGPUProgramAsVec2: {
                auto* pl = static_cast<SetGPUProgramParamVec2Payload*>(p);
                if (pl->param) m_realDevice->setGPUProgramParamAsVec2(pl->param->m_realParam, pl->x, pl->y);
                break;
            }
            case Cmd_SetGPUProgramAsVec3: {
                auto* pl = static_cast<SetGPUProgramParamVec3Payload*>(p);
                if (pl->param) m_realDevice->setGPUProgramParamAsVec3(pl->param->m_realParam, pl->x, pl->y, pl->z);
                break;
            }
            case Cmd_SetGPUProgramAsVec4: {
                auto* pl = static_cast<SetGPUProgramParamVec4Payload*>(p);
                if (pl->param) m_realDevice->setGPUProgramParamAsVec4(pl->param->m_realParam, pl->x, pl->y, pl->z, pl->w);
                break;
            }
            case Cmd_SetGPUProgramAsMat4: {
                auto* pl = static_cast<SetGPUProgramParamMat4Payload*>(p);
                if (pl->param) m_realDevice->setGPUProgramParamAsMat4(pl->param->m_realParam, pl->mat);
                break;
            }
            case Cmd_SetGPUProgramAsIntArray: {
                auto* pl = static_cast<SetGPUProgramParamIntArrayPayload*>(p);
                if (pl->param && pl->size > 0 && !pl->values.empty())
                    m_realDevice->setGPUProgramParamAsIntArray(pl->param->m_realParam, pl->values.data(), pl->size, pl->step);
                break;
            }
            case Cmd_SetGPUProgramAsFloatArray: {
                auto* pl = static_cast<SetGPUProgramParamFloatArrayPayload*>(p);
                if (pl->param && pl->size > 0 && !pl->values.empty())
                    m_realDevice->setGPUProgramParamAsFloatArray(pl->param->m_realParam, pl->values.data(), pl->size, pl->step);
                break;
            }
            case Cmd_SetGPUProgramAsMat4Array: {
                auto* pl = static_cast<SetGPUProgramParamMat4ArrayPayload*>(p);
                if (pl->param && !pl->values.empty())
                    m_realDevice->setGPUProgramParamAsMat4Array(pl->param->m_realParam, pl->values);
                break;
            }
            case Cmd_InitThreadGPUProgramParam: {
                auto* pl = static_cast<InitThreadGPUProgramParamPayload*>(p);
                if (pl->program && pl->param)
                    pl->param->m_realParam = m_realDevice->getGPUProgramParam(pl->program->m_realProgram, pl->name);
                break;
            }
            case Cmd_CreateUBO: {
                auto* pl = static_cast<CreateUBOPayload*>(p);
                if (pl->ubo) pl->ubo->m_realUbo = m_realDevice->createUBO();
                break;
            }
            case Cmd_UpdateUBO: {
                auto* pl = static_cast<UpdateUBOPayload*>(p);
                if (pl->ubo && pl->uboData) {
                    m_realDevice->updateUBO(pl->ubo->m_realUbo, pl->uboData);
                    m_frameUBORecyclables.push_back(pl->uboData);
                }
                break;
            }
            case Cmd_BindUBO: {
                auto* pl = static_cast<BindUBOPayload*>(p);
                if (pl->program && pl->ubo)
                    m_realDevice->bindUBO(pl->program->m_realProgram, pl->ubo->getReal(), pl->blockName, pl->bindingPoint);
                break;
            }
            case Cmd_CreateSSBO: {
                auto* pl = static_cast<CreateSSBOPayload*>(p);
                if (pl->ssbo) pl->ssbo->m_realSSBO = m_realDevice->createSSBO();
                break;
            }
            case Cmd_UpdateSSBO: {
                auto* pl = static_cast<UpdateSSBOPayload*>(p);
                if (pl->ssbo && pl->ssboData) {
                    m_realDevice->updateSSBO(pl->ssbo->getReal(), pl->ssboData, pl->bindingPoint);
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
                if (pl->rt) {
                    Texture2D* rawTex = nullptr;
                    pl->rt->m_realRenderTarget = m_realDevice->createRenderTarget(
                        pl->w, pl->h, pl->colorTex ? &rawTex : nullptr);
                    if (pl->colorTex && rawTex)
                        pl->colorTex->m_realTexture = rawTex;
                }
                break;
            }
            case Cmd_DeleteRenderTarget: {
                auto* pl = static_cast<DeleteRenderTargetPayload*>(p);
                if (pl->rt) {
                    m_realDevice->deleteRenderTarget(pl->rt->m_realRenderTarget);
                    delete pl->rt;
                }
                break;
            }
            case Cmd_BindRenderTarget: {
                auto* pl = static_cast<BindRenderTargetPayload*>(p);
                if (pl->rt) m_realDevice->bindRenderTarget(pl->rt->m_realRenderTarget);
                break;
            }
            case Cmd_UnbindRenderTarget:
                m_realDevice->unbindRenderTarget();
                break;
            case Cmd_SetDepthTest: {
                auto* pl = static_cast<SetDepthTestPayload*>(p);
                m_realDevice->setDepthTest(pl->enable);
                break;
            }
            case Cmd_SetDepthWrite: {
                auto* pl = static_cast<SetDepthWritePayload*>(p);
                m_realDevice->setDepthWrite(pl->enable);
                break;
            }
            case Cmd_SetCullFace: {
                auto* pl = static_cast<SetCullFacePayload*>(p);
                m_realDevice->setCullFace(pl->mode);
                break;
            }
            case Cmd_ClearDepth:
                m_realDevice->clearDepth();
                break;
            default: break;
        }

        ptr += sizeof(CommandBuffer::CmdHeader) + CommandBuffer::align8(h->payloadSize);
    }
}

#undef CMD_BUF
} // namespace morrow
