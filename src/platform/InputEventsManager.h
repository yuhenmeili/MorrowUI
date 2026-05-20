//
// Created by lance on 2023/1/18.
//

#ifndef MORROW_OBSERVABLES_MANAGER_H
#define MORROW_OBSERVABLES_MANAGER_H

#include "../core/Observable.h"
#include "../core/EventDispatcherBackUp.h"
// #include "screen/screen.h"
#include <memory>

#include "Rect.h"
#include "Vector2.h"
#include "../ui/base/TouchEvent.h"
#include "InputProvider.h"

namespace morrow {
using namespace Math;

class InputEventsManager {
public:
    /// 设置平台输入提供者（WGL / QNX 等）。每帧在 update 前调用 poll() 拉取事件
    void setInputProvider(IInputProviderSharedPtr provider);

    /// 从当前 provider 拉取本帧事件到 getInputEvents()；若无 provider 则清空
    void poll();

    Observable<std::vector<TouchEvent>&>& getInputEventsDispatcher();

    int32_t GetRectTouchDownID(const Rect& bb);

    bool IsRectTouchRelease(int32_t touchID);

    int32_t GetTouchIDCountByRect(const Rect& bb);

    bool isTouchIDInRect(int32_t touchID, const Rect& bb);

    int32_t GetTouchIDCount();

    std::vector<TouchEvent>& getInputEvents();

    bool hasTouchID(int32_t touchID);

private:
    IInputProviderSharedPtr m_inputProvider;
    Observable<std::vector<TouchEvent>&> m_inputEventsDispatcher;
    std::vector<TouchEvent> m_lastFrameInputEvents;
    std::vector<TouchEvent> m_inputEvents;
};

using InputEventsManagerSharedPtr = std::shared_ptr<InputEventsManager>;
}

#endif //MORROW_OBSERVABLES_MANAGER_H
