//
// Created by lance on 2023/1/18.
//

#ifndef MORROW_OBSERVABLE_H
#define MORROW_OBSERVABLE_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/Log.h"

namespace morrow
{
/**
 * @brief Ordered, type-safe multicast callback collection.
 *
 * Observable is intentionally a synchronous, single-thread event primitive.
 * The owner and its subscribers must be connected, disconnected and notified
 * from the same thread unless the owner supplies its own external
 * synchronization.
 */
template<class ...T>
class Observable
{
private:
    struct Slot {
        uint64_t id = 0;
        int64_t priority = 0;
        std::function<void(T...)> callback;
        bool connected = true;
    };

    struct State {
        std::vector<std::shared_ptr<Slot>> slots;
        uint64_t nextId = 1;
    };

public:
    class Connection {
    public:
        Connection() = default;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(Connection&& other) noexcept
            : m_state(std::move(other.m_state)),
              m_id(std::exchange(other.m_id, 0)) {
        }

        Connection& operator=(Connection&& other) noexcept {
            if (this != &other) {
                disconnect();
                m_state = std::move(other.m_state);
                m_id = std::exchange(other.m_id, 0);
            }
            return *this;
        }

        ~Connection() {
            disconnect();
        }

        void disconnect() {
            auto state = m_state.lock();
            if (!state || m_id == 0) {
                m_state.reset();
                m_id = 0;
                return;
            }

            for (const auto& slot : state->slots) {
                if (slot && slot->id == m_id) {
                    slot->connected = false;
                    break;
                }
            }
            m_state.reset();
            m_id = 0;
        }

        [[nodiscard]] bool connected() const {
            auto state = m_state.lock();
            if (!state || m_id == 0) {
                return false;
            }
            for (const auto& slot : state->slots) {
                if (slot && slot->id == m_id) {
                    return slot->connected;
                }
            }
            return false;
        }

    private:
        friend class Observable;

        Connection(const std::shared_ptr<State>& state, uint64_t id)
            : m_state(state), m_id(id) {
        }

        std::weak_ptr<State> m_state;
        uint64_t m_id = 0;
    };

    Observable()
        : m_state(std::make_shared<State>()) {
    }

    ~Observable() {
        clear();
    }

    Observable(const Observable&) = delete;
    Observable& operator=(const Observable&) = delete;

    Observable(Observable&&) = delete;
    Observable& operator=(Observable&&) = delete;

    template<class Callback>
    Connection connect(Callback&& callback, int64_t priority = 0) {
        std::function<void(T...)> function(std::forward<Callback>(callback));
        if (!function) {
            return {};
        }

        compact();
        auto slot = std::make_shared<Slot>();
        slot->id = m_state->nextId++;
        slot->priority = priority;
        slot->callback = std::move(function);

        const auto insertPosition = std::find_if(
            m_state->slots.begin(),
            m_state->slots.end(),
            [priority](const std::shared_ptr<Slot>& existing) {
                return existing && existing->priority < priority;
            });
        m_state->slots.insert(insertPosition, slot);
        return Connection(m_state, slot->id);
    }

    void notify(T ...args) {
        // Snapshot slots so a callback can connect without joining this emit.
        // We still check connected immediately before invocation, so removing
        // a not-yet-called slot takes effect during the current emit.
        const auto snapshot = m_state->slots;
        for (const auto& slot : snapshot) {
            if (slot && slot->connected && slot->callback) {
                slot->callback(args...);
            }
        }
        compact();
    }

    [[nodiscard]] int32_t size() const {
        int32_t count = 0;
        for (const auto& slot : m_state->slots) {
            if (slot && slot->connected) {
                ++count;
            }
        }
        return count;
    }

    void clear() {
        for (const auto& slot : m_state->slots) {
            if (slot) {
                slot->connected = false;
            }
        }
        compact();
    }

    void log(const std::string& msg) const {
        LOG_I("{}, observers size {}", msg.c_str(), size());
    }

private:
    void compact() {
        m_state->slots.erase(
            std::remove_if(
                m_state->slots.begin(),
                m_state->slots.end(),
                [](const std::shared_ptr<Slot>& slot) {
                    return !slot || !slot->connected;
                }),
            m_state->slots.end());
    }

    std::shared_ptr<State> m_state;
};

} // namespace morrow

#endif // MORROW_OBSERVABLE_H
