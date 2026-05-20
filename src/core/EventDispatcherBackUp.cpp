// //
// // Created by lance on 2023/7/18.
// //
//
// #include "EventDispatcherBackUp.h"
// #include "MathUtils.h"
// #include "../utils/Log.h"
// #include "FrameState.h"
// #include "InputEventsManager.h"
//
// namespace morrow
// {
// EventDispatcherBackUp::~EventDispatcherBackUp()
// {
//     m_eventListeners.clear();
// }
//
// void EventDispatcherBackUp::addEventListener(TouchEventType eventType, TouchEventCallbackSharedPtr callbackSharedPtr, bool useCapture)
// {
//     auto callbackList = m_eventListeners.find(eventType) != m_eventListeners.end() ? m_eventListeners[eventType] : std::vector<TouchEventCallbackSharedPtr>();
//     auto iter = std::find(callbackList.begin(), callbackList.end(), callbackSharedPtr);
//     if (iter == callbackList.end()) {
//         callbackList.emplace_back(callbackSharedPtr);
//     }
//     m_eventListeners[eventType] = callbackList;
// }
//
// void EventDispatcherBackUp::removeEventListener(TouchEventType eventType, TouchEventCallbackSharedPtr callbackSharedPtr)
// {
//     if (m_eventListeners.find(eventType) != m_eventListeners.end()) {
//         auto callbackList = m_eventListeners[eventType];
//         auto iter = std::find(callbackList.begin(), callbackList.end(), callbackSharedPtr);
//         if (iter != callbackList.end()) {
//             callbackList.erase(iter);
//             m_eventListeners[eventType] = callbackList;
//         }
//     }
// }
//
// bool EventDispatcherBackUp::hasEventListener(TouchEventType eventType)
// {
//     bool result = false;
//     if (m_eventListeners.find(eventType) != m_eventListeners.end()) {
//         auto callbackList = m_eventListeners[eventType];
//         result = !callbackList.empty();
//     }
//     if (eventType == TOUCH_EVENT_TYPE_TOUCH) {
//         result = result || hasEventListener(TOUCH_EVENT_TYPE_TOUCH_AND_HOLD);
//     }
//     return result;
// }
//
// void EventDispatcherBackUp::dispatchEvent(TouchEvent& event)
// {
//     if (m_eventListeners.find(event.eventType) != m_eventListeners.end()) {
//         auto callbackList = m_eventListeners[event.eventType];
//         for (auto& callbackStruct : callbackList) {
//             if (callbackStruct->callback) {
//                 if (event.eventType == TOUCH_EVENT_TYPE_TOUCH_AND_HOLD) {
//                     if (event.callbackTimes <= 0) {
//                         if (Math::getCurrentMonotonicTime() - event.touchTime >= callbackStruct->delay) {
//                             callbackStruct->callback(event);
//                             event.callbackTimes++;
//                         }
//                     }
//                 } else {
//                     callbackStruct->callback(event);
//                 }
//             }
//         }
//     }
// }
//
// void EventDispatcherBackUp::touchHander(TouchEvent& event)
// {
//     if (!isInTouchCooldown(event)) {
//         dispatchEvent(event);
//
//         TouchEvent holdEvent;
//         holdEvent.eventType = TOUCH_EVENT_TYPE_TOUCH_AND_HOLD;
//         holdEvent.touchID = event.touchID;
//         holdEvent.positionX = event.positionX;
//         holdEvent.positionY = event.positionY;
//         holdEvent.touchTime = event.touchTime;
//         m_holdEvents.emplace_back(holdEvent);
//         m_touchTime = event.touchTime;
//     }
// }
//
// void EventDispatcherBackUp::moveHandler(FrameStateSharedPtr frameState)
// {
//     auto inputEvents = frameState->inputEventsManager->getInputEvents();
//     for (auto& holdEvent : m_holdEvents) {
//         for (auto& inputEvent : inputEvents) {
//             if (inputEvent.touchID == holdEvent.touchID && inputEvent.eventType == TOUCH_EVENT_TYPE_MOVE) {
//                 holdEvent.positionX = inputEvent.positionX;
//                 holdEvent.positionY = inputEvent.positionY;
//                 dispatchEvent(inputEvent);
//             }
//         }
//     }
// }
//
// void EventDispatcherBackUp::touchAndHoldHandler(FrameStateSharedPtr frameState, Rect bb)
// {
//     for (auto& holdEvent : m_holdEvents) {
//         if (frameState->inputEventsManager->isTouchIDInRect(holdEvent.touchID, bb)) {
//             dispatchEvent(holdEvent);
//         }
//     }
// }
//
// void EventDispatcherBackUp::clickHandler(FrameStateSharedPtr frameState, Rect bb)
// {
//     auto inputEvents = frameState->inputEventsManager->getInputEvents();
//     for (auto& holdEvent : m_holdEvents) {
//         for (auto& inputEvent : inputEvents) {
//             if (inputEvent.touchID == holdEvent.touchID && inputEvent.eventType == TOUCH_EVENT_TYPE_RELEASE) {
//                 if (frameState->inputEventsManager->isTouchIDInRect(inputEvent.touchID, bb)) {
//                     TouchEvent clickEvent = inputEvent;
//                     clickEvent.eventType = TOUCH_EVENT_TYPE_CLICK;
//                     dispatchEvent(clickEvent);
//                 }
//             }
//         }
//     }
// }
//
// void EventDispatcherBackUp::releaseHandler(FrameStateSharedPtr frameState)
// {
//     auto inputEvents = frameState->inputEventsManager->getInputEvents();
//     for (auto iter = m_holdEvents.begin(); iter != m_holdEvents.end();) {
//         bool removed = false;
//         for (auto& inputEvent : inputEvents) {
//             if (inputEvent.touchID == iter->touchID && inputEvent.eventType == TOUCH_EVENT_TYPE_RELEASE) {
//                 iter = m_holdEvents.erase(iter);
//                 dispatchEvent(inputEvent);
//                 m_releaseTime = Math::getCurrentMonotonicTime();
//                 removed = true;
//                 break;
//             }
//         }
//         if (!removed) {
//             ++iter;
//         }
//     }
// }
//
// void EventDispatcherBackUp::reset()
// {
//     m_holdEvents.clear();
// }
//
// bool EventDispatcherBackUp::isTraversalTouchEnabled() const
// {
//     return m_traversalTouchEnabled;
// }
//
// void EventDispatcherBackUp::setTraversalTouchEnabled(bool traversalTouchEnabled)
// {
//     m_traversalTouchEnabled = traversalTouchEnabled;
// }
//
// bool EventDispatcherBackUp::hasTouched() const
// {
//     return !m_holdEvents.empty();
// }
//
// void EventDispatcherBackUp::setTouchCooldown(double cd)
// {
//     m_touchCooldown = cd;
// }
//
// bool EventDispatcherBackUp::isInTouchCooldown(TouchEvent& event) const
// {
//     if (event.eventType == TOUCH_EVENT_TYPE_TOUCH) {
//         return event.touchTime - m_touchTime < m_touchCooldown;
//     } else if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
//         return event.touchTime - m_releaseTime < m_touchCooldown;
//     }
//     return false;
// }
//
// Observable<Vector3>& EventDispatcherBackUp::onDisplayPosChanged()
// {
//     return m_onDisplayPosChanged;
// }
//
// } // MORROWGUI