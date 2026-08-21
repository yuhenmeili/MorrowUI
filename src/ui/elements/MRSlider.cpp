//
// Created by lance on 2026/8/17.
//

#include "MRSlider.h"

#include <algorithm>
#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"

namespace morrow {

MRSliderSharedPtr MRSlider::create() {
    std::shared_ptr<MRSlider> slider(new MRSlider());
    // addChild 内部会调用 shared_from_this()，必须在 shared_ptr 建立之后才能挂子节点
    slider->addChild(slider->m_thumb);
    return slider;
}

MRSlider::MRSlider() {
    setWidgetType("MRSlider");

    // 滑块子节点（复用 MRColor，支持颜色与圆角）；子节点在 create() 里才挂到本节点
    m_thumb = MRColor::create();
    m_thumb->getComponent<Transform>()->setSize(m_thumbSize.x, m_thumbSize.y);
    m_thumb->setColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    repositionThumb();

    // 尺寸变化时滑块位置跟随
    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this]() {
        repositionThumb();
    });

    // 交互：按下/拖拽改值（拖拽依赖平台层指针捕获，出界后 MOVE/RELEASE 仍路由回来）
    auto interaction = addComponent<Interaction>();
    interaction->setClickEnabled(false);
    interaction->setLongPressEnabled(false);
    interaction->setInteractionEnabled(m_interactive);

    m_touchConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_TOUCH, [this](TouchEvent& event) {
        m_dragging = true;
        updateValueFromPosition(event.positionX, event.positionY);
    });
    m_moveConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_MOVE, [this](TouchEvent& event) {
        if (m_dragging) {
            updateValueFromPosition(event.positionX, event.positionY);
        }
    });
    m_releaseConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [this](TouchEvent& /*event*/) {
        m_dragging = false;
    });
}

void MRSlider::setValue(float value) {
    setProgress(value);
}

float MRSlider::getValue() const {
    return getProgress();
}

void MRSlider::setThumbSize(const Vector2& size) {
    m_thumbSize = size;
    m_thumb->getComponent<Transform>()->setSize(size.x, size.y);
    repositionThumb();
}

void MRSlider::setThumbColor(const Vector4& color) {
    m_thumb->setColor(color);
}

void MRSlider::setThumbColor(float r, float g, float b, float a) {
    setThumbColor(Vector4(r, g, b, a));
}

void MRSlider::setThumbRounding(float rounding) {
    m_thumb->setRounding(rounding);
}

void MRSlider::setInteractive(bool interactive) {
    m_interactive = interactive;
    if (auto interaction = getComponent<Interaction>()) {
        interaction->setInteractionEnabled(interactive);
    }
}

MRSlider::Events& MRSlider::events() {
    return m_events;
}

void MRSlider::onProgressChanged(float progress) {
    repositionThumb();
    m_events.onValueChanged.notify(*this, progress);
}

void MRSlider::updateValueFromPosition(float screenX, float screenY) {
    const Math::Rect bounds = getScreenSpaceAABB();
    const float width = bounds.Max.x - bounds.Min.x;
    const float height = bounds.Max.y - bounds.Min.y;
    if (width <= 0.0f && height <= 0.0f) {
        return;
    }

    float value = 0.0f;
    switch (m_direction) {
        case ProgressDirection::LeftToRight:
            value = (screenX - bounds.Min.x) / width;
            break;
        case ProgressDirection::RightToLeft:
            value = (bounds.Max.x - screenX) / width;
            break;
        case ProgressDirection::TopToBottom:
            value = (screenY - bounds.Min.y) / height;
            break;
        case ProgressDirection::BottomToTop:
            value = (bounds.Max.y - screenY) / height;
            break;
    }
    setValue(std::clamp(value, 0.0f, 1.0f));
}

void MRSlider::repositionThumb() {
    auto transform = getComponent<Transform>();
    const Vector3 size = transform->getSize();

    // 子节点位置使用左上原点坐标（x 向右、y 向下），见 Transform::updateMatrix 的换算
    const float W = size.x;
    const float H = size.y;
    const float tw = m_thumbSize.x;
    const float th = m_thumbSize.y;

    float centerX = W * 0.5f;  // 垂直于填充方向的轴：居中
    float centerY = H * 0.5f;
    switch (m_direction) {
        case ProgressDirection::LeftToRight:
            centerX = std::clamp(W * m_progress, tw * 0.5f, std::max(tw * 0.5f, W - tw * 0.5f));
            break;
        case ProgressDirection::RightToLeft:
            centerX = std::clamp(W * (1.0f - m_progress), tw * 0.5f, std::max(tw * 0.5f, W - tw * 0.5f));
            break;
        case ProgressDirection::TopToBottom:
            centerY = std::clamp(H * m_progress, th * 0.5f, std::max(th * 0.5f, H - th * 0.5f));
            break;
        case ProgressDirection::BottomToTop:
            centerY = std::clamp(H * (1.0f - m_progress), th * 0.5f, std::max(th * 0.5f, H - th * 0.5f));
            break;
    }

    m_thumb->getComponent<Transform>()->setPosition(centerX - tw * 0.5f, centerY - th * 0.5f, 0.0f);
}

}  // namespace morrow
