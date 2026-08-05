//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_THREADBUFFERESDEVICE_H_
#define MORROW_RENDERER_THREADBUFFERESDEVICE_H_

#include <memory>
#include <queue>
#include <vector>

#include "CommandBuffer.h"
#include "PlatformSemaphore.h"
#include "RecyclePool.h"
#include "RenderDeviceProxyBase.h"

namespace morrow {
using VBODataRecyclePool = RecyclePool<VBOData>;
using UBODataRecyclePool = RecyclePool<UBOData>;
using SSBODataRecyclePool = RecyclePool<SSBOData>;
// 像素上传缓冲池：glTexImage2D 是同步调用，executeFrame 结束后即可归还，
// 不必等 GPU Fence，回收速度远快于 VBO/UBO。
using PixelDataRecyclePool = RecyclePool<std::vector<uint8_t>>;

class RenderDeviceProxy : public RenderDeviceProxyBase {
public:
    RenderDeviceProxy(PlatformSharedPtr platform, bool returnResImmediately);

    ~RenderDeviceProxy() override;

    void makeCurrent(void* window) override;

    void present(void* window) override;

    void debugDriver() override;

    void clear() override;

    void setClearColor(float r, float g, float b, float alpha) override;

    void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) override;

    void dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight, int32_t comp) override;

    bool checkSSBOSupport() override;

    void bindPipelineState(const GraphicsPipelineState& state) override;

    //----------------------------------------------------------VBO------------------------------------------------
    HwVBO createVBO() override;

    void updateVBO(HwGPUProgram program, HwVBO vbo, VBODataSharedPtr vboData) override;

    void deleteVBO(HwVBO vbo) override;

    void drawVBO(HwVBO vbo, int32_t instanceCount) override;

    //----------------------------------------------------------Texture2D------------------------------------------------
    HwTexture2D createTexture2D(ImageType imageType) override;

    void deleteTexture2D(HwTexture2D texture) override;

    void useTexture2D(HwTexture2D texture, uint32_t index) override;

    bool isTextureFormatSupported(PixelDataFormat textureFormat) override;

    void updateTexture2D(HwTexture2D texture, const TextureData& data) override;

    void updateSubTexture2D(HwTexture2D texture, const TextureData& data, int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData) override;

    //----------------------------------------------------------GPUProgram------------------------------------------------
    void useGPUProgram(HwGPUProgram program) override;

    HwGPUProgram createGPUProgram(const std::string& programFileName, const std::string& vertexShader, const std::string& fragmentShader) override;

    void deletGPUProgram(HwGPUProgram program) override;

    SSBOReflectedLayout reflectSSBOBlock(HwGPUProgram program, const std::string& blockName) override;

    void setGPUProgramParamAsInt(HwGPUProgram program, const std::string& uniformName, int32_t value) override;

    void setGPUProgramParamAsFloat(HwGPUProgram program, const std::string& uniformName, float value) override;

    void setGPUProgramParamAsVec2(HwGPUProgram program, const std::string& uniformName, float x, float y) override;

    void setGPUProgramParamAsVec3(HwGPUProgram program, const std::string& uniformName, float x, float y, float z) override;

    void setGPUProgramParamAsVec4(HwGPUProgram program, const std::string& uniformName, float x, float y, float z, float w) override;

    void setGPUProgramParamAsMat4(HwGPUProgram program, const std::string& uniformName, const Matrix4& mat) override;

    void setGPUProgramParamAsIntArray(HwGPUProgram program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) override;

    void setGPUProgramParamAsFloatArray(HwGPUProgram program, const std::string& uniformName, const float* values, int32_t size, int32_t step) override;

    void setGPUProgramParamAsMat4Array(HwGPUProgram program, const std::string& uniformName, const std::vector<Matrix4>& values) override;

    //-----------------------------------------------------UBO------------------------------------------------

    HwUBO createUBO() override;

    void updateUBO(HwUBO ubo, std::shared_ptr<UBOData> uboData) override;

    void bindUBO(HwGPUProgram program, HwUBO ubo, const std::string& blockName, uint32_t bindingPoint) override;

    //---------------------------------------------------SSBO---------------------------------------------------
    HwSSBO createSSBO() override;

    void updateSSBO(HwSSBO ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) override;

    void* insertFence() override;

    bool waitFence(void* fence, uint64_t timeoutNs) override;

    void deleteFence(void* fence) override;

    // FBO操作
    HwRenderTarget createRenderTarget(int32_t w, int32_t h, HwTexture2D colorTexture);

    void deleteRenderTarget(HwRenderTarget rt) override;

    void bindRenderTarget(HwRenderTarget rt) override;

    void unbindRenderTarget() override;

    // 深度/状态
    void clearDepth() override;

    //----------------------------------------------------Frame control---------------------------------------------------
    /**
     * Submit the current frame's command buffer and advance to the next write
     * slot.  In multithreaded mode this is fully asynchronous: the main thread
     * is only blocked when the ring buffer is full (render thread is more than
     * kRingSize-1 frames behind).
     */
    void endFrame();

    /// 主线程从池中取 VBOData / UBOData / SSBOData
    VBODataRecyclePool* getVBODataRecyclePool() {
        return m_vboRecyclePool.get();
    }

    UBODataRecyclePool* getUBODataRecyclePool() {
        return m_uboRecyclePool.get();
    }

    SSBODataRecyclePool* getSSBODataRecyclePool() {
        return m_ssboRecyclePool.get();
    }

    /// 主线程从池中取像素上传缓冲（原始指针路径，无 shared_ptr 所有权时使用）
    PixelDataRecyclePool* getPixelDataRecyclePool() {
        return m_pixelDataRecyclePool.get();
    }

    /// 在持有 GL 上下文的线程调用：等待已完成的 fence 并将对应 VBOData 回收到池。多线程时在渲染线程 runCommand 内调用，单线程时在 endFrame 后由 Engine 调用。
    void tryRecycle();

    void runCommand() override;

    void signalThreadToExit() override;

private:
    // ---------------------------------------------------------------
    // Triple ring-buffer command queue
    // kRingSize = 3 lets the main thread be up to 2 frames ahead of
    // the render thread with zero per-frame blocking in the common case.
    // ---------------------------------------------------------------
    static constexpr uint32_t kRingSize = 3;

    void submitCurrentBufferAndAdvance();

    void executeFrame(CommandBuffer& buf);

    struct PendingFrame {
        void* fence = nullptr;
        std::vector<VBODataSharedPtr> vboRecyclables;
        std::vector<std::shared_ptr<UBOData>> uboRecyclables;
        std::vector<std::shared_ptr<SSBOData>> ssboRecyclables;
    };

    std::unique_ptr<VBODataRecyclePool> m_vboRecyclePool;
    std::unique_ptr<UBODataRecyclePool> m_uboRecyclePool;
    std::unique_ptr<SSBODataRecyclePool> m_ssboRecyclePool;
    // 像素缓冲池：executeFrame 结束后立即归还（无需等 fence）
    std::unique_ptr<PixelDataRecyclePool> m_pixelDataRecyclePool;
    std::queue<PendingFrame> m_pendingFrames;

    // Render-thread-local recyclable lists (populated during executeFrame)
    std::vector<VBODataSharedPtr> m_frameRecyclables;
    std::vector<std::shared_ptr<UBOData>> m_frameUBORecyclables;
    std::vector<std::shared_ptr<SSBOData>> m_frameSSBORecyclables;
    // 像素缓冲：glTexImage2D 同步消耗，executeFrame 后立即回池，不进 PendingFrame
    std::vector<std::shared_ptr<std::vector<uint8_t>>> m_framePixelRecyclables;

    // Single-threaded-mode recyclable lists (populated in updateVBO/UBO/SSBO)
    std::vector<VBODataSharedPtr> m_currentFrameRecyclables;
    std::vector<std::shared_ptr<UBOData>> m_currentFrameUBORecyclables;
    std::vector<std::shared_ptr<SSBOData>> m_currentFrameSSBORecyclables;

    // Ring buffer slots – one CommandBuffer per slot (pre-allocated, fixed size)
    CommandBuffer m_commandBuffers[kRingSize];
    uint32_t m_writeIdx = 0;  // main-thread write cursor (no sharing)

    // m_frameReadySem: main thread signals once per endFrame(); render thread
    //                  waits for it before draining a buffer.
    // m_freeSlotsSem:  initialised to (kRingSize-1); render thread signals
    //                  after consuming each buffer; main thread waits here –
    //                  blocks only when the ring is full.
    Semaphore m_frameReadySem;
    Semaphore m_freeSlotsSem;
};

using ThreadBufferESDeviceSharedPtr = std::shared_ptr<RenderDeviceProxy>;
}  // namespace morrow

#endif  // MORROW_RENDERER_THREADBUFFERESDEVICE_H_
