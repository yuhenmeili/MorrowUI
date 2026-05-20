//
// Created by lance on 2023/12/11.
//

#ifndef MORROW_RENDERER_ESCONTEXT_H_
#define MORROW_RENDERER_ESCONTEXT_H_

#include "EGL/egl.h"
#include <screen/screen.h>

namespace morrow
{
struct ESContext
{
    screen_context_t screen_ctx = nullptr;
    screen_event_t screen_ev = nullptr;
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;
    EGLConfig egl_conf = nullptr;
};

using ESContextSharedPtr = std::shared_ptr<ESContext>;
}
#endif //MORROW_RENDERER_ESCONTEXT_H_
