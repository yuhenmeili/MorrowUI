//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_PLATFORMSEMAPHORE_H_
#define MORROW_RENDERER_PLATFORMSEMAPHORE_H_

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace morrow
{
class PlatformSemaphore
{
    friend class Semaphore;

protected:
    void create();

    void destroy();

    void waitForSignal();

    void signal();

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    uint32_t m_signalCount = 0;
};

inline void PlatformSemaphore::create()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signalCount = 0;
}

inline void PlatformSemaphore::destroy()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signalCount = 0;
}

inline void PlatformSemaphore::waitForSignal()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return m_signalCount > 0; });
    --m_signalCount;
}

inline void PlatformSemaphore::signal()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_signalCount;
    }
    m_condition.notify_one();
}

class Semaphore
{
public:
    Semaphore()
    {
        m_Semaphore.create();
    }

    ~Semaphore()
    {
        m_Semaphore.destroy();
    }

    void reset()
    {
        m_Semaphore.destroy();
        m_Semaphore.create();
    }

    void waitForSignal()
    {
        m_Semaphore.waitForSignal();
    }

    void signal()
    {
        m_Semaphore.signal();
    }

private:
    PlatformSemaphore m_Semaphore{};
};
}
#endif //MORROW_RENDERER_PLATFORMSEMAPHORE_H_
