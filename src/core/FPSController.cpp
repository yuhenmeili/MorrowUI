//
// Created by lance on 24-4-11.
//

#include <thread>
#include "FPSController.h"

namespace morrow
{

FPSController::FPSController(int32_t fps)
{
    m_frameTime = std::chrono::duration<double>(1.0 / fps);
    m_startTime = std::chrono::high_resolution_clock::now();
}

void FPSController::sleep()
{
    std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - m_startTime;
    std::chrono::duration<double> sleepTime = m_frameTime - elapsed;
    if (sleepTime.count() > 0) {
        std::this_thread::sleep_for(sleepTime);
    }
    m_startTime = std::chrono::high_resolution_clock::now();
}

void FPSController::setFPS(int32_t fps)
{
    m_frameTime = std::chrono::duration<double>(1.0 / fps);
}
} // MORROWGUI