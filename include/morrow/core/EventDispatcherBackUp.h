// //
// // Created by lance on 2023/7/18.
// //
//
// #ifndef MORROW_CORE_EVENTDISPATCHER_H_
// #define MORROW_CORE_EVENTDISPATCHER_H_
//
// #include <functional>
// #include <map>
// #include <memory>
// #include "Vector3.h"
// #include "Observable.h"
// #include "Rect.h"
//
// namespace morrow {
// class Widget;
// }
//
// namespace morrow
// {
// using namespace Math;
//
// class FrameState;
// using FrameStateSharedPtr = std::shared_ptr<FrameState>;
//
// enum TouchEventType : int32_t
// {
//     TOUCH_EVENT_TYPE_NONE = 0,
//     TOUCH_EVENT_TYPE_TOUCH = 1,
//     TOUCH_EVENT_TYPE_MOVE = 2,
//     TOUCH_EVENT_TYPE_RELEASE = 3,
//     TOUCH_EVENT_TYPE_TOUCH_AND_HOLD = 4,
//     TOUCH_EVENT_TYPE_CLICK = 5
// };
//
// struct TouchEvent
// {
//     int32_t touchID = -1;//0,1,2...
//     float positionX;
//     float positionY;
//     TouchEventType eventType = TOUCH_EVENT_TYPE_NONE;
//     double touchTime = 0.0;
//     int32_t totalTouchIDCount = 0;//当前帧touchid总数
//
//     int32_t callbackTimes = 0;//回调触发次数， 长按回调用
//     std::shared_ptr<Widget> target;
//     std::vector<std::shared_ptr<Widget>> traversalTouchTargets;
//
//     //copy struct
//     TouchEvent& operator=(const TouchEvent& other)
//     {
//         touchID = other.touchID;
//         positionX = other.positionX;
//         positionY = other.positionY;
//         eventType = other.eventType;
//         touchTime = other.touchTime;
//         totalTouchIDCount = other.totalTouchIDCount;
// //        callbackTimes = other.callbackTimes;
// //        target = other.target;
// //        traversalTouchTargets = other.traversalTouchTargets;
//         return *this;
//     }
// };
//
// //using TouchEventSharedPtr = std::shared_ptr<TouchEvent>;
//
// struct TouchEventCallback
// {
//     std::function<void(TouchEvent&)> callback;
//     //只针对TOUCH_EVENT_TYPE_TOUCH_AND_HOLD
//     float delay = 0.0f;//延迟多久触发
// };
//
// using TouchEventCallbackSharedPtr = std::shared_ptr<TouchEventCallback>;
//
// class EventDispatcherBackUp
// {
// public:
//     virtual ~EventDispatcherBackUp();
//
//     //暂时不实现捕获阶段
//     void addEventListener(TouchEventType eventType, TouchEventCallbackSharedPtr callbackSharedPtr, bool useCapture = false);
//
//     void removeEventListener(TouchEventType eventType, TouchEventCallbackSharedPtr callbackSharedPtr);
//
//     bool hasEventListener(TouchEventType eventType);
//
//     void touchHander(TouchEvent& event);
//
//     bool isTraversalTouchEnabled() const;
//
//     void setTraversalTouchEnabled(bool traversalTouchEnabled);
//
//     bool hasTouched() const;
//
//     void setTouchCooldown(double cd);
//
//     Observable<Vector3>& onDisplayPosChanged();
//
// protected:
//     bool isInTouchCooldown(TouchEvent& event) const;
//
//     void dispatchEvent(TouchEvent& event);
//
//     void moveHandler(FrameStateSharedPtr frameState);
//
//     void touchAndHoldHandler(FrameStateSharedPtr frameState, Rect bb);
//
//     void clickHandler(FrameStateSharedPtr frameState, Rect bb);
//
//     void releaseHandler(FrameStateSharedPtr frameState);
//
//     /**
//      * 当组件隐藏的时候，用于状态清理
//      */
//     virtual void reset();
//
// protected:
//     //目前button专用
//     Observable<Vector3> m_onDisplayPosChanged;
//
// private:
//     std::map<TouchEventType, std::vector<TouchEventCallbackSharedPtr>> m_eventListeners;
//     bool m_traversalTouchEnabled = false;
//     double m_touchTime = 0.0;
//     double m_releaseTime = 0.0;
//     double m_touchCooldown = 0.0;
//     std::vector<TouchEvent> m_holdEvents;
// };
//
// } // MORROWGUI
//
// #endif //MORROW_CORE_EVENTDISPATCHER_H_
