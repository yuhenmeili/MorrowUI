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
#include "ResourceRegistry.h"

namespace morrow {

// ---------------------------------------------------------------------------
// GLRenderDevice – OpenGL/GLES 渲染后端的唯一公共类。
// 持有 ResourceRegistry，负责 Handle ↔ GL 对象的映射。
// ---------------------------------------------------------------------------

using GLRenderDevicePtr    = std::shared_ptr<GLRenderDevice>;

class GLRenderDevice : public RenderDevice {
public:
    explicit GLRenderDevice(PlatformSharedPtr platform);

    ~GLRenderDevice() override;

    // ── Handle 预分配（线程安全，前端调用）──
    HwTexture2D    allocateTexture2D()    { return m_registry.allocateTexture2D(); }
    HwVBO          allocateVBO()          { return m_registry.allocateVBO(); }
    HwUBO          allocateUBO()          { return m_registry.allocateUBO(); }
    HwSSBO         allocateSSBO()         { return m_registry.allocateSSBO(); }
    HwGPUProgram   allocateGPUProgram()   { return m_registry.allocateGPUProgram(); }
    HwRenderTarget allocateRenderTarget() { return m_registry.allocateRenderTarget(); }

    void makeCurrent(void* window) override;

    void present(void* window) override;

    void debugDriver() override;

    void clear() override;

    void setClearColor(float r, float g, float b, float alpha) override;

    void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) override;

    void dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight, int32_t comp) override;

    bool checkSSBOSupport() override;

    //------------------------------------------------------VBO------------------------------------------------------
    HwVBO createVBO() override;              // interface (allocate+commit)
    void  commitVBO(HwVBO handle);           // proxy helper (handle pre-allocated)
    void updateVBO(HwGPUProgram program, HwVBO vbo, VBODataSharedPtr vboData) override;
    void deleteVBO(HwVBO vbo) override;
    void drawVBO(HwVBO vbo, int32_t instanceCount) override;

    //------------------------------------------------------Texture2D------------------------------------------------------
    HwTexture2D createTexture2D(ImageType imageType) override;       // interface
    void        commitTexture2D(HwTexture2D handle, ImageType imageType); // proxy helper
    void deleteTexture2D(HwTexture2D texture) override;
    void useTexture2D(HwTexture2D texture, uint32_t index) override;
    bool isTextureFormatSupported(PixelDataFormat textureFormat) override;
    void updateTexture2D(HwTexture2D texture, const TextureData& data) override;
    void updateSubTexture2D(HwTexture2D texture, const TextureData& data, int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData) override;

    //------------------------------------------------------Blend------------------------------------------------------
    void enableBlend() override;
    void disableBlend() override;

    //------------------------------------------------------GPUProgram------------------------------------------------------
    void useGPUProgram(HwGPUProgram program) override;
    HwGPUProgram createGPUProgram(const std::string&, const std::string&, const std::string&) override; // interface
    void         commitGPUProgram(HwGPUProgram handle, const std::string& programFileName, const std::string& vertexShader, const std::string& fragmentShader); // proxy helper
    void deletGPUProgram(HwGPUProgram program) override;

    // All uniform setting via name (GPUProgramParam removed)
    void setGPUProgramParamAsInt(HwGPUProgram program, const std::string& uniformName, int32_t value) override;
    void setGPUProgramParamAsFloat(HwGPUProgram program, const std::string& uniformName, float value) override;
    void setGPUProgramParamAsVec2(HwGPUProgram program, const std::string& uniformName, float x, float y) override;
    void setGPUProgramParamAsVec3(HwGPUProgram program, const std::string& uniformName, float x, float y, float z) override;
    void setGPUProgramParamAsVec4(HwGPUProgram program, const std::string& uniformName, float x, float y, float z, float w) override;
    void setGPUProgramParamAsMat4(HwGPUProgram program, const std::string& uniformName, const Matrix4& mat) override;
    void setGPUProgramParamAsIntArray(HwGPUProgram program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) override;
    void setGPUProgramParamAsFloatArray(HwGPUProgram program, const std::string& uniformName, const float* values, int32_t size, int32_t step) override;
    void setGPUProgramParamAsMat4Array(HwGPUProgram program, const std::string& uniformName, const std::vector<Matrix4>& values) override;

    //---------------------------------------------------UBO---------------------------------------------------
    HwUBO createUBO() override;              // interface
    void  commitUBO(HwUBO handle);           // proxy helper
    void updateUBO(HwUBO ubo, std::shared_ptr<UBOData> uboData) override;
    void bindUBO(HwGPUProgram program, HwUBO ubo, const std::string& blockName, uint32_t bindingPoint) override;

    //---------------------------------------------------SSBO---------------------------------------------------
    HwSSBO createSSBO() override;            // interface
    void   commitSSBO(HwSSBO handle);        // proxy helper
    void updateSSBO(HwSSBO ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) override;

    void* insertFence() override;
    bool waitFence(void* fence, uint64_t timeoutNs) override;
    void deleteFence(void* fence) override;

    // FBO操作
    HwRenderTarget createRenderTarget(int32_t w, int32_t h, HwTexture2D* outColorTexture = nullptr) override; // interface
    void           commitRenderTarget(HwRenderTarget rtHandle, int32_t w, int32_t h, HwTexture2D* outColorTexture = nullptr); // proxy helper
    void deleteRenderTarget(HwRenderTarget rt) override;
    void bindRenderTarget(HwRenderTarget rt) override;
    void unbindRenderTarget() override;

    // 深度/状态
    void setDepthTest(bool enable) override;
    void setDepthWrite(bool enable) override;
    void setCullFace(CullFaceMode mode) override;
    void clearDepth() override;

    // ── 内部辅助 ──
    ResourceRegistry& getRegistry() { return m_registry; }

private:
    PlatformSharedPtr m_platform;
    HwRenderTarget m_boundRenderTarget{0};
    ResourceRegistry m_registry;

    // ── 内部 GL helper（不对外暴露）──
    bool makeFolder();
    bool upLoadTexture(GlTexture2D* textureImp, const TextureData& data);
    bool upLoadOESTexture(GlTexture2D* textureImp, const TextureData& data);
    bool checkCompileErrors(const std::string& programFileName, GLuint shader, std::string type);
};
} // MORROWGUI

#endif //MORROW_RENDERER_ESDEVICEIMP_H_
