#ifndef MORROW_ENGINE_EVENTS_H_
#define MORROW_ENGINE_EVENTS_H_

#include "morrow/base/TouchEvent.h"
#include "morrow/core/Observable.h"
#include "Vector2.h"

namespace morrow
{
/**
 * @brief Synchronous lifecycle events owned by one Engine instance.
 *
 * These events describe completed frame boundaries. They are not a thread-safe
 * queue and must only be connected, disconnected and notified on the Engine
 * thread.
 */
struct EngineEvents {
    /// Fired after the render-request gate passes, before Tween/Widget update.
    Observable<> onFrameBegin;

    /// Fired after endFrame/present and one-shot frame callbacks complete.
    Observable<> onFrameEnd;

    /// 原始键盘/字符输入（KEY_DOWN / CHARACTER），在 Widget 派发前由引擎
    /// 转播。仅主窗口（index 0）桥接至此；其他窗口经 Window::events() 订阅。
    Observable<const TouchEvent&> onRawKeyboardInput;

    /// framebuffer 尺寸变化（Engine 转播主窗口事件；其他窗口经 Window::events()）。
    Observable<const Vector2&> onFramebufferSizeChanged;
};
}

#endif // MORROW_ENGINE_EVENTS_H_
