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

#include "Platform.h"
#include "RenderDevice.h"
#include "ResourceRegistry.h"

namespace morrow {
// ---------------------------------------------------------------------------
// GLRenderDevice – OpenGL/GLES 渲染后端的唯一公共类。
// 持有 ResourceRegistry，负责 Handle ↔ GL 对象的映射。
// ---------------------------------------------------------------------------

using GLRenderDevicePtr = std::shared_ptr<GLRenderDevice>;

class GLRenderDevice {
public:
    explicit GLRenderDevice(PlatformSharedPtr platform);

    ~GLRenderDevice();

    void makeCurrent(void* window);

    void present(void* window);

    void debugDriver();

    void clear();

    void setClearColor(float r, float g, float b, float alpha);

    void setViewPort(int32_t x, int32_t y, int32_t width, int32_t height);

    void dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight, int32_t comp);

    bool checkSSBOSupport();

    void bindPipelineState(const GraphicsPipelineState& state);

    //------------------------------------------------------VBO------------------------------------------------------
    HwVBO createVBOSync();

    void createVBORender(HwVBO handle);

    void updateVBO(HwGPUProgram program, HwVBO vbo, VBODataSharedPtr vboData);

    void deleteVBO(HwVBO vbo);

    void drawVBO(HwVBO vbo, int32_t instanceCount);

    //------------------------------------------------------Texture2D------------------------------------------------------
    HwTexture2D createTexture2DSync();

    void createTexture2DRender(HwTexture2D handle, ImageType imageType);

    void deleteTexture2D(HwTexture2D texture);

    void useTexture2D(HwTexture2D texture, uint32_t index);

    bool isTextureFormatSupported(PixelDataFormat textureFormat);

    void updateTexture2D(HwTexture2D texture, const TextureData& data);

    void updateSubTexture2D(HwTexture2D texture, const TextureData& data, int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData);

    //------------------------------------------------------GPUProgram------------------------------------------------------
    void useGPUProgram(HwGPUProgram program);

    HwGPUProgram createGPUProgramSync();

    void createGPUProgramRender(HwGPUProgram handle, const std::string& programFileName, const std::string& vertexShader, const std::string& fragmentShader);

    void deletGPUProgram(HwGPUProgram program);

    SSBOReflectedLayout reflectSSBOBlock(HwGPUProgram program, const std::string& blockName);

    // All uniform setting via name (GPUProgramParam removed)
    void setGPUProgramParamAsInt(HwGPUProgram program, const std::string& uniformName, int32_t value);

    void setGPUProgramParamAsFloat(HwGPUProgram program, const std::string& uniformName, float value);

    void setGPUProgramParamAsVec2(HwGPUProgram program, const std::string& uniformName, float x, float y);

    void setGPUProgramParamAsVec3(HwGPUProgram program, const std::string& uniformName, float x, float y, float z);

    void setGPUProgramParamAsVec4(HwGPUProgram program, const std::string& uniformName, float x, float y, float z, float w);

    void setGPUProgramParamAsMat4(HwGPUProgram program, const std::string& uniformName, const Matrix4& mat);

    void setGPUProgramParamAsIntArray(HwGPUProgram program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step);

    void setGPUProgramParamAsFloatArray(HwGPUProgram program, const std::string& uniformName, const float* values, int32_t size, int32_t step);

    void setGPUProgramParamAsMat4Array(HwGPUProgram program, const std::string& uniformName, const std::vector<Matrix4>& values);

    //---------------------------------------------------UBO---------------------------------------------------
    HwUBO createUBOSync();

    void createUBORender(HwUBO handle);

    void updateUBO(HwUBO ubo, std::shared_ptr<UBOData> uboData);

    void bindUBO(HwGPUProgram program, HwUBO ubo, const std::string& blockName, uint32_t bindingPoint);

    //---------------------------------------------------SSBO---------------------------------------------------
    HwSSBO createSSBOSync();

    void createSSBORender(HwSSBO handle);

    void updateSSBO(HwSSBO ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint);

    void* insertFence();

    bool waitFence(void* fence, uint64_t timeoutNs);

    void deleteFence(void* fence);

    // FBO操作
    HwRenderTarget createRenderTargetSync();

    void createRenderTargetRender(HwRenderTarget rtHandle, int32_t w, int32_t h, HwTexture2D colorTexture);

    void deleteRenderTarget(HwRenderTarget rt);

    void bindRenderTarget(HwRenderTarget rt);

    void unbindRenderTarget();

    // 深度/状态
    void clearDepth();

    // ── 内部辅助 ──
    ResourceRegistry& getRegistry() {
        return m_registry;
    }

private:
    // GPU 状态缓存：避免重复调用相同的 GL 状态设置命令。
    // 每个状态设置函数在调用 GL 前先检查缓存，值未变化则跳过。
    struct GLStateCache {
        HwGPUProgram currentProgram{0};

        static constexpr uint32_t kMaxTextureUnits = 8;
        HwTexture2D currentTextures[kMaxTextureUnits] = {};

        bool blendEnabled = false;
        bool blendStateValid = false;
        BlendFactor srcRgbBlendFactor = BlendFactor::ONE;
        BlendFactor dstRgbBlendFactor = BlendFactor::ZERO;
        BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
        BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
        bool blendFuncValid = false;

        int32_t viewportX = 0, viewportY = 0, viewportW = 0, viewportH = 0;
        bool viewportValid = false;

        bool depthTestEnabled = false;
        bool depthTestValid = false;

        bool depthWriteEnabled = true;
        bool depthWriteValid = false;

        CullFaceMode cullFaceMode = CullFaceMode::NONE;
        bool cullFaceValid = false;

        /// 上下文切换或 RenderTarget 解绑后调用，强制下次全量设置
        void invalidate() {
            currentProgram = HwGPUProgram{0};
            for (auto& t : currentTextures)
                t = HwTexture2D{0};
            blendStateValid = false;
            blendFuncValid = false;
            viewportValid = false;
            depthTestValid = false;
            depthWriteValid = false;
            cullFaceValid = false;
        }
    };

    GLStateCache m_stateCache;

    PlatformSharedPtr m_platform;
    HwRenderTarget m_boundRenderTarget{0};
    GLint m_viewportBeforeRenderTarget[4] = {0, 0, 0, 0};
    ResourceRegistry m_registry;

    // ── 内部 GL helper（不对外暴露）──
    bool makeFolder();

    bool upLoadTexture(GlTexture2D* textureImp, const TextureData& data);

    bool upLoadOESTexture(GlTexture2D* textureImp, const TextureData& data);

    void enableBlend();

    void disableBlend();

    void setBlendFunc(BlendFactor srcRgbFactor, BlendFactor dstRgbFactor, BlendFactor srcAlphaFactor, BlendFactor dstAlphaFactor);

    void setDepthTest(bool enable);

    void setDepthWrite(bool enable);

    void setCullFace(CullFaceMode mode);

    bool checkCompileErrors(const std::string& programFileName, GLuint shader, std::string type);
};
} // namespace morrow

#endif  // MORROW_RENDERER_ESDEVICEIMP_H_