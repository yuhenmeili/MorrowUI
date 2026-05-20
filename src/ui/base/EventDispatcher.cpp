//
// Created by lance on 2025/12/15.
//

#include "EventDispatcher.h"
#include <algorithm>

namespace morrow {

ListenerHandle EventDispatcher::addEventListener(TouchEventType type, std::function<void(TouchEvent&)> callback) {
    if (!callback) return 0;
    ListenerHandle h = m_nextHandle++;
    m_listeners[type].push_back(ListenerEntry{h, std::move(callback)});
    return h;
}

void EventDispatcher::removeEventListener(ListenerHandle handle) {
    if (handle == 0) return;
    for (auto& pair : m_listeners) {
        auto& list = pair.second;
        list.erase(
            std::remove_if(list.begin(), list.end(),
                           [handle](const ListenerEntry& e) { return e.handle == handle; }),
            list.end());
    }
}

void EventDispatcher::removeEventListener(TouchEventType type) {
    m_listeners.erase(type);
}

bool EventDispatcher::hasEventListener(TouchEventType type) const {
    auto it = m_listeners.find(type);
    return it != m_listeners.end() && !it->second.empty();
}

void EventDispatcher::dispatchEvent(TouchEventType type, TouchEvent& event) {
    auto it = m_listeners.find(type);
    if (it == m_listeners.end()) return;
    for (const auto& entry : it->second) {
        if (entry.callback) entry.callback(event);
    }
}

void EventDispatcher::clearEventListeners() {
    m_listeners.clear();
}

} // namespace morrow
