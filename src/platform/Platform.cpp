//
// Created by 0060328 on 25-9-18.
//

#include "Platform.h"

#include <chrono>
#include <thread>

#include "FontManager.h"
#include "GlobalObject.h"
#include "RenderDeviceProxy.h"
#include "Window.h"
#include "ui/base/Interaction.h"
#include "ui/base/TouchEvent.h"
#include "ui/base/Widget.h"

namespace morrow {
void Platform::initialize(bool multithread) {
    GlobalObject::getInstance().getRenderingThread()->run(shared_from_this(), multithread);
}

WindowSharedPtr Platform::getWindow() const {
    return m_window;
}

void Platform::waitForEvents(double timeoutSeconds) {
    if (timeoutSeconds <= 0.0)
        return;
    std::this_thread::sleep_for(std::chrono::duration<double>(timeoutSeconds));
}

void Platform::wakeEventLoop() {
}

bool Platform::shouldClose() const {
    return m_window->isWindowShouldClose();
}

std::shared_ptr<Widget> Platform::findTopmostInteractiveWidget(const std::shared_ptr<Widget>& root, float x, float y) {
    if (!root || !root->getVisible()) {
        return nullptr;
    }

    for (auto childIt = root->m_children.rbegin(); childIt != root->m_children.rend(); ++childIt) {
        if (auto hit = findTopmostInteractiveWidget(*childIt, x, y)) {
            return hit;
        }
    }

    auto interaction = root->getComponent<Interaction>();
    if (interaction && interaction->isInteractionEnabled() && interaction->containsPoint(x, y)) {
        return root;
    }

    return nullptr;
}

void Platform::resolveInputTargets(const FrameStateSharedPtr& frameState) {
    if (!frameState || !frameState->inputEventsManager) {
        return;
    }

    auto& inputEvents = frameState->inputEventsManager->getInputEvents();
    std::vector<TouchEvent> resolvedEvents;
    resolvedEvents.reserve(inputEvents.size() * 3);

    for (auto& touchEvent : inputEvents) {
        touchEvent.target.reset();
        std::shared_ptr<Widget> resolvedTarget;
        bool fromCapture = false;
        const bool isKeyboardEvent = touchEvent.eventType == TOUCH_EVENT_TYPE_CHARACTER || touchEvent.eventType == TOUCH_EVENT_TYPE_KEY_DOWN;
        if (isKeyboardEvent) {
            resolvedTarget = m_keyboardFocusTarget.lock();
        }
        const bool isCapturedEvent = touchEvent.eventType == TOUCH_EVENT_TYPE_MOVE || touchEvent.eventType == TOUCH_EVENT_TYPE_RELEASE;
        if (!isKeyboardEvent && isCapturedEvent) {
            auto captureIt = m_pointerCaptureTargets.find(touchEvent.touchID);
            if (captureIt != m_pointerCaptureTargets.end()) {
                resolvedTarget = captureIt->second.lock();
                fromCapture = (resolvedTarget != nullptr);
                if (!resolvedTarget) {
                    m_pointerCaptureTargets.erase(captureIt);
                }
            }
        }
        if (!isKeyboardEvent && !resolvedTarget) {
            resolvedTarget = findTopmostInteractiveWidget(m_window, touchEvent.positionX, touchEvent.positionY);
        }
        touchEvent.target = resolvedTarget;

        if (touchEvent.eventType == TOUCH_EVENT_TYPE_MOVE && touchEvent.deviceType == TOUCH_DEVICE_TYPE_MOUSE) {
            // Capture controls the MOVE target while dragging, whereas hover
            // transitions follow the widget physically under the cursor. 未捕获时
            // 二者在同一坐标下命中结果一致，直接复用 resolvedTarget，省一次全树遍历。
            std::shared_ptr<Widget> hoverTarget = fromCapture ? findTopmostInteractiveWidget(m_window, touchEvent.positionX, touchEvent.positionY) : resolvedTarget;
            auto previousHover = m_hoverTarget.lock();
            if (previousHover != hoverTarget) {
                if (previousHover) {
                    TouchEvent leaveEvent = touchEvent;
                    leaveEvent.eventType = TOUCH_EVENT_TYPE_POINTER_LEAVE;
                    leaveEvent.target = previousHover;
                    resolvedEvents.push_back(std::move(leaveEvent));
                }
                if (hoverTarget) {
                    TouchEvent enterEvent = touchEvent;
                    enterEvent.eventType = TOUCH_EVENT_TYPE_POINTER_ENTER;
                    enterEvent.target = hoverTarget;
                    resolvedEvents.push_back(std::move(enterEvent));
                }
            }
            m_hoverTarget = hoverTarget;
        }

        if (touchEvent.eventType == TOUCH_EVENT_TYPE_TOUCH) {
            auto previousFocus = m_keyboardFocusTarget.lock();
            std::shared_ptr<Widget> nextFocus;
            if (resolvedTarget) {
                auto interaction = resolvedTarget->getComponent<Interaction>();
                if (interaction && interaction->isKeyboardFocusable()) {
                    nextFocus = resolvedTarget;
                }
            }
            if (previousFocus != nextFocus) {
                if (previousFocus)
                    previousFocus->onFocusChanged(false);
                if (nextFocus)
                    nextFocus->onFocusChanged(true);
                m_keyboardFocusTarget = nextFocus;
            }
            if (resolvedTarget) {
                m_pointerCaptureTargets[touchEvent.touchID] = resolvedTarget;
            } else {
                m_pointerCaptureTargets.erase(touchEvent.touchID);
            }
        } else if (touchEvent.eventType == TOUCH_EVENT_TYPE_RELEASE) {
            m_pointerCaptureTargets.erase(touchEvent.touchID);
        }

        resolvedEvents.push_back(std::move(touchEvent));
    }

    inputEvents = std::move(resolvedEvents);
    frameState->inputEventsManager->notifyInputEventsResolved();
}

void Platform::ensureRenderCapabilitiesInitialized() {
    RENDERINGTHREAD->debugDriver();
    m_isSSBOSupport = RENDERINGTHREAD->checkSSBOSupport();
    GlobalObject::getInstance().getFontManager()->initialize();
}

void Platform::dispatchEvents(const FrameStateSharedPtr& frameState) {
    bool needRender = false;
    // dispatch event
    for (auto& touchEvent : frameState->inputEventsManager->getInputEvents()) {
        if (touchEvent.target) {
            touchEvent.target->dispatchTouchEvent(touchEvent);
            needRender = true;
        }
    }
    if (!frameState->callAfterTouched.empty()) {
        for (const auto& callback : frameState->callAfterTouched) {
            callback();
        }
        needRender = true;
    }
    if (needRender) {
        REQUESTRENDER;
    }
}

void Platform::updateWidgets(FrameStateSharedPtr frameState) {
    if (m_window) {
        m_window->updateWidgets(frameState);
    }
}

void Platform::lateUpdateWidgets(FrameStateSharedPtr frameState) {
    if (m_window) {
        m_window->lateUpdateWidgets(frameState);
    }
}
}  // namespace morrow
