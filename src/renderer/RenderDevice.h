//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_ESDEVICE_H_
#define MORROW_RENDERER_ESDEVICE_H_

#include <string>
#include <memory>
#include <vector>

#include "GpuTypes.h"         // 资源句柄 + DTO（UBO/VBO/Texture2D/TextureData 等）
#include "GlobalDefine.h"
#include "Vector4.h"

namespace morrow {
using namespace Math;

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual void makeCurrent(void* window) = 0;

    virtual void present(void* window) = 0;

    virtual void debugDriver() = 0;

    virtual void clear() = 0;

    // OpenGL渲染相关接口（与EGL解耦）
    virtual void setClearColor(float r, float g, float b, float alpha) = 0;

    virtual void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) = 0;

    virtual void dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight,
                                 int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight,
                                 int32_t comp) = 0;

    virtual bool checkSSBOSupport() = 0;

    //---------------------------------------------------VBO---------------------------------------------------
    virtual VBO* createVBO() = 0;

    virtual void updateVBO(GPUProgram* program, VBO* vbo, VBODataSharedPtr vboData) = 0;

    virtual void deleteVBO(VBO* vbo) = 0;

    virtual void drawVBO(VBO* vbo, int32_t instanceCount) = 0;

    //---------------------------------------------------Texture2D---------------------------------------------------
    virtual Texture2D* createTexture2D(ImageType imageType) = 0;

    virtual void deleteTexture2D(Texture2D* texture) = 0;

    virtual void useTexture2D(Texture2D* texture, uint32_t index) = 0;

    virtual bool isTextureFormatSupported(PixelDataFormat textureFormat) = 0;

    virtual void updateTexture2D(Texture2D* texture, const TextureData& data) = 0;

    virtual void updateSubTexture2D(Texture2D* texture, const TextureData& data,
                                    int32_t x, int32_t y, int32_t width, int32_t height,
                                    const unsigned char* sourceData) = 0;

    //---------------------------------------------------Blend---------------------------------------------------
    virtual void enableBlend() = 0;

    virtual void disableBlend() = 0;

    //---------------------------------------------------GPUProgram---------------------------------------------------
    virtual void useGPUProgram(GPUProgram* program) = 0;

    virtual GPUProgram* createGPUProgram(const std::string& programFileName,
                                         const std::string& vertexShader,
                                         const std::string& fragmentShader) = 0;

    virtual void deletGPUProgram(GPUProgram* program) = 0;

    virtual GPUProgramParam* getGPUProgramParam(GPUProgram* program, const std::string& name) = 0;

    virtual void setGPUProgramParamAsInt(GPUProgramParam* param, int32_t value) = 0;

    virtual void setGPUProgramParamAsFloat(GPUProgramParam* param, float value) = 0;

    virtual void setGPUProgramParamAsVec2(GPUProgramParam* param, float x, float y) = 0;

    virtual void setGPUProgramParamAsVec3(GPUProgramParam* param, float x, float y, float z) = 0;

    virtual void setGPUProgramParamAsVec4(GPUProgramParam* param, float x, float y, float z, float w) = 0;

    virtual void setGPUProgramParamAsMat4(GPUProgramParam* param, const Matrix4& mat) = 0;

    virtual void setGPUProgramParamAsIntArray(GPUProgramParam* param, const int32_t* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsFloatArray(GPUProgramParam* param, const float* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsMat4Array(GPUProgramParam* param, const std::vector<Matrix4>& values) = 0;

    //------------------------------------------------------shader专用（按名称设置）-------------------------------------------------------
    virtual void setGPUProgramParamAsInt(GPUProgram* program, const std::string& uniformName, int32_t value) = 0;

    virtual void setGPUProgramParamAsFloat(GPUProgram* program, const std::string& uniformName, float value) = 0;

    virtual void setGPUProgramParamAsVec2(GPUProgram* program, const std::string& uniformName, float x, float y) = 0;

    virtual void setGPUProgramParamAsVec3(GPUProgram* program, const std::string& uniformName, float x, float y, float z) = 0;

    virtual void setGPUProgramParamAsVec4(GPUProgram* program, const std::string& uniformName, float x, float y, float z, float w) = 0;

    virtual void setGPUProgramParamAsMat4(GPUProgram* program, const std::string& uniformName, const Matrix4& mat) = 0;

    virtual void setGPUProgramParamAsIntArray(GPUProgram* program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) = 0;

    virtual void setGPUProgramParamAsFloatArray(GPUProgram* program, const std::string& uniformName, const float* values, int32_t size, int32_t step) = 0;

    //---------------------------------------------------UBO---------------------------------------------------
    virtual UBO* createUBO() = 0;

    virtual void updateUBO(UBO* ubo, std::shared_ptr<UBOData> uboData) = 0;

    virtual void bindUBO(GPUProgram* program, UBO* ubo, const std::string& blockName, uint32_t bindingPoint) = 0;

    //---------------------------------------------------SSBO---------------------------------------------------
    virtual SSBO* createSSBO() = 0;

    virtual void updateSSBO(SSBO* ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) = 0;

    //---------------------------------------------------GPU Fence（用于 VBOData 回收）---------------------------------------------------
    /// 在 GL 命令流中插入 fence，返回不透明句柄（GL 下为 GLsync）。仅在当前 GL 上下文线程调用。
    virtual void* insertFence() { return nullptr; }

    /// 等待 fence 完成，timeoutNs 为 0 表示非阻塞。返回 true 表示已完成可安全回收。
    virtual bool waitFence(void* fence, uint64_t timeoutNs) { return false; }

    virtual void deleteFence(void* fence) {
    }

    // FBO操作
    virtual RenderTarget* createRenderTarget(int32_t w, int32_t h,
                                             Texture2D** outColorTexture) = 0;
    virtual void deleteRenderTarget(RenderTarget* rt) = 0;
    virtual void bindRenderTarget(RenderTarget* rt) = 0;
    virtual void unbindRenderTarget() = 0;   // glBindFramebuffer(0)

    // 深度/状态（3D必须）
    virtual void setDepthTest(bool enable) = 0;
    virtual void setDepthWrite(bool enable) = 0;
    virtual void setCullFace(CullFaceMode mode) = 0;
    virtual void clearDepth() = 0;
};
} // namespace morrow

#endif //MORROW_RENDERER_ESDEVICE_H_
