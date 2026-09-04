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
        m_indicator->setColor(m_indicatorColor);
    }
    if (m_indicatorFill) {
        m_indicatorFill->setColor(m_indicatorCheckedColor);
        m_indicatorFill->setVisible(isChecked());
    }
}

void MRCheckBox::setIndicatorColor(const Vector4& color) {
    m_indicatorColor = color;
    updateVisualState();
}

void MRCheckBox::setIndicatorColor(float r, float g, float b, float a) {
    setIndicatorColor(Vector4(r, g, b, a));
}

void MRCheckBox::setIndicatorCheckedColor(const Vector4& color) {
    m_indicatorCheckedColor = color;
    updateVisualState();
}

void MRCheckBox::setIndicatorCheckedColor(float r, float g, float b, float a) {
    setIndicatorCheckedColor(Vector4(r, g, b, a));
}

void MRCheckBox::initializeIndicator() {
    m_indicator = MRColor::create();
    m_indicatorFill = MRColor::create();
    m_indicator->addChild(m_indicatorFill);
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
    m_indicatorFill->getComponent<Transform>()->setSize(14.0f, 14.0f);
    m_indicatorFill->getComponent<Transform>()->setPosition(5.0f, 5.0f, 0.0f);
    if (m_label) {
        m_label->getComponent<Transform>()->setPosition(textOffset, 0.0f, 0.0f);
        m_label->getComponent<Transform>()->setSize(std::max(0.0f, size.x - textOffset - 8.0f), size.y);
    }
}

}  // namespace morrow
