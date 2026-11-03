//
// Created by 0060328 on 25-9-18.
//

#ifndef EGLHEADER_H
#define EGLHEADER_H
#include "EGL/egl.h"

#if defined(__has_include)
#if __has_include("EGL/eglextQCOM.h")
#include "EGL/eglextQCOM.h"
#elif __has_include("eglextQCOM.h")
// SDK/BSP 未提供 QCOM 扩展头时，使用仓库收口副本（src/extern/common，
// 经 PRIVATE include 可达）。缺失时 QCOM 快速上传路径的扩展宏无定义。
#include "eglextQCOM.h"
#elif __has_include("EGL/eglext.h")
#include "EGL/eglext.h"
#endif
#else
#include "EGL/eglext.h"
#endif
#endif //EGLHEADER_H
