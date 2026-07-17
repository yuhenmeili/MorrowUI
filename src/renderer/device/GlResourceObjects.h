// GlResourceObjects.h
//
// GPU 资源对象的内部定义 — GL 资源的轻量 RAII 包装。
// 由 GLRenderDevice 和 ResourceRegistry 共享，确保 delete 时类型完整。

#pragma once

#ifdef OPENGL_GLFW
#include "platform/wgl/OpenglHeader.h"
#else
#include "platform/egl/EGLHeader.h"
#include "platform/egl/GLESHeader.h"
#endif

#include <string>
#include <unordered_map>

namespace morrow {

class GlProgram {
public:
    explicit GlProgram(GLuint id) : ProgramID(id) {}

    ~GlProgram() { glDeleteProgram(ProgramID); }

    GLint getUniformLocation(const std::string& name) const {
        return glGetUniformLocation(ProgramID, name.c_str());
    }

    GLint getAttribLocation(const std::string& name) const {
        return glGetAttribLocation(ProgramID, name.c_str());
    }

    GLuint ProgramID;
};

class GlTexture2D {
public:
    explicit GlTexture2D(GLuint id) : textureID(id) {}

    ~GlTexture2D() {
        if (!m_ownedByFBO && textureID) glDeleteTextures(1, &textureID);
    }

    GLuint textureID;
    GLenum textureTarget = GL_TEXTURE_2D;
    bool   m_ownedByFBO  = false;
#ifdef OPENGL_EGL
    PFNEGLCREATEIMAGEKHRPROC p_eglCreateImageKHR = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC p_glEGLImageTargetTexture2DOES = nullptr;
    EGLImageKHR m_pixel = nullptr;
    std::unordered_map<void*, EGLImageKHR> m_eglImageMap;
#endif
    uint32_t m_debugTimes = 0;
};

class GlVBO {
public:
    ~GlVBO() {
        if (vertexArrayID) glDeleteVertexArrays(1, &vertexArrayID);
        if (vertexbuffer) glDeleteBuffers(1, &vertexbuffer);
        if (elementbuffer) glDeleteBuffers(1, &elementbuffer);
    }

    GLuint vertexArrayID = 0;
    GLuint vertexbuffer = 0;
    GLuint verticesCount = 0;
    GLuint elementbuffer = 0;
    GLuint indicesCount = 0;
    GLenum drawMode = GL_TRIANGLES;
};

class GlUBO {
public:
    ~GlUBO() { if (bufferID) glDeleteBuffers(1, &bufferID); }

    GLuint bufferID = 0;
    size_t bufferSize = 0;
    uint32_t bindingPoint = 0;
};

class GlSSBO {
public:
    ~GlSSBO() { if (bufferID) glDeleteBuffers(1, &bufferID); }

    GLuint bufferID = 0;
    size_t bufferSize = 0;
    uint32_t bindingPoint = 0;
};

class GlRenderTarget {
public:
    ~GlRenderTarget() {
        if (fboID) glDeleteFramebuffers(1, &fboID);
        if (resolveFBOID) glDeleteFramebuffers(1, &resolveFBOID);
        if (colorTexID) glDeleteTextures(1, &colorTexID);
        if (colorRBOID) glDeleteRenderbuffers(1, &colorRBOID);
        if (depthRBOID) glDeleteRenderbuffers(1, &depthRBOID);
    }

    GLuint fboID = 0;
    GLuint resolveFBOID = 0;
    GLuint colorTexID = 0;
    GLuint colorRBOID = 0;
    GLuint depthRBOID = 0;
    int32_t width = 0, height = 0;
    int32_t samples = 1;
};

} // namespace morrow
