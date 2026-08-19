#include "MRCheckBox.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRCheckBox> MRCheckBox::create() {
    auto button = std::shared_ptr<MRCheckBox>(new MRCheckBox());
    button->initializeIndicator();
    return button;
}

MRCheckBox::MRCheckBox() {
    setWidgetType("MRCheckBox");
    setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutIndicator(); });
}

void MRCheckBox::updateVisualState() {
    MRSelectableButton::updateVisualState();
    if (m_indicator) {
        m_indicator->setColor(isChecked() ? Vector4(0.08f, 0.42f, 0.32f, 1.0f) : Vector4(0.68f, 0.72f, 0.78f, 1.0f));
    }
}

void MRCheckBox::initializeIndicator() {
    m_indicator = MRColor::create();
    m_indicator->setRounding(4.0f);
    addChild(m_indicator);
    layoutIndicator();
    updateVisualState();
}

void MRCheckBox::layoutIndicator() {
    if (!m_indicator)
        return;
    const Vector3 size = getComponent<Transform>()->getSize();
    constexpr float indicatorSize = 24.0f;
    constexpr float leftPadding = 12.0f;
    constexpr float textOffset = 48.0f;
    m_indicator->getComponent<Transform>()->setSize(indicatorSize, indicatorSize);
    m_indicator->getComponent<Transform>()->setPosition(leftPadding, (size.y - indicatorSize) * 0.5f, 0.0f);
    if (m_label) {
        m_label->getComponent<Transform>()->setPosition(textOffset, 0.0f, 0.0f);
        m_label->getComponent<Transform>()->setSize(std::max(0.0f, size.x - textOffset - 8.0f), size.y);
    }
}

}  // namespace morrow
