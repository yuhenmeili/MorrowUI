#include "MRScrollBar.h"

#include <algorithm>

#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRScrollBar> MRScrollBar::create() {
    auto bar = std::shared_ptr<MRScrollBar>(new MRScrollBar());
    bar->addChild(bar->m_track);
    bar->addChild(bar->m_thumb);
    bar->getComponent<Transform>()->addSizeChangeListener([weakBar = std::weak_ptr<MRScrollBar>(bar)]() {
        if (auto self = weakBar.lock())
            self->updateThumb();
    });
    bar->updateThumb();
    return bar;
}

MRScrollBar::MRScrollBar() : UIWidget(false) {
    setWidgetType("MRScrollBar");
    m_track = MRColor::create();
    m_track->setColor(0.84f, 0.87f, 0.91f, 1.0f);
    m_track->setRounding(6.0f);
    m_thumb = MRColor::create();
    m_thumb->setColor(0.35f, 0.48f, 0.62f, 1.0f);
    m_thumb->setRounding(6.0f);

    auto interaction = addComponent<Interaction>();
    interaction->setClickEnabled(false);
    interaction->addEventListener(TOUCH_EVENT_TYPE_TOUCH, [this](TouchEvent& event) {
        const auto bounds = m_thumb->getScreenSpaceAABB();
        if (bounds.Contains(event.positionX, event.positionY)) {
            m_dragging = true;
            m_dragCenterOffset = event.positionY - (bounds.Min.y + bounds.Max.y) * 0.5f;
        } else {
            updateValueFromThumbCenter(event.positionY);
        }
    });
    interaction->addEventListener(TOUCH_EVENT_TYPE_MOVE, [this](TouchEvent& event) {
        if (m_dragging) {
            updateValueFromThumbCenter(event.positionY - m_dragCenterOffset);
        }
    });
    interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [this](TouchEvent&) { m_dragging = false; });
}

void MRScrollBar::setValue(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (m_value == clamped)
        return;
    m_value = clamped;
    updateThumb();
    if (m_onValueChanged)
        m_onValueChanged(m_value);
}

void MRScrollBar::setPageRatio(float ratio) {
    m_pageRatio = std::clamp(ratio, 0.05f, 1.0f);
    updateThumb();
}

void MRScrollBar::setTrackColor(const Vector4& color) {
    m_track->setColor(color);
}

void MRScrollBar::setThumbColor(const Vector4& color) {
    m_thumb->setColor(color);
}

void MRScrollBar::setOnValueChangedCallback(ValueChangedCallback callback) {
    m_onValueChanged = std::move(callback);
}

void MRScrollBar::updateThumb() {
    const Vector3 size = getComponent<Transform>()->getSize();
    constexpr float padding = 2.0f;
    const float trackHeight = std::max(0.0f, size.y - padding * 2.0f);
    const float thumbHeight = std::max(24.0f, trackHeight * m_pageRatio);
    const float travel = std::max(0.0f, trackHeight - thumbHeight);

    m_track->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    m_track->getComponent<Transform>()->setSize(size.x, size.y);
    m_thumb->getComponent<Transform>()->setPosition(0.0f, padding + travel * m_value, 0.1f);
    m_thumb->getComponent<Transform>()->setSize(size.x, thumbHeight);
}

void MRScrollBar::updateValueFromThumbCenter(float screenY) {
    const auto bounds = getScreenSpaceAABB();
    const float height = bounds.Max.y - bounds.Min.y;
    const float trackHeight = std::max(0.0f, height - 4.0f);
    const float thumbHeight = std::max(24.0f, trackHeight * m_pageRatio);
    const float travel = std::max(1.0f, trackHeight - thumbHeight);
    const float position = std::clamp(screenY - bounds.Min.y - 2.0f - thumbHeight * 0.5f, 0.0f, travel);
    setValue(position / travel);
}

}  // namespace morrow
