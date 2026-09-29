//
// Created by lance on 24-4-11.
//

#ifndef MORROW_CORE_FPSCONTROLLER_H_
#define MORROW_CORE_FPSCONTROLLER_H_

#include <chrono>
#include <memory>

namespace morrow
{

class FPSController
{
public:
    explicit FPSController(int32_t fps = 60);

    void sleep();

    void setFPS(int32_t fps);

private:
    std::chrono::duration<double> m_frameTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
};

using FPSControllerPtr =  std::shared_ptr<FPSController>;

} // MORROWGUI

#endif //MORROW_CORE_FPSCONTROLLER_H_
