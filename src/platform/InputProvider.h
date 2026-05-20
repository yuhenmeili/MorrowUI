//
// Platform-agnostic input provider interface.
// WGL / QNX 等平台实现此接口，由 InputEventsManager 在每帧 poll 拉取事件。
//

#ifndef MORROW_INPUT_PROVIDER_H
#define MORROW_INPUT_PROVIDER_H

#include <vector>
#include <memory>
#include "../ui/base/TouchEvent.h"

namespace morrow {

/// 平台输入提供者接口：每帧由 InputEventsManager::poll() 调用，向 outEvents 填入本帧的触摸/指针事件
class IInputProvider {
public:
    virtual ~IInputProvider() = default;

    /// 拉取本帧输入事件，追加到 outEvents（调用方会在 poll 前清空）
    virtual void poll(std::vector<TouchEvent>& outEvents) = 0;
};

using IInputProviderSharedPtr = std::shared_ptr<IInputProvider>;

} // namespace morrow

#endif // MORROW_INPUT_PROVIDER_H
