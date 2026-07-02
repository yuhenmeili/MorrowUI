//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_ESDEVICEIMP_H_
#define MORROW_RENDERER_ESDEVICEIMP_H_

#ifdef OPENGL_GLFW
#include "platform/wgl/OpenglHeader.h"
#else
#include "platform/egl/EGLHeader.h"
#include "platform/egl/GLESHeader.h"
#endif

#include "RenderDevice.h"
#include "Platform.h"

namespace morrow {

// ---------------------------------------------------------------------------
// GLRenderDevice – OpenGL/GLES 渲染后端的唯一公共类。
// ---------------------------------------------------------------------------

using GLRenderDevicePtr    = std::shared_ptr<GLRenderDevice>;

class GLRenderDevice : public RenderDevice {
public:
    explicit GLRenderDevice(PlatformSharedPtr platform);

    ~GLRenderDevice() override;

    void makeCurrent(void* window) override;

    void present(void* window) override;

    void debugDriver() override;

    void clear() override;

    void setClearColor(float r, float g, float b, float alpha) override;

    void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) override;

    void dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight, int32_t comp) override;

    bool checkSSBOSupport() override;

    //------------------------------------------------------VBO------------------------------------------------------

    VBO* createVBO() override;

    void updateVBO(GPUProgram* program, VBO* vbo, VBODataSharedPtr vboData) override;

    void deleteVBO(VBO* vbo) override;

    void drawVBO(VBO* vbo, int32_t instanceCount) override;

    //------------------------------------------------------Texture2D------------------------------------------------------
    Texture2D* createTexture2D(ImageType imageType) override;

    void deleteTexture2D(Texture2D* texture) override;

    void useTexture2D(Texture2D* texture, uint32_t index) override;

    bool isTextureFormatSupported(PixelDataFormat textureFormat) override;

    void updateTexture2D(Texture2D* texture, const TextureData& data) override;

    void updateSubTexture2D(Texture2D* texture, const TextureData& data, int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData) override;

    bool upLoadTexture(Texture2D* texture, const TextureData& data);

    bool upLoadOESTexture(Texture2D* texture, const TextureData& data);

    //------------------------------------------------------Blend------------------------------------------------------

    void enableBlend() override;

    void disableBlend() override;

    //------------------------------------------------------GPUProgram------------------------------------------------------
    //gpu program
    void useGPUProgram(GPUProgram* program) override;

    bool makeFolder();

    GPUProgram* createGPUProgram(const std::string& programFileName, const std::string& vertexShader, const std::string& fragmentShader) override;

    bool checkCompileErrors(const std::string& programFileName, GLuint shader, std::string type);

    void deletGPUProgram(GPUProgram* program) override;

    GPUProgramParam* getGPUProgramParam(GPUProgram* program, const std::string& name) override;

    void setGPUProgramParamAsInt(GPUProgramParam* param, int32_t value) override;

    void setGPUProgramParamAsFloat(GPUProgramParam* param, float value) override;

    void setGPUProgramParamAsVec2(GPUProgramParam* param, float x, float y) override;

    void setGPUProgramParamAsVec3(GPUProgramParam* param, float x, float y, float z) override;

    void setGPUProgramParamAsVec4(GPUProgramParam* param, float x, float y, float z, float w) override;

    void setGPUProgramParamAsMat4(GPUProgramParam* param, const Matrix4& mat) override;

    void setGPUProgramParamAsIntArray(GPUProgramParam* param, const int32_t* values, int32_t size, int32_t step) override;

    void setGPUProgramParamAsFloatArray(GPUProgramParam* param, const float* values, int32_t size, int32_t step) override;

    void setGPUProgramParamAsMat4Array(GPUProgramParam* param, const std::vector<Matrix4>& values) override;

    //-----------------------------------------------------------------------------------------------------

    void setGPUProgramParamAsInt(GPUProgram* program, const std::string& uniformName, int32_t value) override;

    void setGPUProgramParamAsFloat(GPUProgram* program, const std::string& uniformName, float value) override;

    void setGPUProgramParamAsVec2(GPUProgram* program, const std::string& uniformName, float x, float y) override;

    void setGPUProgramParamAsVec3(GPUProgram* program, const std::string& uniformName, float x, float y, float z) override;

    void setGPUProgramParamAsVec4(GPUProgram* program, const std::string& uniformName, float x, float y, float z, float w) override;

    void setGPUProgramParamAsMat4(GPUProgram* program, const std::string& uniformName, const Matrix4& mat) override;

    void setGPUProgramParamAsIntArray(GPUProgram* program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) override;

    void setGPUProgramParamAsFloatArray(GPUProgram* program, const std::string& uniformName, const float* values, int32_t size, int32_t step) override;

    //---------------------------------------------------UBO---------------------------------------------------
    UBO* createUBO() override;

    void updateUBO(UBO* ubo, std::shared_ptr<UBOData> uboData) override;

    void bindUBO(GPUProgram* program, UBO* ubo, const std::string& blockName, uint32_t bindingPoint) override;

    //---------------------------------------------------SSBO---------------------------------------------------
    SSBO* createSSBO() override;

    void updateSSBO(SSBO* ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) override;

    void* insertFence() override;

    bool waitFence(void* fence, uint64_t timeoutNs) override;

    void deleteFence(void* fence) override;

    // FBO操作
    RenderTarget* createRenderTarget(int32_t w, int32_t h,
                                     Texture2D** outColorTexture = nullptr) override;
    void deleteRenderTarget(RenderTarget* rt) override;
    void bindRenderTarget(RenderTarget* rt) override;
    void unbindRenderTarget() override;

    // 深度/状态
    void setDepthTest(bool enable) override;
    void setDepthWrite(bool enable) override;
    void setCullFace(CullFaceMode mode) override;
    void clearDepth() override;

private:
    PlatformSharedPtr m_platform;
    RenderTarget* m_boundRenderTarget = nullptr;
};
} // MORROWGUI

#endif //MORROW_RENDERER_ESDEVICEIMP_H_
