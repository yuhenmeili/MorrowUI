//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_PLATFORMSEMAPHORE_H_
#define MORROW_RENDERER_PLATFORMSEMAPHORE_H_

#include <semaphore.h>
//#include <errno.h>


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
    sem_t m_Semaphore;
};

inline void PlatformSemaphore::create()
{
    sem_init(&m_Semaphore, 0, 0);
}

inline void PlatformSemaphore::destroy()
{
    sem_destroy(&m_Semaphore);
}

inline void PlatformSemaphore::waitForSignal()
{
    sem_wait(&m_Semaphore);
}

inline void PlatformSemaphore::signal()
{
    sem_post(&m_Semaphore);
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
