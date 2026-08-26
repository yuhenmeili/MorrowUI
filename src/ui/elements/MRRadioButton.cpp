#include "MRRadioButton.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRRadioGroup> MRRadioGroup::create() {
    return std::make_shared<MRRadioGroup>();
}

MRRadioGroup::MRRadioGroup() : UIWidget(false) {
    setWidgetType("MRRadioGroup");
}

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

void MRRadioGroup::addChild(std::shared_ptr<Widget> widget) {
    UIWidget::addChild(widget);
    if (auto button = std::dynamic_pointer_cast<MRRadioButton>(widget))
        button->setGroup(std::static_pointer_cast<MRRadioGroup>(shared_from_this()));
}

bool MRRadioGroup::removeChild(std::shared_ptr<Widget> widget) {
    const bool removed = UIWidget::removeChild(widget);
    if (removed) {
        if (auto button = std::dynamic_pointer_cast<MRRadioButton>(widget))
            button->setGroup(nullptr);
    }
    return removed;
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
    const auto currentGroup = m_group.lock();
    if (currentGroup == group)
        return;
    if (currentGroup)
        currentGroup->remove(std::static_pointer_cast<MRRadioButton>(shared_from_this()));
    m_group = group;
    if (group) {
        auto self = std::static_pointer_cast<MRRadioButton>(shared_from_this());
        group->add(self);
        if (isChecked())
            group->select(self);
    }
}

void MRRadioButton::onCheckedChanged(bool checked) {
    if (const auto group = m_group.lock(); checked && group) {
        group->select(std::static_pointer_cast<MRRadioButton>(shared_from_this()));
    }
    MRSelectableButton::onCheckedChanged(checked);
}

void MRRadioButton::updateVisualState() {
    MRSelectableButton::updateVisualState();
    if (m_indicator) {
        m_indicator->setColor(Vector4(0.68f, 0.72f, 0.78f, 1.0f));
    }
    if (m_indicatorFill) {
        m_indicatorFill->setColor(Vector4(0.08f, 0.62f, 0.42f, 1.0f));
        m_indicatorFill->setVisible(isChecked());
    }
}

void MRRadioButton::initializeIndicator() {
    m_indicator = MRColor::create();
    m_indicator->setRounding(12.0f);
    m_indicatorFill = MRColor::create();
    m_indicatorFill->setRounding(6.0f);
    m_indicator->addChild(m_indicatorFill);
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
    m_indicatorFill->getComponent<Transform>()->setSize(12.0f, 12.0f);
    m_indicatorFill->getComponent<Transform>()->setPosition(6.0f, 6.0f, 0.0f);
    if (m_label) {
        m_label->getComponent<Transform>()->setPosition(textOffset, 0.0f, 0.0f);
        m_label->getComponent<Transform>()->setSize(std::max(0.0f, size.x - textOffset - 8.0f), size.y);
    }
}

}  // namespace morrow
