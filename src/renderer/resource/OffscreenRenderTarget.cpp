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
    m_colorTexture = RENDERINGTHREAD->createTexture2D(ImageType::IMAGE);
    m_renderTarget = RENDERINGTHREAD->createRenderTarget(w, h, m_colorTexture);
}

void OffscreenRenderTarget::resize(int32_t w, int32_t h) {
    if (w == m_width && h == m_height)
        return;
    destroy();
    create(w, h);
}

void OffscreenRenderTarget::destroy() {
    if (m_renderTarget.isValid()) {
        RENDERINGTHREAD->deleteRenderTarget(m_renderTarget);
        m_renderTarget = HwRenderTarget{0};
    }
    if (m_colorTexture.isValid()) {
        RENDERINGTHREAD->deleteTexture2D(m_colorTexture);
        m_colorTexture = HwTexture2D{0};
    }
    m_width = m_height = 0;
}

HwTexture2D OffscreenRenderTarget::getColorTexture() const {
    return m_colorTexture;
}

HwRenderTarget OffscreenRenderTarget::getRenderTarget() const {
    return m_renderTarget;
}
}  // namespace morrow
