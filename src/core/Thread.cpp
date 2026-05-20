//
// Created by lance on 2023/11/14.
//

#include "Thread.h"

namespace morrow
{
Thread::Thread()
{
    worker = std::thread(&Thread::queueLoop, this);
}

Thread::~Thread()
{
    if (worker.joinable()) {
        wait();
        queueMutex.lock();
        destroying = true;
        condition.notify_one();
        queueMutex.unlock();
        worker.join();
    }
}

void Thread::addJob(std::function<void()> function)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    if (m_commandPool.empty()) {
        m_commandPool.push(std::make_shared<Command>());
    }
    CommandSharedPtr renderCommand = m_commandPool.pop();
    renderCommand->func = std::move(function);
    m_renderQueue.push(renderCommand);
    condition.notify_one();
}

void Thread::wait()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    condition.wait(lock, [this]() { return m_renderQueue.empty(); });
}

void Thread::queueLoop()
{
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !m_renderQueue.empty() || destroying; });
            if (destroying) {
                break;
            }
            job = m_renderQueue.front()->func;
        }

        job();

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            m_commandPool.push(m_renderQueue.pop());
            condition.notify_one();
        }
    }
}

void ThreadPool::setThreadCount(uint32_t count)
{
    threads.clear();
    for (auto i = 0; i < count; i++) {
        threads.push_back(make_unique<Thread>());
    }
}

void ThreadPool::wait()
{
    for (auto& thread : threads) {
        thread->wait();
    }
}
} // MORROWGUI