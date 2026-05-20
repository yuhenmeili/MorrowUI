//
// Created by lance on 2025/12/15.
//

#ifndef MORROW_GUI_EVENTDISPATCHER_H
#define MORROW_GUI_EVENTDISPATCHER_H
#include <functional>
#include <map>
#include <vector>
#include "TouchEvent.h"

namespace morrow {
/// 监听器句柄，用于移除监听
using ListenerHandle = uint64_t;

class EventDispatcher {
public:
    EventDispatcher() = default;

    virtual ~EventDispatcher() = default;

    /// 添加监听器，返回句柄用于移除
    ListenerHandle addEventListener(TouchEventType type, std::function<void(TouchEvent&)> callback);

    /// 按句柄移除监听器
    void removeEventListener(ListenerHandle handle);

    /// 按类型移除该类型下所有监听器
    void removeEventListener(TouchEventType type);

    bool hasEventListener(TouchEventType type) const;

    /// 派发事件（由 Interaction 等调用）
    void dispatchEvent(TouchEventType type, TouchEvent& event);

protected:
    void clearEventListeners();

private:
    struct ListenerEntry {
        ListenerHandle handle;
        std::function<void(TouchEvent&)> callback;
    };

    ListenerHandle m_nextHandle = 1;
    std::map<TouchEventType, std::vector<ListenerEntry>> m_listeners;
};
} // morrow

#endif //MORROW_GUI_EVENTDISPATCHER_H
