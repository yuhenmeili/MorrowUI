//
// Created by lance on 2023/12/8.
//

#include <algorithm>
#include <fstream>
#include "GLRenderDevice.h"
#include "PixelFormat.h"
#include "TextureLoader.h"
#include "unistd.h"
#include "sys/stat.h"
#include "ToolUtils.h"
#include "utils/OpenglUtils.h"
#ifdef OPENGL_EGL
#include "egl/QNXEGLPlatform.h"
#endif

#define MR_POSITION "a_position"
#define MR_NORMAL "a_normal"
#define MR_TEXCOORD "a_texCoord"
#define MR_TANGENT "a_tangent"
#define MR_COLOR "a_color"
#define MR_BATCH "a_batch"

namespace morrow {
class GlProgramParam : public GPUProgramParam {
public:
    explicit GlProgramParam(GLint loc) : location(loc) {
    }

    GPUProgramParam* getReal() override { return this; }

    GLint location;
};

class GlProgram : public GPUProgram {
public:
    explicit GlProgram(GLuint id) : ProgramID(id) {
    }

    ~GlProgram() override {
        for (auto& p : _nameToParams) delete p.second;
    }

    GPUProgram* getReal() override { return this; }

    GPUProgramParam* getUniformParam(const std::string& name) override {
        auto it = _nameToParams.find(name);
        if (it == _nameToParams.end()) {
            auto* param = new GlProgramParam(glGetUniformLocation(ProgramID, name.c_str()));
            _nameToParams.emplace(name, param);
            return param;
        }
        return it->second;
    }

    GPUProgramParam* getAttributeParam(const std::string& name) override {
        auto it = _nameToParams.find(name);
        if (it == _nameToParams.end()) {
            auto* param = new GlProgramParam(glGetAttribLocation(ProgramID, name.c_str()));
            _nameToParams.emplace(name, param);
            return param;
        }
        return it->second;
    }

    GLuint ProgramID;

private:
    std::unordered_map<std::string, GlProgramParam*> _nameToParams;
};

class GlTexture2D : public Texture2D {
public:
    explicit GlTexture2D(GLuint id) : textureID(id) {
    }

    Texture2D* getReal() override { return this; }

    GLuint textureID;
    GLenum textureTarget = GL_TEXTURE_2D;
    bool   m_ownedByFBO  = false;  // true → GL texture owned by FBO, don't delete here
#ifdef OPENGL_EGL
    PFNEGLCREATEIMAGEKHRPROC p_eglCreateImageKHR = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC p_glEGLImageTargetTexture2DOES = nullptr;
    EGLImageKHR m_pixel = nullptr;
    std::unordered_map<void*, EGLImageKHR> m_eglImageMap;
#endif
    uint32_t m_debugTimes = 0;
};

class GlVBO : public VBO {
    friend class GLRenderDevice;

protected:
    ~GlVBO() override = default;

public:
    VBO* getReal() override { return this; }

    GLuint vertexArrayID = 0;
    GLuint vertexbuffer = 0;
    GLuint verticesCount = 0;
    GLuint elementbuffer = 0;
    GLuint indicesCount = 0;
    GLenum drawMode = GL_TRIANGLES;
};

class GlUBO : public UBO {
public:
    UBO* getReal() override { return this; }

    GLuint bufferID = 0;
    size_t bufferSize = 0;
    uint32_t bindingPoint = 0;
};

class GlSSBO : public SSBO {
public:
    SSBO* getReal() override { return this; }

    GLuint bufferID = 0;
    size_t bufferSize = 0;
    uint32_t bindingPoint = 0;
};

GLRenderDevice::GLRenderDevice(PlatformSharedPtr platform) : m_platform(platform) {
}

GLRenderDevice::~GLRenderDevice() {
}

void GLRenderDevice::makeCurrent(void* window) {
#ifdef OPENGL_GLFW
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(window));
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG_E("Failed to initialize GLAD");
        return;
    }
#else
    auto* eglPlatform = dynamic_cast<QNXEGLPlatform*>(m_platform.get());
    if (!eglPlatform) {
        return;
    }

    auto context = eglPlatform->getContext();
    if (!context) {
        return;
    }

    auto surface = static_cast<EGLSurface>(window);
    if (surface != nullptr) {
        if (eglGetCurrentSurface(EGL_DRAW) != surface || eglGetCurrentContext() != context->eglContext) {
            eglMakeCurrent(context->eglDisplay, surface, surface, context->eglContext);
        }
    } else {
        eglMakeCurrent(context->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
#endif
}

void GLRenderDevice::present(void* window) {
#ifdef OPENGL_GLFW
    glfwSwapBuffers(static_cast<GLFWwindow*>(window));
#else
    auto* eglPlatform = dynamic_cast<QNXEGLPlatform*>(m_platform.get());
    if (!eglPlatform || !window) {
        return;
    }

    auto context = eglPlatform->getContext();
    if (!context) {
        return;
    }

    eglSwapBuffers(context->eglDisplay, static_cast<EGLSurface>(window));
#endif
}

void GLRenderDevice::debugDriver() {
#ifdef OPENGL_EGL
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        LOG_W("GLRenderDevice::debugDriver skipped: no current EGL context");
        return;
    }
#endif
    const GLubyte* version = glGetString(GL_VERSION);
    if (!version) {
        LOG_W("OpenGL Version unavailable: glGetString(GL_VERSION) returned null");
        return;
    }
    LOG_I("OpenGL Version: {}", reinterpret_cast<const char*>(version));
}

void GLRenderDevice::clear() {
    GLboolean depthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    if (depthMask != GL_TRUE) {
        glDepthMask(GL_TRUE);
    }
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (depthMask != GL_TRUE) {
        glDepthMask(depthMask);
    }
}

void GLRenderDevice::setViewPort(int32_t x, int32_t y, int32_t width, int32_t height) {
    glViewport(x, y, width, height);
}

void GLRenderDevice::setClearColor(float r, float g, float b, float alpha) {
    glClearColor(r, g, b, alpha);
}

void GLRenderDevice::dumpFrameBuffer(int32_t x, int32_t y, int32_t displayWidth, int32_t displayHeight, int32_t rectX, int32_t rectY, int32_t rectWidth, int32_t rectHeight, int32_t comp) {
    std::string fileName = "/data/log/framebuffer" + Math::generate_uuid() + ".png";
    std::vector<uint8_t> data(rectWidth * rectHeight * comp);
    rectY = displayHeight - rectHeight;
    glReadPixels(rectX, rectY, rectWidth, rectHeight, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    TextureFromFile::save(fileName.c_str(), rectWidth, rectHeight, comp, data.data(), 0, true);
}

bool GLRenderDevice::checkSSBOSupport() {
#ifdef OPENGL_EGL
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        LOG_W("GLRenderDevice::checkSSBOSupport skipped: no current EGL context");
        return false;
    }
#endif
    const char* versionStr = (const char*)glGetString(GL_VERSION);
    if (!versionStr) {
        return false;
    }

    // 检查桌面版OpenGL
    if (strstr(versionStr, "OpenGL ES") == nullptr) {
        // OpenGL桌面版
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        return (major > 4 || (major == 4 && minor >= 3));
    } else {
        // OpenGL ES版
        return (strstr(versionStr, "OpenGL ES 3.1") ||
            strstr(versionStr, "OpenGL ES 3.2") ||
            strstr(versionStr, "OpenGL ES 4."));
    }
}

VBO* GLRenderDevice::createVBO() {
    auto vbo = new GlVBO();
    glGenVertexArrays(1, &vbo->vertexArrayID);
    glBindVertexArray(vbo->vertexArrayID);
    glGenBuffers(1, &vbo->vertexbuffer);
    glGenBuffers(1, &vbo->elementbuffer);
    return vbo;
}

void GLRenderDevice::updateVBO(GPUProgram* program, VBO* vbo, VBODataSharedPtr vboData) {
    auto* glVbo = (GlVBO*)vbo;

    glBindVertexArray(glVbo->vertexArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, glVbo->vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vboData->vertexData.size(), vboData->vertexData.data(), GL_STATIC_DRAW);

    // 为每个属性设置顶点指针（现在是连续存储）
    for (const auto& attr : vboData->attributes) {
        switch (attr.type) {
            case VertexAttributeType::Position: {
                auto positionParam = dynamic_cast<GlProgramParam*>(program->getAttributeParam(MR_POSITION));
                if (positionParam && positionParam->location != -1) {
                    glEnableVertexAttribArray(positionParam->location);
                    glVertexAttribPointer(positionParam->location, 3, GL_FLOAT, GL_FALSE, 0, (void*)attr.offset);
                }
                break;
            }
            case VertexAttributeType::Color: {
                auto colorParam = dynamic_cast<GlProgramParam*>(program->getAttributeParam(MR_COLOR));
                if (colorParam && colorParam->location != -1) {
                    glEnableVertexAttribArray(colorParam->location);
                    glVertexAttribPointer(colorParam->location, 4, GL_FLOAT, GL_FALSE, 0, (void*)attr.offset);
                }
                break;
            }
            case VertexAttributeType::UV: {
                auto uvParam = dynamic_cast<GlProgramParam*>(program->getAttributeParam(MR_TEXCOORD));
                if (uvParam && uvParam->location != -1) {
                    glEnableVertexAttribArray(uvParam->location);
                    glVertexAttribPointer(uvParam->location, 2, GL_FLOAT, GL_FALSE, 0, (void*)attr.offset);
                }
                break;
            }
            case VertexAttributeType::Normal: {
                auto normalParam = dynamic_cast<GlProgramParam*>(program->getAttributeParam(MR_NORMAL));
                if (normalParam && normalParam->location != -1) {
                    glEnableVertexAttribArray(normalParam->location);
                    glVertexAttribPointer(normalParam->location, 3, GL_FLOAT, GL_FALSE, 0, (void*)attr.offset);
                }
                break;
            }
            case VertexAttributeType::Tangent: {
                auto tangentParam = dynamic_cast<GlProgramParam*>(program->getAttributeParam(MR_TANGENT));
                if (tangentParam && tangentParam->location != -1) {
                    glEnableVertexAttribArray(tangentParam->location);
                    glVertexAttribPointer(tangentParam->location, 4, GL_FLOAT, GL_FALSE, 0, (void*)attr.offset);
                }
                break;
            }
            case VertexAttributeType::BatchID: {
                auto batchParam = dynamic_cast<GlProgramParam*>(program->getAttributeParam(MR_BATCH));
                if (batchParam && batchParam->location != -1) {
                    glEnableVertexAttribArray(batchParam->location);
                    glVertexAttribPointer(batchParam->location, 1, GL_FLOAT, GL_FALSE, 0, (void*)attr.offset);
                }
                break;
            }
        }
    }

    // 处理索引数据
    if (vboData->indexCount > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glVbo->elementbuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, vboData->indexCount * sizeof(uint32_t),
                     vboData->indices.data(), GL_STATIC_DRAW);
    }

    glVbo->verticesCount = vboData->vertexCount;
    glVbo->indicesCount = vboData->indexCount;
    glVbo->drawMode = GLenum(vboData->drawMode);
}


void GLRenderDevice::deleteVBO(VBO* vbo) {
    auto* glVbo = dynamic_cast<GlVBO*>(vbo);
    glDeleteBuffers(1, &glVbo->vertexbuffer);
    glDeleteBuffers(1, &glVbo->elementbuffer);
    glDeleteVertexArrays(1, &glVbo->vertexArrayID);
    delete glVbo;
}

void GLRenderDevice::drawVBO(VBO* vbo, int32_t instanceCount) {
    auto* glVbo = dynamic_cast<GlVBO*>(vbo);
    glBindVertexArray(glVbo->vertexArrayID);
    glDisable(GL_CULL_FACE);
    if (glVbo->indicesCount == 0) {
        if (glVbo->drawMode == GL_POINTS) {
#ifdef OPENGL_GLFW
            glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        }
        // glDrawArrays(glVbo->drawMode, 0, glVbo->verticesCount);
        if (instanceCount > 1) {
            glDrawArraysInstanced(glVbo->drawMode, 0, glVbo->verticesCount, instanceCount);
        } else {
            glDrawArrays(glVbo->drawMode, 0, glVbo->verticesCount);
        }
    } else {
        // glDrawElements(glVbo->drawMode, glVbo->indicesCount, GL_UNSIGNED_SHORT, (void*) 0);
        if (instanceCount > 1) {
            glDrawElementsInstanced(glVbo->drawMode, glVbo->indicesCount, GL_UNSIGNED_INT, (void*)0, instanceCount);
        } else {
            glDrawElements(glVbo->drawMode, glVbo->indicesCount, GL_UNSIGNED_INT, (void*)0);
        }
    }
}

Texture2D* GLRenderDevice::createTexture2D(ImageType imageType) {
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    auto realTexture = new GlTexture2D(textureID);
#ifdef OPENGL_GLFW
    realTexture->textureTarget = GL_TEXTURE_2D;
#else
    realTexture->textureTarget = imageType == ImageType::OES ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
#endif

    return realTexture;
}

void GLRenderDevice::deleteTexture2D(Texture2D* texture) {
    auto realTex = dynamic_cast<GlTexture2D*>(texture);
    if (!realTex) {
        return;
    }
#ifdef OPENGL_GLFW
#else
    if (!realTex->m_eglImageMap.empty()) {
        auto p_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
        if (p_eglDestroyImageKHR != nullptr) {
            auto qnxPlatform = std::dynamic_pointer_cast<QNXEGLPlatform>(m_platform);
            if (qnxPlatform && qnxPlatform->getContext()) {
                for (auto& iter : realTex->m_eglImageMap) {
                    (*p_eglDestroyImageKHR)(qnxPlatform->getContext()->eglDisplay, iter.second);
                }
            }
        }
    }
#endif

    if (!realTex->m_ownedByFBO) {
        glDeleteTextures(1, &realTex->textureID);
    }
    delete realTex;
}

void GLRenderDevice::useTexture2D(Texture2D* texture, uint32_t index) {
    auto textureImp = dynamic_cast<GlTexture2D*>(texture);
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(textureImp->textureTarget, dynamic_cast<GlTexture2D*>(texture)->textureID);
}

bool GLRenderDevice::isTextureFormatSupported(PixelDataFormat textureFormat) {
    auto format = (int32_t)OpenglUtils::getFormat(textureFormat);
    int32_t num_formats;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &num_formats);
    if (num_formats > 0) {
        auto* formats = (int32_t*)alloca(num_formats * sizeof(int32_t));
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);
        for (int32_t index = 0; index < num_formats; index++) {
            if (format == formats[index]) {
                return true;
            }
        }
    }
    return false;
}

void GLRenderDevice::updateTexture2D(Texture2D* texture, const TextureData& data) {
    data.imageType == ImageType::OES ? upLoadOESTexture(texture, data) : upLoadTexture(texture, data);
}

void GLRenderDevice::updateSubTexture2D(Texture2D* texture, const TextureData& data, int32_t x, int32_t y, int32_t width, int32_t height, const unsigned char* sourceData) {
    auto textureImp = dynamic_cast<GlTexture2D*>(texture);
    GLenum textureTarget = textureImp->textureTarget;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(textureTarget, dynamic_cast<GlTexture2D*>(texture)->textureID);

    int32_t unpackAlignment = data.imageType == ImageType::TEXT
                                  ? 1
                                  : PixelFormat::alignmentInBytes(
                                      data.format,
                                      GL_UNSIGNED_BYTE,
                                      data.width
                                  );
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    GLenum format = OpenglUtils::getFormat(data.format);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, sourceData);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_E("glTexSubImage2D failed for texture update, imageType={}, format={}, formatEnum=0x{:x}, region=({}, {}, {}, {}), glError=0x{:x}",
              static_cast<int32_t>(data.imageType),
              static_cast<int32_t>(data.format),
              static_cast<uint32_t>(format),
              x, y, width, height,
              static_cast<uint32_t>(error));
    }
}

bool GLRenderDevice::upLoadTexture(Texture2D* texture, const TextureData& data) {
    auto textureImp = dynamic_cast<GlTexture2D*>(texture);
    GLenum textureTarget = textureImp->textureTarget;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(textureTarget, dynamic_cast<GlTexture2D*>(texture)->textureID);

    GLint minFilterType = OpenglUtils::getMinFilterType(data.minFilterType);
    GLint magFilterType = OpenglUtils::getMagFilterType(data.magFilterType);

    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, minFilterType);
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, magFilterType);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLenum internalFormat = OpenglUtils::getInternalFormat(data.format);
    GLenum format = OpenglUtils::getFormat(data.format);

    int32_t unpackAlignment = data.imageType == ImageType::TEXT
                                  ? 1
                                  : PixelFormat::alignmentInBytes(
                                      data.format,
                                      GL_UNSIGNED_BYTE,
                                      data.width
                                  );
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    if (data.compressedTexture) {
        glCompressedTexImage2D(textureTarget, 0, internalFormat, data.width, data.height, 0, data.bytes, data.pixels);
    } else {
        glTexImage2D(textureTarget,
                     0,
                     internalFormat,
                     data.width,
                     data.height,
                     0,
                     format,
                     GL_UNSIGNED_BYTE,
                     data.pixels);
    }

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_E("glTexImage2D failed, imageType={}, format={}, internalFormat=0x{:x}, formatEnum=0x{:x}, size={}x{}, glError=0x{:x}",
              static_cast<int32_t>(data.imageType),
              static_cast<int32_t>(data.format),
              static_cast<uint32_t>(internalFormat),
              static_cast<uint32_t>(format),
              data.width, data.height,
              static_cast<uint32_t>(error));
    }
    //    free(data.pixels);
    return true;
}

bool GLRenderDevice::upLoadOESTexture(Texture2D* texture, const TextureData& data) {
#ifdef OPENGL_GLFW
#else
#if defined(EGL_NEW_IMAGE_QCOM) && defined(EGL_FORMAT_RGBA_8888_QCOM) && defined(EGL_FORMAT_RGB_888_QCOM) && defined(EGL_IMAGE_FORMAT_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_BASE_ADDR_LOW_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_BASE_ADDR_HIGH_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_SIZE_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_STRIDE_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_MEMORY_TYPE_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_MEMORY_TYPE_PMEM_QCOM) && defined(EGL_IMAGE_EXT_BUFFER_PLANE0_OFFSET_QCOM)
    auto textureImp = dynamic_cast<GlTexture2D*>(texture);
    GLenum textureTarget = textureImp->textureTarget;

    QnxPlatformSharedPtr qnxPlatform = std::static_pointer_cast<QNXEGLPlatform>(m_platform);
    EGLDisplay egl_display = qnxPlatform->getContext()->eglDisplay;
    EGLContext egl_context = qnxPlatform->getContext()->eglContext;

    textureImp->p_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    if (textureImp->p_eglCreateImageKHR == nullptr) {
        LOG_I("ERROR: no eglCreateImageKHR support");
        return false;
    }
    textureImp->p_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
        "glEGLImageTargetTexture2DOES");
    if (textureImp->p_glEGLImageTargetTexture2DOES == NULL) {
        LOG_I("ERROR: no glEGLImageTargetTexture2DOES support");
        return false;
    }

    uint32_t width = data.width;
    uint32_t height = data.height;

    //default GL_RGBA
    uint32_t alignment = 64;
    uint32_t numbers = 4;
    GLenum format = EGL_FORMAT_RGBA_8888_QCOM;
    if (data.format == PixelDataFormat::RGB) {
        //应用端确定
        alignment = 256;
        numbers = 3;
        format = EGL_FORMAT_RGB_888_QCOM;
    }
    uint32_t stride = numbers * ((width + (alignment - 1)) & ~(alignment - 1)); //YUV is 2*...
    uint32_t size = (((stride * height) + 4096 - 1) & ~(4096 - 1));
    EGLint attribs[] = {
        EGL_WIDTH, (GLint)width,
        EGL_HEIGHT, (GLint)height,
        EGL_IMAGE_FORMAT_QCOM, (GLint)format,
        EGL_IMAGE_EXT_BUFFER_BASE_ADDR_LOW_QCOM, 0,
        EGL_IMAGE_EXT_BUFFER_BASE_ADDR_HIGH_QCOM, 0,
        EGL_IMAGE_EXT_BUFFER_SIZE_QCOM, (EGLint)(size),
        EGL_IMAGE_EXT_BUFFER_STRIDE_QCOM, (GLint)stride,
        EGL_IMAGE_EXT_BUFFER_MEMORY_TYPE_QCOM, EGL_IMAGE_EXT_BUFFER_MEMORY_TYPE_PMEM_QCOM,
        EGL_IMAGE_EXT_BUFFER_PLANE0_OFFSET_QCOM, 0, // for RGB format only
        EGL_NONE
    };

    auto buff_addr = (unsigned char*)(data.pixels);

    if (textureImp->m_eglImageMap.find(data.pixels) == textureImp->m_eglImageMap.end()) {
        attribs[7] = (EGLint)(((uint64_t)(buff_addr)) & 0xFFFFFFFF);
        attribs[9] = (EGLint)(((uint64_t)(buff_addr)) >> 32);

        textureImp->m_pixel = (*textureImp->p_eglCreateImageKHR)(egl_display, egl_context, EGL_NEW_IMAGE_QCOM, (EGLClientBuffer)0, attribs);
        if (textureImp->m_pixel == EGL_NO_IMAGE_KHR) {
            LOG_I("ERROR: eglCreateImageKHR failed with Image width {}, height {}, format {}", data.width, data.height, data.format);
            if (textureImp->m_debugTimes == 0) {
                ToolUtils::debugTexture(buff_addr, data.width, data.height, "/data/log/image_" + Math::generate_uuid(), data.isOES, data.format);
                textureImp->m_debugTimes++;
            }
            return false;
        }

        textureImp->m_eglImageMap.insert(std::make_pair(data.pixels, textureImp->m_pixel));
    } else {
        textureImp->m_pixel = textureImp->m_eglImageMap.find(data.pixels)->second;
    }

    if (textureImp->m_pixel != nullptr) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(textureTarget, textureImp->textureID);

        GLint minFilterType = OpenglUtils::getMinFilterType(data.minFilterType);
        GLint magFilterType = OpenglUtils::getMagFilterType(data.magFilterType);

        glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, minFilterType);
        glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, magFilterType);
        glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        (*textureImp->p_glEGLImageTargetTexture2DOES)(textureTarget, (GLeglImageOES)textureImp->m_pixel);
        EGLint errorNumber = eglGetError();
        if (EGL_SUCCESS != errorNumber) {
            LOG_I("eglGetError({}), upload data to GPU error, data is invalid!", errorNumber);
            return false;
        }
    }
#else
    (void)texture;
    (void)data;
    LOG_I("ERROR: QCOM EGL image extensions are not available in this SDK");
    return false;
#endif
#endif
    return true;
}

void GLRenderDevice::enableBlend() {
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void GLRenderDevice::disableBlend() {
    glDisable(GL_BLEND);
}

const std::string SHADERS_FOLDER_PATH = "/var/data/shaders/";

bool GLRenderDevice::makeFolder() {
#ifdef OPENGL_GLFW
#else
    try {
        if (access(SHADERS_FOLDER_PATH.c_str(), 0) == -1) {
            int32_t isCreate = ::mkdir(SHADERS_FOLDER_PATH.c_str(), S_IRWXU);
            if (!isCreate) {
                return true;
            }
            return false;
        } else {
            return true;
        }
    } catch (std::exception const& ex) {
        LOG_E("makeFolder error -- {}", ex.what());
    }
#endif
    return true;
}

GPUProgram* GLRenderDevice::createGPUProgram(const std::string& programFileName, const std::string& vertexShaderStr, const std::string& fragmentShaderStr) {
    // LOG_I("shader: {}, vert: {}, frag: {}", programFileName, vertexShaderStr, fragmentShaderStr);
    GLenum binaryFormat = 0x8740;
    std::string version = "20251031";
    auto programObject = glCreateProgram();

    // Load binary from file
    //    std::ifstream file("shader.bin", std::ios::binary);
    //    std::istreambuf_iterator<char> startIt(file), endIt;
    //    std::vector<char> buffer(startIt, endIt);
    //    file.close();

    // Install shader binary
    //    glProgramBinary(program, format, reinterpret_cast<char *>(buffer.data()), buffer.size());

    bool loadBinarySuccess = false;

    // FILE* fp = fopen((SHADERS_FOLDER_PATH + programFileName + "." + version).c_str(), "r");
    // if (fp) {
    //     fseek(fp, 0, SEEK_END);
    //     size_t file_len = ftell(fp);
    //     fseek(fp, 0, SEEK_SET);
    //     char buffer[file_len];
    //     fread(buffer, file_len, 1, fp);
    //     fclose(fp);
    //
    //     glProgramBinary(programObject, binaryFormat, buffer, file_len);
    //     loadBinarySuccess = checkCompileErrors(programFileName, programObject, "PROGRAM");
    // }

    if (!loadBinarySuccess) {
        const char* vShaderCode = vertexShaderStr.c_str();
        const char* fShaderCode = fragmentShaderStr.c_str();

        uint32_t vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(programFileName, vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(programFileName, fragment, "FRAGMENT");
        // shader Program
        glAttachShader(programObject, vertex);
        glAttachShader(programObject, fragment);
        glLinkProgram(programObject);
        checkCompileErrors(programFileName, programObject, "PROGRAM");

#ifdef OPENGL_GLFW
#else
        //save shader to binary
        // if (makeFolder()) {
        //     GLint formats = 0;
        //     glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
        //     if (formats >= 1) {
        //         // Get the binary length
        //         GLint length = 0;
        //         glGetProgramiv(programObject, GL_PROGRAM_BINARY_LENGTH, &length);
        //
        //         // Retrieve the binary code
        //         std::vector<GLubyte> buffer(length);
        //         GLenum format = 0;
        //         glGetProgramBinary(programObject, length, nullptr, &format, buffer.data());
        //
        //         // Write the binary to a file.
        //         std::string fName(SHADERS_FOLDER_PATH + programFileName + "." + version);
        //         std::ofstream out(fName.c_str(), std::ios::binary);
        //         out.write(reinterpret_cast<char*>(buffer.data()), length);
        //         out.close();
        //         LOG_I("{} Saved Succeed!", programFileName);
        //     }
        // }
#endif

        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    return new GlProgram(programObject);
}

bool GLRenderDevice::checkCompileErrors(const std::string& programFileName, GLuint shader, std::string type) {
    GLint success;
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen > 1) {
                char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
                LOG_I("{}, {}, ERROR: {}", programFileName, type, infoLog);
                free(infoLog);
            } else {
                LOG_E("{}, {}, ERROR", programFileName, type);
            }
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            GLint infoLen = 0;
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen > 1) {
                char* infoLog = (char*)malloc(sizeof(char) * infoLen);
                glGetProgramInfoLog(shader, infoLen, nullptr, infoLog);
                LOG_I("{}, {}, ERROR: {}", programFileName, type, infoLog);
                free(infoLog);
            } else {
                LOG_E("{}, {}, ERROR", programFileName, type);
            }
        }
    }
    return !!success;
}


void GLRenderDevice::useGPUProgram(GPUProgram* program) {
    auto programId = dynamic_cast<GlProgram*>(program)->ProgramID;
    glUseProgram(programId);
}

void GLRenderDevice::deletGPUProgram(GPUProgram* program) {
    glDeleteProgram(dynamic_cast<GlProgram*>(program)->ProgramID);
    delete program;
}

GPUProgramParam* GLRenderDevice::getGPUProgramParam(GPUProgram* program, const std::string& name) {
    return program->getUniformParam(name);
}

void GLRenderDevice::setGPUProgramParamAsInt(GPUProgramParam* param, int32_t value) {
    glUniform1i(dynamic_cast<GlProgramParam*>(param)->location, value);
}


void GLRenderDevice::setGPUProgramParamAsFloat(GPUProgramParam* param, float value) {
    glUniform1f(dynamic_cast<GlProgramParam*>(param)->location, value);
}

void GLRenderDevice::setGPUProgramParamAsVec2(GPUProgramParam* param, float x, float y) {
    glUniform2f(dynamic_cast<GlProgramParam*>(param)->location, x, y);
}

void GLRenderDevice::setGPUProgramParamAsVec3(GPUProgramParam* param, float x, float y, float z) {
    glUniform3f(dynamic_cast<GlProgramParam*>(param)->location, x, y, z);
}

void GLRenderDevice::setGPUProgramParamAsVec4(GPUProgramParam* param, float x, float y, float z, float w) {
    glUniform4f(dynamic_cast<GlProgramParam*>(param)->location, x, y, z, w);
}

void GLRenderDevice::setGPUProgramParamAsMat4(GPUProgramParam* param, const Matrix4& mat) {
    glUniformMatrix4fv(dynamic_cast<GlProgramParam*>(param)->location, 1, /*transpose=*/GL_FALSE, mat.elements);
}

void GLRenderDevice::setGPUProgramParamAsIntArray(GPUProgramParam* param, const int32_t* values, int32_t size, int32_t step) {
    if (step == 1) {
        glUniform1iv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    } else if (step == 2) {
        glUniform2iv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    } else if (step == 3) {
        glUniform3iv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    } else if (step == 4) {
        glUniform4iv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    }
}

void GLRenderDevice::setGPUProgramParamAsFloatArray(GPUProgramParam* param, const float* values, int32_t size, int32_t step) {
    if (step == 1) {
        glUniform1fv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    } else if (step == 2) {
        glUniform2fv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    } else if (step == 3) {
        glUniform3fv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    } else if (step == 4) {
        glUniform4fv(dynamic_cast<GlProgramParam*>(param)->location, size, values);
    }
}

void GLRenderDevice::setGPUProgramParamAsMat4Array(GPUProgramParam* param, const std::vector<Matrix4>& values) {
    //    glUniformMatrix4fv(static_cast<GlProgramParam*>(param)->location, values.size(), /*transpose=*/GL_FALSE, &values[0][0]);
}

void GLRenderDevice::setGPUProgramParamAsInt(GPUProgram* program, const std::string& uniformName, int32_t value) {
}

void GLRenderDevice::setGPUProgramParamAsFloat(GPUProgram* program, const std::string& uniformName, float value) {
}

void GLRenderDevice::setGPUProgramParamAsMat4(GPUProgram* program, const std::string& uniformName, const Matrix4& mat) {
}

void GLRenderDevice::setGPUProgramParamAsIntArray(GPUProgram* program, const std::string& uniformName, const int32_t* values, int32_t size, int32_t step) {
}

void GLRenderDevice::setGPUProgramParamAsFloatArray(GPUProgram* program, const std::string& uniformName, const float* values, int32_t size, int32_t step) {
}

void GLRenderDevice::setGPUProgramParamAsVec2(GPUProgram* program, const std::string& uniformName, float x, float y) {
}

void GLRenderDevice::setGPUProgramParamAsVec3(GPUProgram* program, const std::string& uniformName, float x, float y, float z) {
}

void GLRenderDevice::setGPUProgramParamAsVec4(GPUProgram* program, const std::string& uniformName, float x, float y, float z, float w) {
}

//---------------------------------------------------UBO---------------------------------------------------
UBO* GLRenderDevice::createUBO() {
    auto ubo = new GlUBO;
    glGenBuffers(1, &ubo->bufferID);
    return ubo;
}

void GLRenderDevice::updateUBO(UBO* ubo, std::shared_ptr<UBOData> uboData) {
    auto* glUbo = (GlUBO*)ubo;
    if (!glUbo || !uboData) {
        return;
    }

    const uint32_t byteSize = uboData->size > 0 ? uboData->size : static_cast<uint32_t>(uboData->data.size());
    if (byteSize == 0 || uboData->data.empty()) {
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, glUbo->bufferID);
    glBufferData(GL_UNIFORM_BUFFER, byteSize, uboData->data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GLRenderDevice::bindUBO(GPUProgram* program, UBO* ubo, const std::string& blockName, uint32_t bindingPoint) {
    auto* glUbo = (GlUBO*)ubo;
    GLuint programID = dynamic_cast<GlProgram*>(program)->ProgramID;
    GLuint blockIndex = glGetUniformBlockIndex(programID, blockName.c_str());

    if (blockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(programID, blockIndex, bindingPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, glUbo->bufferID);
    }
}

SSBO* GLRenderDevice::createSSBO() {
    auto ssbo = new GlSSBO();
    glGenBuffers(1, &ssbo->bufferID);
    return ssbo;
}

void GLRenderDevice::updateSSBO(SSBO* ssbo, std::shared_ptr<SSBOData> ssboData, uint32_t bindingPoint) {
    auto* glSsbo = (GlSSBO*)ssbo;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glSsbo->bufferID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ssboData->size, ssboData->data.data(), GL_STREAM_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, glSsbo->bufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLint boundBuffer = 0;
    glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, bindingPoint, &boundBuffer);
    if (boundBuffer != glSsbo->bufferID) {
        LOG_E("SSBO bound at point {}: {} (expected: {})", bindingPoint, boundBuffer, glSsbo->bufferID);
    }
}

void* GLRenderDevice::insertFence() {
    return (void*)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool GLRenderDevice::waitFence(void* fence, uint64_t timeoutNs) {
    if (!fence) return true;
    GLenum r = glClientWaitSync((GLsync)fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeoutNs);
    return (r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED);
}

void GLRenderDevice::deleteFence(void* fence) {
    if (!fence) return;
    glDeleteSync((GLsync)fence);
}

// ---------------------------------------------------------------------------
// RenderTarget (FBO)
// ---------------------------------------------------------------------------

/// Internal struct holding a GL FBO + colour texture + depth RBO.
class GlRenderTarget : public RenderTarget {
public:
    RenderTarget* getReal() override { return this; }

    GLuint fboID        = 0;
    GLuint resolveFBOID = 0;
    GLuint colorTexID   = 0;
    GLuint colorRBOID   = 0;
    GLuint depthRBOID   = 0;
    int32_t width       = 0;
    int32_t height      = 0;
    int32_t samples     = 1;
};

RenderTarget* GLRenderDevice::createRenderTarget(int32_t w, int32_t h,
                                                  Texture2D** outColorTexture) {
    auto* rt = new GlRenderTarget();
    rt->width  = w;
    rt->height = h;

    GLint maxSamples = 1;
#ifdef OPENGL_GLFW
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
#else
    if (eglGetCurrentContext() != EGL_NO_CONTEXT) {
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    }
#endif
    const int32_t requestedSamples = m_platform ? std::max(m_platform->getRequestedSamples(), 1) : 1;
    rt->samples = std::max(1, std::min(requestedSamples, maxSamples));
    LOG_I("GLRenderDevice::createRenderTarget {}x{}, requestedSamples={}, actualSamples={}",
          w,
          h,
          requestedSamples,
          rt->samples);

    // Resolve colour texture (always sampleable by the display quad)
    glGenTextures(1, &rt->colorTexID);
    glBindTexture(GL_TEXTURE_2D, rt->colorTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (rt->samples > 1) {
        glGenRenderbuffers(1, &rt->colorRBOID);
        glBindRenderbuffer(GL_RENDERBUFFER, rt->colorRBOID);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, rt->samples, GL_RGBA8, w, h);

        glGenRenderbuffers(1, &rt->depthRBOID);
        glBindRenderbuffer(GL_RENDERBUFFER, rt->depthRBOID);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, rt->samples, GL_DEPTH_COMPONENT16, w, h);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glGenFramebuffers(1, &rt->fboID);
        glBindFramebuffer(GL_FRAMEBUFFER, rt->fboID);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rt->colorRBOID);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depthRBOID);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_E("GLRenderDevice: MSAA FBO incomplete, status=0x{:X}, samples={}", status, rt->samples);
        }

        glGenFramebuffers(1, &rt->resolveFBOID);
        glBindFramebuffer(GL_FRAMEBUFFER, rt->resolveFBOID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->colorTexID, 0);

        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_E("GLRenderDevice: resolve FBO incomplete, status=0x{:X}", status);
        }
    } else {
        glGenRenderbuffers(1, &rt->depthRBOID);
        glBindRenderbuffer(GL_RENDERBUFFER, rt->depthRBOID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glGenFramebuffers(1, &rt->fboID);
        glBindFramebuffer(GL_FRAMEBUFFER, rt->fboID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->colorTexID, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depthRBOID);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            LOG_E("GLRenderDevice: FBO incomplete, status=0x{:X}", status);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Optionally expose the colour texture via a thin GlTexture2D wrapper
    if (outColorTexture) {
        auto* colorTex = new GlTexture2D(rt->colorTexID);
        // The FBO owns the GL texture; this wrapper must NOT delete it.
        // We mark it by setting a flag so deleteTexture2D skips glDeleteTextures.
        colorTex->m_ownedByFBO = true;
        *outColorTexture = colorTex;
    }

    return rt;
}

void GLRenderDevice::deleteRenderTarget(RenderTarget* rt) {
    if (!rt) return;
    auto* glRt = static_cast<GlRenderTarget*>(rt);
    if (m_boundRenderTarget == rt) {
        m_boundRenderTarget = nullptr;
    }
    if (glRt->fboID) glDeleteFramebuffers(1, &glRt->fboID);
    if (glRt->resolveFBOID) glDeleteFramebuffers(1, &glRt->resolveFBOID);
    if (glRt->colorTexID) glDeleteTextures(1, &glRt->colorTexID);
    if (glRt->colorRBOID) glDeleteRenderbuffers(1, &glRt->colorRBOID);
    if (glRt->depthRBOID) glDeleteRenderbuffers(1, &glRt->depthRBOID);
    delete glRt;
}

void GLRenderDevice::bindRenderTarget(RenderTarget* rt) {
    if (!rt) return;
    auto* glRt = static_cast<GlRenderTarget*>(rt);
    glBindFramebuffer(GL_FRAMEBUFFER, glRt->fboID);
    glViewport(0, 0, glRt->width, glRt->height);
    m_boundRenderTarget = rt;
}

void GLRenderDevice::unbindRenderTarget() {
    if (m_boundRenderTarget) {
        auto* glRt = static_cast<GlRenderTarget*>(m_boundRenderTarget);
        if (glRt->resolveFBOID != 0 && glRt->samples > 1) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, glRt->fboID);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glRt->resolveFBOID);
            glBlitFramebuffer(0, 0, glRt->width, glRt->height,
                              0, 0, glRt->width, glRt->height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_boundRenderTarget = nullptr;
}

// ---------------------------------------------------------------------------
// Depth / rasteriser state
// ---------------------------------------------------------------------------

void GLRenderDevice::setDepthTest(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
        // Keep a deterministic depth compare state even if external code changed it.
        glDepthFunc(GL_LEQUAL);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void GLRenderDevice::setDepthWrite(bool enable) {
    glDepthMask(enable ? GL_TRUE : GL_FALSE);
}

void GLRenderDevice::setCullFace(CullFaceMode mode) {
    if (mode == CullFaceMode::NONE) {
        glDisable(GL_CULL_FACE);
        return;
    }
    glEnable(GL_CULL_FACE);
    switch (mode) {
        case CullFaceMode::FRONT:          glCullFace(GL_FRONT);          break;
        case CullFaceMode::BACK:           glCullFace(GL_BACK);           break;
        case CullFaceMode::FRONT_AND_BACK: glCullFace(GL_FRONT_AND_BACK); break;
        default: break;
    }
}

void GLRenderDevice::clearDepth() {
    GLboolean depthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    if (depthMask != GL_TRUE) {
        glDepthMask(GL_TRUE);
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    if (depthMask != GL_TRUE) {
        glDepthMask(depthMask);
    }
}

} // MORROWGUI


