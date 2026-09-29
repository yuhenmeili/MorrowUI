#ifndef MORROW_MAIN_THREAD_DISPATCHER_H_
#define MORROW_MAIN_THREAD_DISPATCHER_H_

#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>

namespace morrow
{
/**
 * @brief Thread-safe one-shot task handoff to the Engine thread.
 *
 * Any thread may post. Only the Engine thread may drain. Tasks posted while a
 * drain is running are deferred until the next drain.
 */
class MainThreadDispatcher {
public:
    using Task = std::function<void()>;
    using WakeCallback = std::function<void()>;

    MainThreadDispatcher() = default;
    ~MainThreadDispatcher();

    MainThreadDispatcher(const MainThreadDispatcher&) = delete;
    MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;

    void setWakeCallback(WakeCallback callback);

    bool post(Task task);

    size_t drain();

    void shutdown();

    [[nodiscard]] bool acceptingTasks() const;

    [[nodiscard]] size_t pendingTaskCount() const;

private:
    mutable std::mutex m_mutex;
    std::deque<Task> m_tasks;
    WakeCallback m_wakeCallback;
    bool m_acceptingTasks = true;
};
}

#endif // MORROW_MAIN_THREAD_DISPATCHER_H_
