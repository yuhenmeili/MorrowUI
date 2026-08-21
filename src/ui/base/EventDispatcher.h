//
// Created by lance on 2025/12/15.
//

#ifndef MORROW_GUI_EVENTDISPATCHER_H
#define MORROW_GUI_EVENTDISPATCHER_H
#include <map>
#include <memory>

#include "TouchEvent.h"
#include "core/Observable.h"

namespace morrow {
using EventConnection = Observable<TouchEvent&>::Connection;

class EventDispatcher {
public:
    EventDispatcher() = default;

    virtual ~EventDispatcher() = default;

    /// 添加监听器，返回 RAII 连接用于主动或自动移除。
    EventConnection addEventListener(TouchEventType type, std::function<void(TouchEvent&)> callback, int64_t priority = 0);

    /// 按类型移除该类型下所有监听器
    void removeEventListener(TouchEventType type);

    bool hasEventListener(TouchEventType type) const;

    /// 派发事件（由 Interaction 等调用）
    void dispatchEvent(TouchEventType type, TouchEvent& event);

protected:
    void clearEventListeners();

private:
    std::map<TouchEventType, std::unique_ptr<Observable<TouchEvent&>>> m_listeners;
};
}  // namespace morrow

#endif  // MORROW_GUI_EVENTDISPATCHER_H
