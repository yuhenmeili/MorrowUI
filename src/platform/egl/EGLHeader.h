//
// Created by 0060328 on 25-9-18.
//

#ifndef EGLHEADER_H
#define EGLHEADER_H
#include "EGL/egl.h"

#if defined(__has_include)
#if __has_include("EGL/eglextQCOM.h")
#include "EGL/eglextQCOM.h"
#elif __has_include("EGL/eglext.h")
#include "EGL/eglext.h"
#endif
#else
#include "EGL/eglext.h"
#endif
#endif //EGLHEADER_H
