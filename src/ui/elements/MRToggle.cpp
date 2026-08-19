#include "MRToggle.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRToggle> MRToggle::create() {
    auto toggle = std::shared_ptr<MRToggle>(new MRToggle());
    toggle->initializeSwitch();
    return toggle;
}

MRToggle::MRToggle() {
    setWidgetType("MRToggle");
    setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutSwitch(); });
}

void MRToggle::updateVisualState() {
    MRSelectableButton::updateVisualState();
    if (m_track) {
        m_track->setColor(isChecked() ? Vector4(0.12f, 0.55f, 0.42f, 1.0f) : Vector4(0.62f, 0.66f, 0.72f, 1.0f));
    }
    if (m_thumb) {
        m_thumb->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
    layoutSwitch();
}

void MRToggle::initializeSwitch() {
    m_track = MRColor::create();
    m_track->setRounding(14.0f);
    m_thumb = MRColor::create();
    m_thumb->setRounding(11.0f);
    addChild(m_track);
    addChild(m_thumb);
    layoutSwitch();
    updateVisualState();
}

void MRToggle::layoutSwitch() {
    if (!m_track || !m_thumb)
        return;
    const Vector3 size = getComponent<Transform>()->getSize();
    constexpr float trackWidth = 52.0f;
    constexpr float trackHeight = 28.0f;
    constexpr float thumbSize = 22.0f;
    constexpr float rightPadding = 12.0f;
    constexpr float inset = 3.0f;
    const float trackX = std::max(0.0f, size.x - trackWidth - rightPadding);
    const float trackY = (size.y - trackHeight) * 0.5f;
    m_track->getComponent<Transform>()->setPosition(trackX, trackY, 0.0f);
    m_track->getComponent<Transform>()->setSize(trackWidth, trackHeight);
    m_thumb->getComponent<Transform>()->setPosition(trackX + (isChecked() ? trackWidth - thumbSize - inset : inset), trackY + inset, 0.1f);
    m_thumb->getComponent<Transform>()->setSize(thumbSize, thumbSize);
    if (m_label) {
        m_label->getComponent<Transform>()->setPosition(12.0f, 0.0f, 0.0f);
        m_label->getComponent<Transform>()->setSize(std::max(0.0f, trackX - 20.0f), size.y);
    }
}

}  // namespace morrow
