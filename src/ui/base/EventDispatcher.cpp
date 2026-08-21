//
// Created by lance on 2025/12/15.
//

#include "EventDispatcher.h"
#include <algorithm>

namespace morrow {

EventConnection EventDispatcher::addEventListener(
    TouchEventType type,
    std::function<void(TouchEvent&)> callback,
    int64_t priority) {
    if (!callback) {
        return {};
    }
    auto& observable = m_listeners[type];
    if (!observable) {
        observable = std::make_unique<Observable<TouchEvent&>>();
    }
    return observable->connect(std::move(callback), priority);
}

void EventDispatcher::removeEventListener(TouchEventType type) {
    auto it = m_listeners.find(type);
    if (it != m_listeners.end() && it->second) {
        it->second->clear();
    }
}

bool EventDispatcher::hasEventListener(TouchEventType type) const {
    auto it = m_listeners.find(type);
    return it != m_listeners.end() && it->second && it->second->size() > 0;
}

void EventDispatcher::dispatchEvent(TouchEventType type, TouchEvent& event) {
    auto it = m_listeners.find(type);
    if (it == m_listeners.end() || !it->second) {
        return;
    }
    it->second->notify(event);
}

void EventDispatcher::clearEventListeners() {
    for (auto& [type, observable] : m_listeners) {
        (void)type;
        if (observable) {
            observable->clear();
        }
    }
}

} // namespace morrow
