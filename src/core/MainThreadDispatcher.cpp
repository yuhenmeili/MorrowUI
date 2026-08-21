#include "MainThreadDispatcher.h"

#include <exception>
#include <utility>

#include "utils/Log.h"

namespace morrow {
MainThreadDispatcher::~MainThreadDispatcher() {
    shutdown();
}

void MainThreadDispatcher::setWakeCallback(WakeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_acceptingTasks) {
        return;
    }
    m_wakeCallback = std::move(callback);
}

bool MainThreadDispatcher::post(Task task) {
    if (!task) {
        return false;
    }

    WakeCallback wakeCallback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_acceptingTasks) {
            return false;
        }
        m_tasks.push_back(std::move(task));
        wakeCallback = m_wakeCallback;
    }

    if (wakeCallback) {
        wakeCallback();
    }
    return true;
}

size_t MainThreadDispatcher::drain() {
    std::deque<Task> tasks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        tasks.swap(m_tasks);
    }

    size_t executedTaskCount = 0;
    for (auto& task : tasks) {
        if (!task) {
            continue;
        }
        try {
            task();
        } catch (const std::exception& exception) {
            LOG_E("MainThreadDispatcher task failed: {}", exception.what());
        } catch (...) {
            LOG_E("MainThreadDispatcher task failed with an unknown exception");
        }
        ++executedTaskCount;
    }
    return executedTaskCount;
}

void MainThreadDispatcher::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_acceptingTasks = false;
    m_tasks.clear();
    m_wakeCallback = {};
}

bool MainThreadDispatcher::acceptingTasks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_acceptingTasks;
}

size_t MainThreadDispatcher::pendingTaskCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}
}  // namespace morrow
