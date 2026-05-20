//
// QNX 平台输入提供者：桩实现。可在此处接入 screen_get_event 等逻辑。
//

#include "QNXInputProvider.h"

#include <algorithm>

#include "../../math/MathUtils.h"
#include "../../utils/Log.h"

namespace morrow {
namespace {
constexpr int32_t kScreenEventWaitTimeNs = 1000000;

bool isUnhandledEventType(int32_t eventType) {
    return eventType != SCREEN_EVENT_NONE &&
        eventType != SCREEN_EVENT_PROPERTY &&
        eventType != SCREEN_EVENT_MTOUCH_MOVE &&
        eventType != SCREEN_EVENT_MTOUCH_TOUCH &&
        eventType != SCREEN_EVENT_MTOUCH_RELEASE &&
        eventType != SCREEN_EVENT_POINTER &&
        eventType != SCREEN_EVENT_KEYBOARD;
}

int32_t countUniqueTouchIDs(const std::vector<TouchEvent>& events) {
    std::vector<int32_t> touchIDs;
    for (const auto& touchEvent : events) {
        if (touchEvent.touchID < 0) {
            continue;
        }
        if (std::find(touchIDs.begin(), touchIDs.end(), touchEvent.touchID) == touchIDs.end()) {
            touchIDs.emplace_back(touchEvent.touchID);
        }
    }
    return static_cast<int32_t>(touchIDs.size());
}
} // namespace

void QNXInputProvider::setScreenContext(screen_context_t screenContext, screen_event_t screenEvent) {
    m_screenContext = screenContext;
    m_screenEvent = screenEvent;
}

void QNXInputProvider::setWindowOffset(int32_t windowX, int32_t windowY) {
    m_windowX = windowX;
    m_windowY = windowY;
}

void QNXInputProvider::setSkipEvents(bool skipEvents) {
    m_skipEvents = skipEvents;
}

bool QNXInputProvider::hasTouchID(const std::vector<TouchEvent>& events, int32_t touchID) const {
    for (const auto& touchEvent : events) {
        if (touchID >= 0 && touchEvent.touchID == touchID) {
            return true;
        }
    }
    return false;
}

void QNXInputProvider::poll(std::vector<TouchEvent>& outEvents) {
    outEvents.clear();
    if (!m_screenContext || !m_screenEvent) {
        return;
    }

    int32_t eventType = 0;
    int32_t touchID = 0;
    int32_t touchPos[2]{0, 0};
    const double touchTime = Math::getCurrentMonotonicTime();

    while (!screen_get_event(m_screenContext, m_screenEvent, kScreenEventWaitTimeNs)) {
        if (screen_get_event_property_iv(m_screenEvent, SCREEN_PROPERTY_TYPE, &eventType) != 0) {
            LOG_I("screen_get_event_property_iv(SCREEN_PROPERTY_TYPE) failed");
            break;
        }

        if (eventType == SCREEN_EVENT_NONE || eventType == SCREEN_EVENT_PROPERTY) {
            break;
        }

        if (isUnhandledEventType(eventType)) {
            LOG_I("screen event type={} not handled by QNXInputProvider", eventType);
        }

        TouchEventType mappedType = TOUCH_EVENT_TYPE_NONE;
        switch (eventType) {
            case SCREEN_EVENT_MTOUCH_MOVE:
                mappedType = TOUCH_EVENT_TYPE_MOVE;
                break;
            case SCREEN_EVENT_MTOUCH_TOUCH:
                mappedType = TOUCH_EVENT_TYPE_TOUCH;
                break;
            case SCREEN_EVENT_MTOUCH_RELEASE:
                mappedType = TOUCH_EVENT_TYPE_RELEASE;
                break;
            default:
                continue;
        }

        if (screen_get_event_property_iv(m_screenEvent, SCREEN_PROPERTY_TOUCH_ID, &touchID) != 0) {
            LOG_I("screen_get_event_property_iv(SCREEN_PROPERTY_TOUCH_ID) failed");
            continue;
        }
        if (screen_get_event_property_iv(m_screenEvent, SCREEN_PROPERTY_POSITION, touchPos) != 0) {
            LOG_I("screen_get_event_property_iv(SCREEN_PROPERTY_POSITION) failed");
            continue;
        }

        TouchEvent touchEvent;
        touchEvent.touchID = touchID;
        touchEvent.positionX = static_cast<float>(touchPos[0] - m_windowX);
        touchEvent.positionY = static_cast<float>(touchPos[1] - m_windowY);
        touchEvent.touchTime = touchTime;
        touchEvent.eventType = mappedType;
        touchEvent.deviceType = TOUCH_DEVICE_TYPE_TOUCH;
        outEvents.emplace_back(touchEvent);

        if (mappedType == TOUCH_EVENT_TYPE_TOUCH) {
            LOG_I("SCREEN_EVENT_MTOUCH_TOUCH id={}, pos=({}, {})", touchID, touchEvent.positionX, touchEvent.positionY);
        } else if (mappedType == TOUCH_EVENT_TYPE_RELEASE) {
            LOG_I("SCREEN_EVENT_MTOUCH_RELEASE id={}, pos=({}, {})", touchID, touchEvent.positionX, touchEvent.positionY);
        }
    }

    if (m_skipEvents) {
        outEvents.clear();
        m_lastFrameInputEvents.clear();
        LOG_I("skip events for thread wait");
        return;
    }

    for (const auto& lastTouchEvent : m_lastFrameInputEvents) {
        if ((lastTouchEvent.eventType == TOUCH_EVENT_TYPE_TOUCH || lastTouchEvent.eventType == TOUCH_EVENT_TYPE_MOVE) &&
            !hasTouchID(outEvents, lastTouchEvent.touchID)) {
            TouchEvent touchEvent;
            touchEvent.touchID = lastTouchEvent.touchID;
            touchEvent.positionX = lastTouchEvent.positionX;
            touchEvent.positionY = lastTouchEvent.positionY;
            touchEvent.touchTime = touchTime;
            touchEvent.eventType = TOUCH_EVENT_TYPE_RELEASE;
            touchEvent.deviceType = TOUCH_DEVICE_TYPE_TOUCH;
            outEvents.emplace_back(touchEvent);
        }
    }

    m_lastFrameInputEvents.clear();
    if (outEvents.empty()) {
        return;
    }

    const int32_t totalTouchIDCount = countUniqueTouchIDs(outEvents);
    for (auto& touchEvent : outEvents) {
        touchEvent.totalTouchIDCount = totalTouchIDCount;
        m_lastFrameInputEvents.emplace_back(touchEvent);
    }
}

// void InputEventsManager::qnxInputEventsHandle(screen_context_t screen_ctx, screen_event_t screen_ev, int32_t windowX, int32_t windowY, bool skipEvent)
// {
//     int32_t ev_type = 0;
//     int32_t touch_id = 0;
//     int32_t touch_pos[2]{0, 0};
//     m_inputEvents.clear();
//     double touchTime = Math::getCurrentMonotonicTime();
//
//     while (!screen_get_event(screen_ctx, screen_ev, 1000000)) {
//         screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TYPE, &ev_type);
//
//         /// No events in the queue
//         if (ev_type == SCREEN_EVENT_NONE || ev_type == SCREEN_EVENT_PROPERTY) {
//             break;
//         }
//
//         if (ev_type != SCREEN_EVENT_NONE &&
//             ev_type != SCREEN_EVENT_MTOUCH_MOVE &&
//             ev_type != SCREEN_EVENT_MTOUCH_TOUCH &&
//             ev_type != SCREEN_EVENT_MTOUCH_RELEASE &&
//             ev_type != SCREEN_EVENT_POINTER &&
//             ev_type != SCREEN_EVENT_KEYBOARD) {
//             LOG_I("screen_get_event_property_iv, SCREEN_PROPERTY_TYPE, %d", ev_type);
//             printf("MORROWGUI | screen_get_event_property_iv, SCREEN_PROPERTY_TYPE, %d \n", ev_type);
//         }
//
//         switch (ev_type) {
//             /// MTOUCH Events
//             case SCREEN_EVENT_MTOUCH_MOVE: {
//                 screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TOUCH_ID, &touch_id);
//                 screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_POSITION, touch_pos);
//
//                 TouchEvent touch_event;
//                 touch_event.touchID = touch_id;
//                 touch_event.positionX = float(touch_pos[0] - windowX);
//                 touch_event.positionY = float(touch_pos[1] - windowY);
//                 touch_event.touchTime = touchTime;
//                 touch_event.eventType = TouchEventType::TOUCH_EVENT_TYPE_MOVE;
//                 m_inputEvents.emplace_back(touch_event);
//
// //                LOG_I("SCREEN_EVENT_MTOUCH_MOVE %d, pos: %f, %f", touch_id, touch_event.positionX, touch_event.positionY);
//                 break;
//             }
//
//             case SCREEN_EVENT_MTOUCH_TOUCH: {
//                 screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TOUCH_ID, &touch_id);
//                 screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_POSITION, touch_pos);
//
//                 TouchEvent touch_event;
//                 touch_event.touchID = touch_id;
//                 touch_event.positionX = float(touch_pos[0] - windowX);
//                 touch_event.positionY = float(touch_pos[1] - windowY);
//                 touch_event.touchTime = touchTime;
//                 touch_event.eventType = TouchEventType::TOUCH_EVENT_TYPE_TOUCH;
//                 m_inputEvents.emplace_back(touch_event);
//
//                 LOG_I("SCREEN_EVENT_MTOUCH_TOUCH %d, pos: %f, %f", touch_id, touch_event.positionX, touch_event.positionY);
//                 break;
//             }
//
//             case SCREEN_EVENT_MTOUCH_RELEASE: {
//                 screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_TOUCH_ID, &touch_id);
//                 screen_get_event_property_iv(screen_ev, SCREEN_PROPERTY_POSITION, touch_pos);
//
//                 TouchEvent touch_event;
//                 touch_event.touchID = touch_id;
//                 touch_event.positionX = float(touch_pos[0] - windowX);
//                 touch_event.positionY = float(touch_pos[1] - windowY);
//                 touch_event.touchTime = touchTime;
//                 touch_event.eventType = TouchEventType::TOUCH_EVENT_TYPE_RELEASE;
//                 m_inputEvents.emplace_back(touch_event);
//
//                 LOG_I("SCREEN_EVENT_MTOUCH_RELEASE %d, pos: %f, %f", touch_id, touch_event.positionX, touch_event.positionY);
//                 break;
//             }
//             default: {
//                 break;
//             }
//         }
//     }
//     if (skipEvent) {
//         m_inputEvents.clear();
//         m_lastFrameInputEvents.clear();
//         LOG_I("skip events for thread wait");
//         return;
//     }
//     //防止release事件不触发，安全冗余
//     for (const auto& lastTouchEvent : m_lastFrameInputEvents) {
//         if (lastTouchEvent.eventType == TouchEventType::TOUCH_EVENT_TYPE_TOUCH || lastTouchEvent.eventType == TouchEventType::TOUCH_EVENT_TYPE_MOVE) {
//             if (!hasTouchID(lastTouchEvent.touchID)) {
//                 TouchEvent touch_event;
//                 touch_event.touchID = lastTouchEvent.touchID;
//                 touch_event.positionX = lastTouchEvent.positionX;
//                 touch_event.positionY = lastTouchEvent.positionY;
//                 touch_event.touchTime = touchTime;
//                 touch_event.eventType = TouchEventType::TOUCH_EVENT_TYPE_RELEASE;
//                 m_inputEvents.emplace_back(touch_event);
//             }
//         }
//     }
//     m_lastFrameInputEvents.clear();
//     //dispatch event
//     if (!m_inputEvents.empty()) {
//         int32_t totalTouchIDCount = GetTouchIDCount();
//         for (auto& touchEvent : m_inputEvents) {
//             touchEvent.totalTouchIDCount = totalTouchIDCount;
//             m_lastFrameInputEvents.emplace_back(touchEvent);
//         }
//         m_inputEventsDispatcher.notify(m_inputEvents);
//     }
// }

} // namespace morrow
