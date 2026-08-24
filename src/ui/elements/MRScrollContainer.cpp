#include "MRScrollContainer.h"

#include <algorithm>

#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRScrollContainer> MRScrollContainer::create() {
    auto container = std::shared_ptr<MRScrollContainer>(new MRScrollContainer());
    container->addChild(container->m_scrollBar);
    auto weakSelf = std::weak_ptr<MRScrollContainer>(container);
    container->m_scrollBarConnection =
        container->m_scrollBar->events().onValueChanged.connect(
            [weakSelf](MRScrollBar&, float value) {
        if (auto self = weakSelf.lock()) {
            self->setScrollOffset(value * self->m_maxScrollOffset);
        }
    });
    container->addComponent<Interaction>();
    container->attachInput(container);
    return container;
}

MRScrollContainer::MRScrollContainer() : UIWidget(false) {
    setWidgetType("MRScrollContainer");
    m_scrollBar = MRScrollBar::create();
    m_scrollBar->setDisplayLayer(2);
    getComponent<Transform>()->addSizeChangeListener([this]() { m_metricsDirty = true; });
}

void MRScrollContainer::setContent(const std::shared_ptr<UIWidget>& content) {
    if (m_content)
        removeChild(m_content);
    m_content = content;
    if (!m_content)
        return;
    addChild(m_content);
    if (!m_content->getComponent<Interaction>()) {
        m_content->addComponent<Interaction>();
    }
    attachInputRecursive(m_content);
    m_metricsDirty = true;
}

void MRScrollContainer::addScrollChild(const std::shared_ptr<Widget>& child) {
    if (!m_content) {
        auto content = std::make_shared<UIWidget>(false);
        setContent(content);
    }
    m_content->addChild(child);
    attachInputRecursive(child);
    m_metricsDirty = true;
}

void MRScrollContainer::clearScrollChildren() {
    if (!m_content)
        return;
    const auto children = m_content->m_children;
    for (const auto& child : children)
        m_content->removeChild(child);
    m_inputConnections.clear();
    attachInput(shared_from_this());
    attachInputRecursive(m_content);
    m_scrollOffset = 0.0f;
    m_maxScrollOffset = 0.0f;
    m_metricsDirty = true;
}

void MRScrollContainer::setScrollStep(float step) {
    m_scrollStep = std::max(1.0f, step);
}

void MRScrollContainer::setScrollOffset(float offset) {
    if (m_metricsDirty)
        refreshMetrics();
    const float clamped = std::clamp(offset, 0.0f, m_maxScrollOffset);
    if (m_scrollOffset == clamped)
        return;
    m_scrollOffset = clamped;
    if (m_content) {
        m_content->getComponent<Transform>()->setPosition(0.0f, -m_scrollOffset, 0.0f);
    }
    m_scrollBar->setValue(m_maxScrollOffset > 0.0f ? m_scrollOffset / m_maxScrollOffset : 0.0f);
    updateChildVisibility();
    requestRender("scroll offset");
}

void MRScrollContainer::update(FrameStateSharedPtr frameState) {
    if (m_metricsDirty)
        refreshMetrics();
    updateChildVisibility();
    UIWidget::update(frameState);
}

void MRScrollContainer::attachInput(const std::shared_ptr<Widget>& widget) {
    if (!widget)
        return;
    auto interaction = widget->getComponent<Interaction>();
    if (!interaction)
        return;
    m_inputConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_WHEEL,
            [this](TouchEvent& event) { scrollBy(-event.wheelDeltaY * m_scrollStep); }));
    m_inputConnections.emplace_back(interaction->addEventListener(TOUCH_EVENT_TYPE_TOUCH, [this](TouchEvent& event) {
        m_dragging = true;
        m_lastDragY = event.positionY;
    }));
    m_inputConnections.emplace_back(interaction->addEventListener(TOUCH_EVENT_TYPE_MOVE, [this](TouchEvent& event) {
        if (!m_dragging)
            return;
        const float delta = m_lastDragY - event.positionY;
        m_lastDragY = event.positionY;
        scrollBy(delta);
    }));
    m_inputConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_RELEASE,
            [this](TouchEvent&) { m_dragging = false; }));
}

void MRScrollContainer::attachInputRecursive(const std::shared_ptr<Widget>& widget) {
    attachInput(widget);
    for (const auto& child : widget->m_children) {
        attachInputRecursive(child);
    }
}

void MRScrollContainer::scrollBy(float amount) {
    setScrollOffset(m_scrollOffset + amount);
}

void MRScrollContainer::refreshMetrics() {
    if (!m_content) {
        m_maxScrollOffset = 0.0f;
        m_scrollBar->setPageRatio(1.0f);
        m_metricsDirty = false;
        return;
    }

    const Vector3 viewportSize = getComponent<Transform>()->getSize();
    float contentHeight = m_content->getComponent<Transform>()->getSize().y;
    for (const auto& child : m_content->m_children) {
        if (!child)
            continue;
        const Vector3 position = child->getComponent<Transform>()->getPosition();
        const Vector3 size = child->getComponent<Transform>()->getSize();
        contentHeight = std::max(contentHeight, position.y + size.y);
    }
    const float contentWidth = std::max(0.0f, viewportSize.x - 24.0f);
    m_content->getComponent<Transform>()->setSize(contentWidth, contentHeight);
    m_maxScrollOffset = std::max(0.0f, contentHeight - viewportSize.y);
    m_scrollBar->setPageRatio(viewportSize.y / std::max(viewportSize.y, contentHeight));
    m_scrollBar->getComponent<Transform>()->setPosition(viewportSize.x - 16.0f, 0.0f, 2.0f);
    m_scrollBar->getComponent<Transform>()->setSize(12.0f, viewportSize.y);
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, m_maxScrollOffset);
    m_metricsDirty = false;
    setScrollOffset(m_scrollOffset);
}

void MRScrollContainer::updateChildVisibility() {
    if (!m_content)
        return;
    const float viewportHeight = getComponent<Transform>()->getSize().y;
    for (const auto& child : m_content->m_children) {
        if (!child)
            continue;
        const Vector3 position = child->getComponent<Transform>()->getPosition();
        const Vector3 size = child->getComponent<Transform>()->getSize();
        const float top = position.y - m_scrollOffset;
        const float bottom = top + size.y;
        child->setVisible(top >= 0.0f && bottom <= viewportHeight);
    }
}

}  // namespace morrow
