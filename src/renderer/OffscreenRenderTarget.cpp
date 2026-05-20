//
// Created by lance on 2026/3/31.
//

#include "OffscreenRenderTarget.h"

#include "GlobalObject.h"
#include "GpuTypes.h"

namespace morrow {
void OffscreenRenderTarget::create(int32_t w, int32_t h) {
    m_width = w;
    m_height = h;
    m_renderTarget = RENDERINGTHREAD->createRenderTarget(w, h, &m_colorTexture);
}

void OffscreenRenderTarget::resize(int32_t w, int32_t h) {
    if (w == m_width && h == m_height) return;
    destroy();
    create(w, h);
}

void OffscreenRenderTarget::destroy() {
    if (m_colorTexture) {
        RENDERINGTHREAD->deleteTexture2D(m_colorTexture);
        m_colorTexture = nullptr;
    }
    if (m_renderTarget) {
        RENDERINGTHREAD->deleteRenderTarget(m_renderTarget);
        m_renderTarget = nullptr;
    }
    m_width = m_height = 0;
}

Texture2D* OffscreenRenderTarget::getColorTexture() const {
    return m_colorTexture;
}

RenderTarget* OffscreenRenderTarget::getRenderTarget() const {
    return m_renderTarget;
}
} // morrow
