//
// QNX 平台输入提供者桩：实现 IInputProvider，poll() 为空。
// 实际 QNX 可将 InputEventsManager.cpp 中注释掉的 qnxInputEventsHandle 逻辑迁入此处，
// 在 poll() 内调用 screen_get_event 等并填充 outEvents。
//

#ifndef MORROW_QNX_INPUT_PROVIDER_H
#define MORROW_QNX_INPUT_PROVIDER_H

#include <screen/screen.h>
#include "../InputProvider.h"

namespace morrow {

class QNXInputProvider : public IInputProvider {
public:
    void setScreenContext(screen_context_t screenContext, screen_event_t screenEvent);

    void setWindowOffset(int32_t windowX, int32_t windowY);

    void setSkipEvents(bool skipEvents);

    void poll(std::vector<TouchEvent>& outEvents) override;

private:
    bool hasTouchID(const std::vector<TouchEvent>& events, int32_t touchID) const;

    screen_context_t m_screenContext = nullptr;
    screen_event_t m_screenEvent = nullptr;
    int32_t m_windowX = 0;
    int32_t m_windowY = 0;
    bool m_skipEvents = false;
    std::vector<TouchEvent> m_lastFrameInputEvents;
};

} // namespace morrow

#endif // MORROW_QNX_INPUT_PROVIDER_H
