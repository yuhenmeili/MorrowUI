//
// Created by lance on 2023/11/14.
//

#ifndef MORROW_CORE_THREAD_H_
#define MORROW_CORE_THREAD_H_

#include <functional>
#include <condition_variable>
#include <thread>
#include <queue>
#include "CommandQueue.h"

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace morrow
{
class Thread {
public:
    Thread();

    virtual ~Thread();

    void addJob(std::function<void()> function);

    void wait();

private:
    void queueLoop();

private:
    bool destroying = false;
    std::thread worker;
    std::mutex queueMutex;
    std::condition_variable condition;
    CommandQueue m_renderQueue;
    CommandQueue m_commandPool;
};

class ThreadPool {
public:
    std::vector<std::unique_ptr<Thread>> threads;

    void setThreadCount(uint32_t count);

    void wait();
};

} // MORROWGUI

#endif //MORROW_CORE_THREAD_H_
