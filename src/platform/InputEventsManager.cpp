//
// Created by lance on 2023/1/18.
//

#include "InputEventsManager.h"
#include "core/ToolUtils.h"
#include "base/TouchEvent.h"

namespace morrow
{
void InputEventsManager::setInputProvider(IInputProviderSharedPtr provider)
{
    m_inputProvider = provider;
}

void InputEventsManager::poll()
{
    m_inputEvents.clear();
    if (m_inputProvider) {
        m_inputProvider->poll(m_inputEvents);
    }
}

Observable<std::vector<TouchEvent>&>& InputEventsManager::getInputEventsDispatcher()
{
    return m_inputEventsDispatcher;
}

int32_t InputEventsManager::GetRectTouchDownID(const Rect& bb)
{
    for (const auto& touchEvent : m_inputEvents) {
        if (bb.Contains(touchEvent.positionX, touchEvent.positionY) && TouchEventType::TOUCH_EVENT_TYPE_TOUCH == touchEvent.eventType) {
            return touchEvent.touchID;
        }
    }
    return -1;
}

bool InputEventsManager::IsRectTouchRelease(int32_t touchID)
{
    for (const auto& touchEvent : m_inputEvents) {
        if (touchEvent.touchID == touchID) {
            return TouchEventType::TOUCH_EVENT_TYPE_RELEASE == touchEvent.eventType;
        }
    }
    return false;
}

int32_t InputEventsManager::GetTouchIDCountByRect(const Rect& bb)
{
    int32_t result = 0;
    for (const auto& touchEvent : m_inputEvents) {
        if (touchEvent.touchID >= 0 && bb.Contains(touchEvent.positionX, touchEvent.positionY)) {
            ++result;
        }
    }
    return result;
}

bool InputEventsManager::isTouchIDInRect(int32_t touchID, const Rect& bb)
{
    for (const auto& touchEvent : m_inputEvents) {
        if (touchID >= 0 && touchEvent.touchID == touchID && bb.Contains(touchEvent.positionX, touchEvent.positionY)) {
            return true;
        }
    }
    return false;
}

bool InputEventsManager::hasTouchID(int32_t touchID)
{
    for (const auto& touchEvent : m_inputEvents) {
        if (touchID >= 0 && touchEvent.touchID == touchID) {
            return true;
        }
    }
    return false;
}

int32_t InputEventsManager::GetTouchIDCount()
{
    std::vector<int32_t> RECT_TOUCHIDS;
    for (auto& touchEvent : m_inputEvents) {
        if (touchEvent.touchID < 0) {
            continue;
        }
        //由于touch事件是7-8ms上报一次，所以每帧move事件会有两个或者以上相同的touchID，需要过滤一下
        if (std::find(RECT_TOUCHIDS.begin(), RECT_TOUCHIDS.end(), touchEvent.touchID) == RECT_TOUCHIDS.end()) {
            RECT_TOUCHIDS.emplace_back(touchEvent.touchID);
        }
    }
    return RECT_TOUCHIDS.size();
}

std::vector<TouchEvent>& InputEventsManager::getInputEvents()
{
    return m_inputEvents;
}
}
