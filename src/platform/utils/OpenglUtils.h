//
// Created by 0060328 on 25-9-18.
//

#ifndef OPENGLUTILS_H
#define OPENGLUTILS_H
#include "DriverEnums.h"
#include "ssbo/SSBOFieldBinding.h"
#ifdef OPENGL_GLFW
#include "wgl/OpenglHeader.h"  // 桌面OpenGL
#else
#include "platform/egl/GLESHeader.h"
#endif

namespace morrow {
namespace OpenglUtils {
/// 获取CPU侧像素数据格式
/// @param pixelDataFormat
/// @return
GLenum getFormat(PixelDataFormat pixelDataFormat);

/// 获取GPU内部存储格式
/// @param pixelDataFormat
/// @return
GLenum getInternalFormat(PixelDataFormat pixelDataFormat);

GLint getMinFilterType(SamplerMinFilter minFilterType);

GLint getMagFilterType(SamplerMagFilter magFilterType);

GLenum toGLBlendFactor(BlendFactor factor);

ShaderDataType mapGLTypeToShaderDataType(GLenum type);
}  // namespace OpenglUtils
}  // namespace morrow
#endif  // OPENGLUTILS_H
