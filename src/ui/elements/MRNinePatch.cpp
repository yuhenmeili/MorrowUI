//
// 九宫格实现：update 每帧按当前尺寸与 inset 重排 9 个象限（容器布局模式，
// 值相等时 Transform 早退保证零额外开销）。UV 切片随 inset 固定，仅在
// setTexture/setInset 时重算。
//

#include "morrow/elements/MRNinePatch.h"

#include <algorithm>

#include "morrow/scene3d/FrameState.h"
#include "morrow/base/Interaction.h"
#include "morrow/base/Transform.h"

namespace morrow {

MRNinePatch::MRNinePatch() : UIWidget(false) {
    setWidgetType("MRNinePatch");
    for (auto& cell : m_cells) {
        cell = MRImage::create();
        if (auto interaction = cell->getComponent<Interaction>())
            interaction->setInteractionEnabled(false);
        addChild(cell);
    }
}

std::shared_ptr<MRNinePatch> MRNinePatch::create() {
    return std::make_shared<MRNinePatch>();
}

void MRNinePatch::setTexture(TextureSharedPtr texture) {
    m_texture = std::move(texture);
    m_textureWidth = 0.0f;
    m_textureHeight = 0.0f;
    if (m_texture) {
        m_textureWidth = static_cast<float>(m_texture->getWidth());
        m_textureHeight = static_cast<float>(m_texture->getHeight());
    }
    for (auto& cell : m_cells)
        cell->setTexture(m_texture);
    requestRender("ninePatchTexture");
}

void MRNinePatch::setInset(float left, float top, float right, float bottom) {
    m_insetLeft = std::max(0.0f, left);
    m_insetTop = std::max(0.0f, top);
    m_insetRight = std::max(0.0f, right);
    m_insetBottom = std::max(0.0f, bottom);
    requestRender("ninePatchInset");
}

void MRNinePatch::setInset(float inset) {
    setInset(inset, inset, inset, inset);
}

void MRNinePatch::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    UIWidget::update(frameState);
}

void MRNinePatch::layoutChildren() {
    if (m_textureWidth <= 0.0f || m_textureHeight <= 0.0f)
        return;
    const Vector3 size = getComponent<Transform>()->getSize();
    if (size.x <= 0.0f || size.y <= 0.0f)
        return;

    // 中间区至少 1px：inset 超出显示尺寸时按比例收缩
    float left = std::min(m_insetLeft, size.x * 0.5f - 0.5f);
    float right = std::min(m_insetRight, size.x * 0.5f - 0.5f);
    float top = std::min(m_insetTop, size.y * 0.5f - 0.5f);
    float bottom = std::min(m_insetBottom, size.y * 0.5f - 0.5f);

    const float columnSpans[3] = {left, std::max(1.0f, size.x - left - right), std::max(1.0f, right)};
    const float rowSpans[3] = {top, std::max(1.0f, size.y - top - bottom), std::max(1.0f, bottom)};

    // 源纹理的 3x3 切片（像素 → UV；inset 钳制在纹理内）
    const float texLeft = std::min(m_insetLeft, m_textureWidth * 0.5f);
    const float texRight = std::min(m_insetRight, m_textureWidth * 0.5f);
    const float texTop = std::min(m_insetTop, m_textureHeight * 0.5f);
    const float texBottom = std::min(m_insetBottom, m_textureHeight * 0.5f);
    const float columnUV[3][2] = {{0.0f, texLeft / m_textureWidth},
                                  {texLeft / m_textureWidth, (m_textureWidth - texRight) / m_textureWidth},
                                  {(m_textureWidth - texRight) / m_textureWidth, 1.0f}};
    const float rowUV[3][2] = {{0.0f, texTop / m_textureHeight},
                               {texTop / m_textureHeight, (m_textureHeight - texBottom) / m_textureHeight},
                               {(m_textureHeight - texBottom) / m_textureHeight, 1.0f}};

    float offsetY = 0.0f;
    for (int row = 0; row < 3; ++row) {
        float offsetX = 0.0f;
        for (int column = 0; column < 3; ++column) {
            const size_t index = static_cast<size_t>(row * 3 + column);
            const auto& cell = m_cells[index];
            cell->getTransform()->setPosition(offsetX, offsetY, 0.0f);
            cell->getTransform()->setSize(columnSpans[column], rowSpans[row]);
            cell->setUVData(Vector2(columnUV[column][0], rowUV[row][0]), Vector2(columnUV[column][1], rowUV[row][1]));
            offsetX += columnSpans[column];
        }
        offsetY += rowSpans[row];
    }
}

}  // namespace morrow
