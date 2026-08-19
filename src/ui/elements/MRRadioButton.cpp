#include "MRRadioButton.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

void MRRadioGroup::add(const std::shared_ptr<MRRadioButton>& button) {
    if (!button)
        return;
    m_buttons.erase(std::remove_if(m_buttons.begin(), m_buttons.end(), [](const auto& item) { return item.expired(); }), m_buttons.end());
    for (const auto& item : m_buttons) {
        if (item.lock() == button)
            return;
    }
    m_buttons.emplace_back(button);
}

void MRRadioGroup::remove(const std::shared_ptr<MRRadioButton>& button) {
    m_buttons.erase(std::remove_if(m_buttons.begin(), m_buttons.end(), [&button](const auto& item) { return item.expired() || item.lock() == button; }), m_buttons.end());
}

void MRRadioGroup::select(const std::shared_ptr<MRRadioButton>& button) {
    add(button);
    for (const auto& item : m_buttons) {
        if (auto candidate = item.lock()) {
            candidate->setChecked(candidate == button);
        }
    }
}

std::shared_ptr<MRRadioButton> MRRadioGroup::getSelected() const {
    for (const auto& item : m_buttons) {
        if (auto button = item.lock(); button && button->isChecked())
            return button;
    }
    return nullptr;
}

std::shared_ptr<MRRadioButton> MRRadioButton::create() {
    auto button = std::shared_ptr<MRRadioButton>(new MRRadioButton());
    button->initializeIndicator();
    return button;
}

MRRadioButton::MRRadioButton() {
    setWidgetType("MRRadioButton");
    setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutIndicator(); });
}

void MRRadioButton::setGroup(const MRRadioGroupSharedPtr& group) {
    if (m_group == group)
        return;
    if (m_group)
        m_group->remove(std::static_pointer_cast<MRRadioButton>(shared_from_this()));
    m_group = group;
    if (m_group) {
        auto self = std::static_pointer_cast<MRRadioButton>(shared_from_this());
        m_group->add(self);
        if (isChecked())
            m_group->select(self);
    }
}

void MRRadioButton::onCheckedChanged(bool checked) {
    if (checked && m_group) {
        m_group->select(std::static_pointer_cast<MRRadioButton>(shared_from_this()));
    }
    MRSelectableButton::onCheckedChanged(checked);
}

void MRRadioButton::updateVisualState() {
    MRSelectableButton::updateVisualState();
    if (m_indicator) {
        m_indicator->setColor(isChecked() ? Vector4(0.08f, 0.42f, 0.32f, 1.0f) : Vector4(0.68f, 0.72f, 0.78f, 1.0f));
    }
}

void MRRadioButton::initializeIndicator() {
    m_indicator = MRColor::create();
    m_indicator->setRounding(12.0f);
    addChild(m_indicator);
    layoutIndicator();
    updateVisualState();
}

void MRRadioButton::layoutIndicator() {
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
