//
// SafeStreamTexture.cpp — 安全流媒体纹理组件实现
//
// 着色器硬编码于 .rodata，纹理在构造时预分配。
// 每帧通过 updateFrame() 更新像素数据（glTexSubImage2D / 全量上传）。
// QNX 下 updateFromEGLImage() 绕过 CPU 拷贝，直接绑定 EGLImage。
//

#include "SafeStreamTexture.h"

#include <cstring>
#include <utility>

#include "GlobalObject.h"
#include "Material.h"
#include "RenderDeviceProxyBase.h"
#include "base/Mesh.h"
#include "base/MeshFilter.h"
#include "utils/Log.h"

namespace morrow {
using namespace Math;

// =========================================================================
// 硬编码图像着色器（引擎内置 ROM）
//   顶点：position + texCoord → MVP 变换
//   片元：texture 采样 × alpha 混合
// =========================================================================
#ifdef OPENGL_EGL
static const char kStreamTextureVert[] = R"GLSL(#version 320 es
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) out vec2 v_texCoord;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kStreamTextureFrag[] = R"GLSL(#version 320 es
#extension GL_OES_EGL_image_external : require
#extension GL_OES_EGL_image_external_essl3 : require

layout (location = 0) in vec2 v_texCoord;

uniform samplerExternalOES u_texture;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_texCoord);
    fragColor.a *= u_alpha;
}
)GLSL";
#else
static const char kStreamTextureVert[] = R"GLSL(#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texCoord;

layout (location = 0) out vec2 v_texCoord;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}
)GLSL";

static const char kStreamTextureFrag[] = R"GLSL(#version 460 core
layout (location = 0) in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform float u_alpha;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_texCoord);
    fragColor.a *= u_alpha;
}
)GLSL";

#endif

// =========================================================================
// SafeStreamTexture 实现
// =========================================================================

SafeStreamTextureSharedPtr SafeStreamTexture::create(int width, int height) {
    return SafeStreamTextureSharedPtr(new SafeStreamTexture(width, height));
}

SafeStreamTexture::SafeStreamTexture(int width, int height) : m_width(width), m_height(height) {
    setWidgetType("SafeStreamTexture");

    // 预分配 GPU 纹理
    initTexture();

    // 注入内置着色器
    initShader();
}

// -------------------------------------------------------------------------
// 着色器初始化
// -------------------------------------------------------------------------
void SafeStreamTexture::initShader() {
    m_material->setShaderFromMemory("safe_stream_texture", std::string(kStreamTextureVert), std::string(kStreamTextureFrag));
    m_material->setBlendEnabled(false);
    m_material->setFloat("u_alpha", 1.0f);
    m_material->setTexture("u_texture", m_streamTexture);
}

// -------------------------------------------------------------------------
// 纹理初始化（预分配 GPU 纹理，后续仅更新数据）
// -------------------------------------------------------------------------
void SafeStreamTexture::initTexture() {
#ifdef OPENGL_EGL
    m_streamTexture = Texture::create(ImageType::OES);
#else
    m_streamTexture = Texture::create(ImageType::IMAGE);
#endif

    // OES 纹理只创建 GPU 纹理对象；首次有效视频帧通过
    // setOESTextureData() 提交外部 buffer 和 GPU 完成回调。
#ifndef OPENGL_EGL
    std::vector<unsigned char> initData(m_width * m_height * 4, 0);
    m_streamTexture->setTextureData(initData.data(), m_width, m_height, PixelDataFormat::RGBA, 4, false);
#endif
    m_streamTexture->setMinFilterType(SamplerMinFilter::LINEAR);
    m_streamTexture->setMagFilterType(SamplerMagFilter::LINEAR);
}

// -------------------------------------------------------------------------
// 帧数据更新（RGBA 像素替换）
// -------------------------------------------------------------------------
#ifdef OPENGL_EGL
void SafeStreamTexture::updateFromEGLImage(void* eglImage, std::function<void()> gpuUseCompleteCallback) {
    if (!eglImage || !m_streamTexture)
        return;

    m_streamTexture->setOESTextureData(eglImage, m_width, m_height, PixelDataFormat::RGBA, 0, std::move(gpuUseCompleteCallback));
    requestRender("SafeStreamTexture::updateFromEGLImage");
}
#endif

void SafeStreamTexture::updateFrame(const unsigned char* rgbaData) {
    if (!rgbaData || !m_streamTexture)
        return;

    // 重新设置纹理数据（内部调用 glTexImage2D / glTexSubImage2D）
    m_streamTexture->setTextureData(const_cast<unsigned char*>(rgbaData), m_width, m_height, PixelDataFormat::RGBA, 4, false);

    // 触发渲染刷新
    requestRender("SafeStreamTexture::updateFrame");
}
}  // namespace morrow
