//
// Window — 公共窗口门面的转发实现（见 include/morrow/Window.h）。
// 全部方法一比一转发到引擎内部实现 PlatformWindow（src/platform/PlatformWindow.h）。
//

#include "morrow/Window.h"

// 引擎内部实现头（与上面的公共门面 morrow/Window.h 是两个文件）。
#include "PlatformWindow.h"

namespace morrow {

Window::Window(std::shared_ptr<PlatformWindow> impl)
    : m_impl(std::move(impl)) {}

Window::~Window() = default;

void Window::setClearColor(float r, float g, float b, float a) {
    m_impl->setClearColor(r, g, b, a);
}

void Window::setCursorShape(CursorShape shape) {
    m_impl->setCursorShape(shape);
}

void Window::setClipboardText(const std::string& text) {
    m_impl->setClipboardText(text);
}

std::string Window::clipboardText() const {
    return m_impl->clipboardText();
}

Vector2 Window::framebufferSize() const {
    return m_impl->framebufferSize();
}

std::shared_ptr<Scene2D> Window::scene2D() const {
    return m_impl->scene2D();
}

WindowEvents& Window::events() {
    return m_impl->events();
}

void Window::requestRender(const char* reason) {
    m_impl->requestRender(reason);
}

} // namespace morrow
