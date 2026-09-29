//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_OFFSCREENRENDERTARGET_H
#define MORROW_GUI_OFFSCREENRENDERTARGET_H
#include "RenderDeviceProxyBase.h"

namespace morrow {
class OffscreenRenderTarget {
public:
    void create(int32_t w, int32_t h);

    void resize(int32_t w, int32_t h);

    void destroy();

    /// The opaque FBO handle – passed to bindRenderTarget / unbindRenderTarget.
    [[nodiscard]] HwRenderTarget getRenderTarget() const;

    /// Colour attachment HwTexture2D – passed to useTexture2D() for the display quad.
    [[nodiscard]] HwTexture2D getColorTexture() const;

    [[nodiscard]] int32_t getWidth() const {
        return m_width;
    }

    [[nodiscard]] int32_t getHeight() const {
        return m_height;
    }

private:
    HwRenderTarget m_renderTarget{0};
    HwTexture2D m_colorTexture{0};
    int32_t m_width = 0, m_height = 0;
};
}  // namespace morrow

#endif  // MORROW_GUI_OFFSCREENRENDERTARGET_H
