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

    /// 原始键盘/字符输入（KEY_DOWN / CHARACTER），在 Widget 派发前由引擎转播
    /// （Window 内部化后的公共输入入口，见 §6.7）。
    Observable<const TouchEvent&> onRawKeyboardInput;

    /// framebuffer 尺寸变化（Engine 转播内部窗口事件）。
    Observable<const Vector2&> onFramebufferSizeChanged;
};
}

#endif // MORROW_ENGINE_EVENTS_H_
