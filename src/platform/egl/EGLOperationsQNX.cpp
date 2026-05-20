//
// Created by 0060328 on 25-9-18.
//

#include "EGLOperationsQNX.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "../../utils/Log.h"

namespace morrow {
EGLOperationsQNX::EGLOperationsQNX(int32_t samples)
    : m_requestedSamples(std::max(samples, 1)) {
    m_esContext = std::make_shared<ESContext>();
}

EGLOperationsQNX::~EGLOperationsQNX() {
    eglDestroyContext(m_esContext->eglDisplay, m_esContext->eglContext);
    eglTerminate(m_esContext->eglDisplay);
    screen_destroy_event(m_esContext->screen_ev);
    screen_destroy_context(m_esContext->screen_ctx);
}

bool EGLOperationsQNX::initialize() {
    if (EXIT_SUCCESS == initScreen() && EXIT_SUCCESS == initEGL()) {
        LOG_I("initContext successfully");
    }
    return true;
}

ESContextSharedPtr EGLOperationsQNX::getContext() {
    return m_esContext;
}

int32_t EGLOperationsQNX::initScreen() {
    int32_t rc = screen_create_context(&m_esContext->screen_ctx, SCREEN_APPLICATION_CONTEXT);
    if (rc) {
        LOG_I("screen_create_context failed");
        return EXIT_FAILURE;
    }

    rc = screen_create_event(&m_esContext->screen_ev);
    if (rc) {
        LOG_I("screen_create_event failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int32_t EGLOperationsQNX::initEGL() {
    EGLBoolean rc;
    EGLint num_confs = 0;

    auto buildConfigAttribList = [](int32_t samples) {
        std::vector<EGLint> attribs = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        };
        if (samples > 1) {
            attribs.push_back(EGL_SAMPLE_BUFFERS);
            attribs.push_back(1);
            attribs.push_back(EGL_SAMPLES);
            attribs.push_back(samples);
        }
        attribs.push_back(EGL_NONE);
        return attribs;
    };

    const EGLint egl_context_attrib_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    m_esContext->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_esContext->eglDisplay == EGL_NO_DISPLAY) {
        LOG_I("eglGetDisplay failed");
        return EXIT_FAILURE;
    }

    rc = eglInitialize(m_esContext->eglDisplay, nullptr, nullptr);
    if (rc != EGL_TRUE) {
        LOG_I("eglInitialize failed");
        return EXIT_FAILURE;
    }

    auto configAttribs = buildConfigAttribList(m_requestedSamples);
    rc = eglChooseConfig(m_esContext->eglDisplay, configAttribs.data(), &m_esContext->egl_conf, 1, &num_confs);
    if ((rc != EGL_TRUE) || (num_confs == 0)) {
        if (m_requestedSamples > 1) {
            LOG_W("eglChooseConfig failed for requested samples={}, retry without MSAA", m_requestedSamples);
            m_requestedSamples = 1;
            configAttribs = buildConfigAttribList(m_requestedSamples);
            rc = eglChooseConfig(m_esContext->eglDisplay, configAttribs.data(), &m_esContext->egl_conf, 1, &num_confs);
        }
        if ((rc != EGL_TRUE) || (num_confs == 0)) {
            LOG_I("eglChooseConfig failed");
            return EXIT_FAILURE;
        }
    }

    EGLint actualSamples = 0;
    EGLint sampleBuffers = 0;
    eglGetConfigAttrib(m_esContext->eglDisplay, m_esContext->egl_conf, EGL_SAMPLES, &actualSamples);
    eglGetConfigAttrib(m_esContext->eglDisplay, m_esContext->egl_conf, EGL_SAMPLE_BUFFERS, &sampleBuffers);
    LOG_I("QNX EGL config selected: requestedSamples={}, sampleBuffers={}, samples={}",
          m_requestedSamples,
          sampleBuffers,
          actualSamples);

    m_esContext->eglContext = eglCreateContext(m_esContext->eglDisplay, m_esContext->egl_conf, EGL_NO_CONTEXT, egl_context_attrib_list);
    if (m_esContext->eglContext == EGL_NO_CONTEXT) {
        LOG_I("eglCreateContext failed");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
} // morrow
