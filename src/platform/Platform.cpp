//
// Created by 0060328 on 25-9-18.
//

#include "Platform.h"

#include "FontManager.h"
#include "GlobalObject.h"
#include "OrthographicCamera.h"
#include "RenderDeviceProxy.h"
#include "Window.h"
#include "../ui/base/Widget.h"
#include "../ui/base/Interaction.h"
#include "../ui/base/TouchEvent.h"

namespace morrow {
void Platform::initialize(bool multithread) {
    GlobalObject::getInstance().getRenderingThread()->run(shared_from_this(), multithread);
}

WindowSharedPtr Platform::getWindow() const {
    return m_window;
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
    if (!frameState || !frameState->inputEventsManager || !frameState->camera) {
        return;
    }

    auto& inputEvents = frameState->inputEventsManager->getInputEvents();
    for (auto& touchEvent : inputEvents) {
        touchEvent.target.reset();
        touchEvent.traversalTouchTargets.clear();

        std::shared_ptr<Widget> resolvedTarget;
        const bool isCapturedEvent = touchEvent.eventType == TOUCH_EVENT_TYPE_MOVE ||
            touchEvent.eventType == TOUCH_EVENT_TYPE_RELEASE;
        if (isCapturedEvent) {
            auto captureIt = m_pointerCaptureTargets.find(touchEvent.touchID);
            if (captureIt != m_pointerCaptureTargets.end()) {
                resolvedTarget = captureIt->second.lock();
                if (!resolvedTarget) {
                    m_pointerCaptureTargets.erase(captureIt);
                }
            }
        }

        if (!resolvedTarget) {
            const auto worldPoint = frameState->camera->screenToWorld(touchEvent.positionX, touchEvent.positionY);
            if (m_window) {
                resolvedTarget = findTopmostInteractiveWidget(std::static_pointer_cast<Widget>(m_window), worldPoint.x, worldPoint.y);
            }
        }

        touchEvent.target = resolvedTarget;

        if (touchEvent.eventType == TOUCH_EVENT_TYPE_TOUCH) {
            if (resolvedTarget) {
                m_pointerCaptureTargets[touchEvent.touchID] = resolvedTarget;
            } else {
                m_pointerCaptureTargets.erase(touchEvent.touchID);
            }
        } else if (touchEvent.eventType == TOUCH_EVENT_TYPE_RELEASE) {
            m_pointerCaptureTargets.erase(touchEvent.touchID);
        }
    }
}

void Platform::ensureRenderCapabilitiesInitialized() {
    RENDERINGTHREAD->debugDriver();
    m_isSSBOSupport = RENDERINGTHREAD->checkSSBOSupport();
    GlobalObject::getInstance().getFontManager()->initialize();
}

void Platform::eventHandler(const FrameStateSharedPtr& frameState) {
    bool needRender = false;
    // dispatch event
    for (auto& touchEvent : frameState->inputEventsManager->getInputEvents()) {
        if (touchEvent.target) {
            touchEvent.target->dispatchTouchEvent(touchEvent);
            needRender = true;
        }
        if (!touchEvent.traversalTouchTargets.empty()) {
            for (auto& widget : touchEvent.traversalTouchTargets) {
                widget->dispatchTouchEvent(touchEvent);
            }
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
} // morrow
