//
// Created by 0060328 on 25-9-18.
//

#ifndef GLESHEADER_H
#define GLESHEADER_H
#if defined(__has_include)
#if __has_include(<GLES3/gl32.h>)
#include <GLES3/gl32.h>
#elif __has_include(<GLES3/gl31.h>)
#include <GLES3/gl31.h>
#elif __has_include(<GLES3/gl3.h>)
#include <GLES3/gl3.h>
#elif __has_include(<GLES2/gl2.h>)
#include <GLES2/gl2.h>
#else
#error "No supported GLES header found"
#endif
#if __has_include(<GLES3/gl3ext.h>)
#include <GLES3/gl3ext.h>
#endif
#if __has_include(<GLES2/gl2ext.h>)
#include <GLES2/gl2ext.h>
#endif
#else
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif

// 引擎按 OpenGL ES 3.2 目标使用 glBufferStorage（ES 3.2 核心 API，SSBO 持久映射
// 环形分段的唯一上传路径，无回退）。旧 SDK 头文件只到 gl31 时补齐原型与常量
// （值与 ES 3.2 / EXT_buffer_storage 一致），符号由 libGLESv2 直接链接导出。
#ifndef GL_ES_VERSION_3_2
#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif
#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT 0x0080
#endif
#ifndef GL_DYNAMIC_STORAGE_BIT
#define GL_DYNAMIC_STORAGE_BIT 0x0100
#endif
extern "C" {
GL_APICALL void GL_APIENTRY glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
}
#endif
#endif //GLESHEADER_H
