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

// ── buffer storage 兼容 ──
// glBufferStorage / GL_MAP_PERSISTENT_BIT / GL_MAP_COHERENT_BIT 是桌面 GL 4.4
// 的核心 API；OpenGL ES 任何版本（含 3.2）都未将其纳入核心，只有
// EXT_buffer_storage 扩展（常量与函数名带 _EXT 后缀）。因此不能用
// GL_ES_VERSION_3_2 等版本宏判断符号可用性——QNX SDP 自带 gl32.h，但其中
// 并无这些符号。
// 常量按「先 _EXT 后数值」补齐；glBufferStorageEXT 原型在 gl2ext.h 中包在
// GL_GLEXT_PROTOTYPES 里（默认只有 PFNGLBUFFERSTORAGEEXTPROC typedef），
// 按扩展函数规范由调用方经 eglGetProcAddress 运行时加载（见
// GLRenderDevice::ensureSSBOStorage），此处不做链接期声明。
#ifndef GL_MAP_PERSISTENT_BIT
#ifdef GL_MAP_PERSISTENT_BIT_EXT
#define GL_MAP_PERSISTENT_BIT GL_MAP_PERSISTENT_BIT_EXT
#else
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif
#endif
#ifndef GL_MAP_COHERENT_BIT
#ifdef GL_MAP_COHERENT_BIT_EXT
#define GL_MAP_COHERENT_BIT GL_MAP_COHERENT_BIT_EXT
#else
#define GL_MAP_COHERENT_BIT 0x0080
#endif
#endif
#ifndef GL_DYNAMIC_STORAGE_BIT
#ifdef GL_DYNAMIC_STORAGE_BIT_EXT
#define GL_DYNAMIC_STORAGE_BIT GL_DYNAMIC_STORAGE_BIT_EXT
#else
#define GL_DYNAMIC_STORAGE_BIT 0x0100
#endif
#endif
#endif //GLESHEADER_H
