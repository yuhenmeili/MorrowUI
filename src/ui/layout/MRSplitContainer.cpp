#include "MRSplitContainer.h"

#include <algorithm>

#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "platform/Window.h"

namespace morrow {

std::shared_ptr<MRSplitContainer> MRSplitContainer::create() {
    auto split = std::shared_ptr<MRSplitContainer>(new MRSplitContainer());
    split->initializeHandle();
    return split;
}

MRSplitContainer::MRSplitContainer() : UIWidget(false) {
    setWidgetType("MRSplitContainer");
    setClipChildren(true);
    getTransform()->addSizeChangeListener([this]() { layoutChildren(); });
}

void MRSplitContainer::initializeHandle() {
    m_handle = MRButton::create();
    m_handle->setText(L"", "default");
    m_handle->setCornerRadius(0.0f);
    m_handle->setBackgroundColor(Vector4(0.18f, 0.20f, 0.24f, 1.0f));
    m_handle->setHoverColor(Vector4(0.30f, 0.48f, 0.72f, 1.0f));
    m_handle->setPressedColor(Vector4(0.22f, 0.40f, 0.65f, 1.0f));
    m_handle->setDisplayLayer(4);

    auto interaction = m_handle->getComponent<Interaction>();
    m_handleConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_POINTER_ENTER,
            [this](TouchEvent&) { updateCursor(true); }));
    m_handleConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_POINTER_LEAVE,
            [this](TouchEvent&) {
                if (!m_dragging)
                    updateCursor(false);
            }));
    m_handleConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_TOUCH,
            [this](TouchEvent& event) {
                if (event.button != TOUCH_MOUSE_BUTTON_LEFT)
                    return;
                m_dragging = true;
                updateCursor(true);
                updateRatioFromPointer(event.positionX, event.positionY);
            }));
    m_handleConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_MOVE,
            [this](TouchEvent& event) {
                if (m_dragging)
                    updateRatioFromPointer(event.positionX, event.positionY);
            }));
    m_handleConnections.emplace_back(
        interaction->addEventListener(
            TOUCH_EVENT_TYPE_RELEASE,
            [this](TouchEvent&) {
                if (!m_dragging)
                    return;
                m_dragging = false;
                m_events.onDragFinished.notify(*this, m_splitRatio);
                updateCursor(false);
            }));
    addChild(m_handle);
}

void MRSplitContainer::setOrientation(SplitOrientation orientation) {
    if (m_orientation == orientation)
        return;
    m_orientation = orientation;
    layoutChildren();
}

SplitOrientation MRSplitContainer::getOrientation() const {
    return m_orientation;
}

void MRSplitContainer::setFirst(const std::shared_ptr<UIWidget>& widget) {
    if (m_first == widget)
        return;
    if (m_first)
        removeChild(m_first);
    m_first = widget;
    if (m_first)
        addChild(m_first);
    if (m_handle)
        m_handle->setDisplayLayer(4);
    layoutChildren();
}

void MRSplitContainer::setSecond(const std::shared_ptr<UIWidget>& widget) {
    if (m_second == widget)
        return;
    if (m_second)
        removeChild(m_second);
    m_second = widget;
    if (m_second)
        addChild(m_second);
    if (m_handle)
        m_handle->setDisplayLayer(4);
    layoutChildren();
}

std::shared_ptr<UIWidget> MRSplitContainer::getFirst() const {
    return m_first;
}

std::shared_ptr<UIWidget> MRSplitContainer::getSecond() const {
    return m_second;
}

void MRSplitContainer::setSplitRatio(float ratio) {
    const float clamped = clampRatio(ratio);
    if (m_splitRatio == clamped)
        return;
    m_splitRatio = clamped;
    layoutChildren();
    m_events.onSplitRatioChanged.notify(*this, m_splitRatio);
    requestRender("split ratio");
}

float MRSplitContainer::getSplitRatio() const {
    return m_splitRatio;
}

void MRSplitContainer::setFirstMinSize(float size) {
    m_firstMinSize = std::max(0.0f, size);
    setSplitRatio(m_splitRatio);
    layoutChildren();
}

void MRSplitContainer::setSecondMinSize(float size) {
    m_secondMinSize = std::max(0.0f, size);
    setSplitRatio(m_splitRatio);
    layoutChildren();
}

void MRSplitContainer::setHandleWidth(float width) {
    m_handleWidth = std::max(1.0f, width);
    setSplitRatio(m_splitRatio);
    layoutChildren();
}

float MRSplitContainer::getFirstMinSize() const {
    return m_firstMinSize;
}

float MRSplitContainer::getSecondMinSize() const {
    return m_secondMinSize;
}

float MRSplitContainer::getHandleWidth() const {
    return m_handleWidth;
}

MRSplitContainer::Events& MRSplitContainer::events() {
    return m_events;
}

void MRSplitContainer::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    UIWidget::update(frameState);
}

float MRSplitContainer::clampRatio(float ratio) const {
    const Vector3 size = getTransform()->getSize();
    const float total =
        (m_orientation == SplitOrientation::Horizontal ? size.x : size.y) -
        m_handleWidth;
    if (total <= 0.0f)
        return std::clamp(ratio, 0.0f, 1.0f);

    const float minimumRatio = std::min(1.0f, m_firstMinSize / total);
    const float maximumRatio = std::max(0.0f, 1.0f - m_secondMinSize / total);
    if (minimumRatio > maximumRatio)
        return std::clamp(ratio, 0.0f, 1.0f);
    return std::clamp(ratio, minimumRatio, maximumRatio);
}

void MRSplitContainer::layoutChildren() {
    if (!m_handle)
        return;

    const Vector3 size = getTransform()->getSize();
    m_splitRatio = clampRatio(m_splitRatio);
    if (m_orientation == SplitOrientation::Horizontal) {
        const float available = std::max(0.0f, size.x - m_handleWidth);
        const float firstWidth = available * m_splitRatio;
        const float secondWidth = available - firstWidth;
        if (m_first) {
            m_first->getTransform()->setPosition(0.0f, 0.0f, 0.0f);
            m_first->getTransform()->setSize(firstWidth, size.y);
        }
        m_handle->getTransform()->setPosition(firstWidth, 0.0f, 4.0f);
        m_handle->getTransform()->setSize(m_handleWidth, size.y);
        if (m_second) {
            m_second->getTransform()->setPosition(firstWidth + m_handleWidth, 0.0f, 0.0f);
            m_second->getTransform()->setSize(secondWidth, size.y);
        }
    } else {
        const float available = std::max(0.0f, size.y - m_handleWidth);
        const float firstHeight = available * m_splitRatio;
        const float secondHeight = available - firstHeight;
        if (m_first) {
            m_first->getTransform()->setPosition(0.0f, 0.0f, 0.0f);
            m_first->getTransform()->setSize(size.x, firstHeight);
        }
        m_handle->getTransform()->setPosition(0.0f, firstHeight, 4.0f);
        m_handle->getTransform()->setSize(size.x, m_handleWidth);
        if (m_second) {
            m_second->getTransform()->setPosition(0.0f, firstHeight + m_handleWidth, 0.0f);
            m_second->getTransform()->setSize(size.x, secondHeight);
        }
    }
}

void MRSplitContainer::updateRatioFromPointer(float x, float y) {
    const Math::Rect bounds = getScreenSpaceAABB();
    const float total =
        (m_orientation == SplitOrientation::Horizontal ? bounds.GetWidth() : bounds.GetHeight()) -
        m_handleWidth;
    if (total <= 0.0f)
        return;
    const float pointer =
        m_orientation == SplitOrientation::Horizontal ? x - bounds.Min.x : y - bounds.Min.y;
    setSplitRatio((pointer - m_handleWidth * 0.5f) / total);
}

void MRSplitContainer::updateCursor(bool active) {
    std::shared_ptr<Widget> current = m_parent;
    while (current) {
        if (auto window = std::dynamic_pointer_cast<Window>(current)) {
            window->setCursorShape(
                active
                    ? (m_orientation == SplitOrientation::Horizontal
                           ? CursorShape::ResizeHorizontal
                           : CursorShape::ResizeVertical)
                    : CursorShape::Arrow);
            return;
        }
        current = current->m_parent;
    }
}

}  // namespace morrow
