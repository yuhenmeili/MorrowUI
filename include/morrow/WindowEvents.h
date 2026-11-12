//
// WindowEvents — 窗口事件（公共）。
//
// 由引擎内部实现（PlatformWindow）通知，应用经 Window::events() 按窗口订阅。
//

#ifndef MORROW_WINDOWEVENTS_H
#define MORROW_WINDOWEVENTS_H

#include "Vector2.h"

#include "morrow/base/TouchEvent.h"
#include "morrow/core/Observable.h"

namespace morrow
{

struct WindowEvents {
    // Vector2 = morrow::Math::Vector2（src/math，经目标 include 目录可达；
    // 此处用限定名，头文件单独包含即可编译，不依赖 Widget.h 的
    // using namespace Math 注入顺序）
    Observable<const Math::Vector2&> onWindowSizeChanged;
    Observable<const Math::Vector2&> onFramebufferSizeChanged;
    Observable<float> onContentScaleChanged;
    /// 原始键盘/字符输入（KEY_DOWN / CHARACTER），在 Widget 派发前广播。
    /// 仅主窗口（index 0）由 Engine 转播到 EngineEvents::onRawKeyboardInput，
    /// 其他窗口需应用自行连接各窗口的 Window::events()。
    Observable<const TouchEvent&> onRawKeyboardInput;
};

} // namespace morrow

#endif // MORROW_WINDOWEVENTS_H
