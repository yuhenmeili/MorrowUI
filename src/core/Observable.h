//
// Created by lance on 2023/1/18.
//

#ifndef MORROW_OBSERVABLE_H
#define MORROW_OBSERVABLE_H

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include "MathUtils.h"
#include "utils/Log.h"

namespace morrow
{
template<class ...T>
class Observable
{
public:
    virtual ~Observable()
    {
        clear();
    };

    std::string add(const std::function<void(T...)>& observer)
    {
        std::string uuid = Math::generate_uuid();
        m_observers[uuid] = observer;
        return uuid;
    }

    void remove(std::string uuid)
    {
        if (m_observers.find(uuid) != m_observers.end()) {
            m_observers.erase(uuid);
        }
    }

    void notify(T ...Args)
    {
        // Use a snapshot so callbacks can safely add/remove observers while notifying.
        std::vector<std::function<void(T...)>> callbacks;
        callbacks.reserve(m_observers.size());
        for (const auto& observer : m_observers) {
            callbacks.push_back(observer.second);
        }

        for (const auto& callback : callbacks) {
            if (callback) {
                callback(Args...);
            }
        }
    }

    int32_t size()
    {
        return m_observers.size();
    }

    void clear()
    {
        m_observers.clear();
    }

    void log(const std::string& msg)
    {
        LOG_I("{}, observers size {}", msg.c_str(), m_observers.size());
    }

private:
    std::unordered_map<std::string, std::function<void(T...)>> m_observers;
};

} // MORROWGUI

#endif //MORROW_OBSERVABLE_H
