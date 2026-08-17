#include "OrbitController.h"

#include <algorithm>
#include <cmath>

#include "OrbitCamera.h"
#include "OrthographicCamera.h"
#include "base/UIWidget.h"
#include "base/TouchEvent.h"
#include "elements/MR3DSceneView.h"

namespace morrow {
namespace {
constexpr float kMinRotateSpeed = 0.0001f;
constexpr float kMinZoomSpeed = 0.0001f;
constexpr float kMinPinchDistance = 1.0f;

bool isDirectManipulationDevice(TouchDeviceType deviceType) {
    return deviceType == TOUCH_DEVICE_TYPE_TOUCH || deviceType == TOUCH_DEVICE_TYPE_PEN;
}

bool canStartRotate(const TouchEvent& event) {
    return event.button == TOUCH_MOUSE_BUTTON_LEFT || isDirectManipulationDevice(event.deviceType);
}

bool matchesActivePointer(const TouchEvent& event, int32_t activeTouchID, TouchMouseButton activeButton, TouchDeviceType activeDeviceType) {
    if (event.touchID != activeTouchID) {
        return false;
    }

    if (isDirectManipulationDevice(activeDeviceType)) {
        return isDirectManipulationDevice(event.deviceType) || event.deviceType == TOUCH_DEVICE_TYPE_UNKNOWN;
    }

    return event.button == activeButton;
}

bool shouldRotateForActivePointer(TouchMouseButton activeButton, TouchDeviceType activeDeviceType) {
    return activeButton == TOUCH_MOUSE_BUTTON_LEFT || isDirectManipulationDevice(activeDeviceType);
}

float calculatePinchDistance(const std::unordered_map<int32_t, Math::Vector2>& trackedTouchPoints, int32_t touchID0, int32_t touchID1) {
    auto first = trackedTouchPoints.find(touchID0);
    auto second = trackedTouchPoints.find(touchID1);
    if (first == trackedTouchPoints.end() || second == trackedTouchPoints.end()) {
        return 0.0f;
    }

    return (second->second - first->second).length();
}
}

OrbitController::OrbitController(std::shared_ptr<MR3DSceneView> view) {
    m_view = view;
    m_camera = view->getCamera();
}

void OrbitController::update(const FrameStateSharedPtr& frameState) {
    if (!m_enabled) {
        return;
    }

    for (const auto& event : frameState->inputEventsManager->getInputEvents()) {
        if (event.target || !event.traversalTouchTargets.empty()) {
            continue;
        }
        handleEvent(event, frameState);
    }
}

void OrbitController::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    if (!m_enabled) {
        reset();
    }
}

bool OrbitController::isEnabled() const {
    return m_enabled;
}

void OrbitController::setRotateEnabled(bool enabled) {
    m_rotateEnabled = enabled;
    if (!m_rotateEnabled && m_isDragging) {
        reset();
    }
}

bool OrbitController::isRotateEnabled() const {
    return m_rotateEnabled;
}

void OrbitController::setZoomEnabled(bool enabled) {
    m_zoomEnabled = enabled;
    if (!m_zoomEnabled && m_isPinching) {
        endPinch();
    }
}

bool OrbitController::isZoomEnabled() const {
    return m_zoomEnabled;
}

void OrbitController::setRotateSpeed(float speed) {
    m_rotateSpeed = std::max(speed, kMinRotateSpeed);
}

float OrbitController::getRotateSpeed() const {
    return m_rotateSpeed;
}

void OrbitController::setZoomSpeed(float speed) {
    m_zoomSpeed = std::max(speed, kMinZoomSpeed);
}

float OrbitController::getZoomSpeed() const {
    return m_zoomSpeed;
}

void OrbitController::setMinDistance(float distance) {
    m_minDistance = std::max(distance, 0.001f);
    if (m_maxDistance < m_minDistance) {
        m_maxDistance = m_minDistance;
    }
}

float OrbitController::getMinDistance() const {
    return m_minDistance;
}

void OrbitController::setMaxDistance(float distance) {
    m_maxDistance = std::max(distance, m_minDistance);
}

float OrbitController::getMaxDistance() const {
    return m_maxDistance;
}

void OrbitController::reset() {
    m_isDragging = false;
    m_isPinching = false;
    m_activeTouchID = -1;
    m_pinchTouchID0 = -1;
    m_pinchTouchID1 = -1;
    m_activeDeviceType = TOUCH_DEVICE_TYPE_UNKNOWN;
    m_activeButton = TOUCH_MOUSE_BUTTON_NONE;
    m_lastPinchDistance = 0.0f;
    m_lastPointer.set(0.0f, 0.0f);
    m_trackedTouchPoints.clear();
}

void OrbitController::updateTrackedTouchPoint(const TouchEvent& event, bool requireInsideView, const FrameStateSharedPtr& frameState) {
    if (!canParticipateInPinch(event)) {
        return;
    }

    if (requireInsideView && !isEventInsideView(event, frameState)) {
        return;
    }

    m_trackedTouchPoints[event.touchID] = Math::Vector2(event.positionX, event.positionY);
}

void OrbitController::removeTrackedTouchPoint(int32_t touchID) {
    m_trackedTouchPoints.erase(touchID);
}

bool OrbitController::canParticipateInPinch(const TouchEvent& event) const {
    return isDirectManipulationDevice(event.deviceType) && event.touchID >= 0;
}

bool OrbitController::canStartPinch() const {
    return m_zoomEnabled && m_trackedTouchPoints.size() >= 2;
}

void OrbitController::beginPinch() {
    auto first = m_trackedTouchPoints.begin();
    auto second = first;
    ++second;
    m_pinchTouchID0 = first->first;
    m_pinchTouchID1 = second->first;

    const float pinchDistance = calculatePinchDistance(m_trackedTouchPoints, m_pinchTouchID0, m_pinchTouchID1);
    if (pinchDistance < kMinPinchDistance) {
        m_pinchTouchID0 = -1;
        m_pinchTouchID1 = -1;
        return;
    }

    m_isPinching = true;
    m_isDragging = false;
    m_activeTouchID = -1;
    m_activeDeviceType = TOUCH_DEVICE_TYPE_UNKNOWN;
    m_activeButton = TOUCH_MOUSE_BUTTON_NONE;
    m_lastPinchDistance = pinchDistance;
}

void OrbitController::updatePinch() {
    if (!m_isPinching || !m_zoomEnabled || m_trackedTouchPoints.size() < 2) {
        return;
    }

    const float pinchDistance = calculatePinchDistance(m_trackedTouchPoints, m_pinchTouchID0, m_pinchTouchID1);
    if (pinchDistance < kMinPinchDistance) {
        return;
    }

    if (m_lastPinchDistance < kMinPinchDistance) {
        m_lastPinchDistance = pinchDistance;
        return;
    }

    const float distanceRatio = pinchDistance / m_lastPinchDistance;
    if (std::fabs(distanceRatio - 1.0f) > 1e-4f) {
        const float wheelDeltaY = std::log(distanceRatio) / std::max(m_zoomSpeed, kMinZoomSpeed);
        applyZoom(wheelDeltaY);
    }
    m_lastPinchDistance = pinchDistance;
}

void OrbitController::endPinch() {
    m_isPinching = false;
    m_pinchTouchID0 = -1;
    m_pinchTouchID1 = -1;
    m_lastPinchDistance = 0.0f;
}

void OrbitController::resumeDragFromRemainingTouch() {
    if (!m_rotateEnabled || m_isPinching || m_trackedTouchPoints.size() != 1) {
        return;
    }

    const auto& remainingTouch = *m_trackedTouchPoints.begin();
    m_isDragging = true;
    m_activeTouchID = remainingTouch.first;
    m_activeDeviceType = TOUCH_DEVICE_TYPE_TOUCH;
    m_activeButton = TOUCH_MOUSE_BUTTON_NONE;
    m_lastPointer = remainingTouch.second;
}

bool OrbitController::isEventInsideView(const TouchEvent& event, const FrameStateSharedPtr& /*frameState*/) const {
    if (!m_view) {
        return false;
    }

    return m_view->getScreenSpaceAABB().Contains(event.positionX, event.positionY);
}

void OrbitController::handleEvent(const TouchEvent& event, const FrameStateSharedPtr& frameState) {
    switch (event.eventType) {
        case TOUCH_EVENT_TYPE_TOUCH:
            updateTrackedTouchPoint(event, true, frameState);
            if (canStartPinch()) {
                beginPinch();
                break;
            }
            if (!m_isDragging && m_rotateEnabled && canStartRotate(event) && isEventInsideView(event, frameState)) {
                m_isDragging = true;
                m_activeTouchID = event.touchID;
                m_activeDeviceType = event.deviceType;
                m_activeButton = event.button;
                m_lastPointer.set(event.positionX, event.positionY);
            }
            break;

        case TOUCH_EVENT_TYPE_MOVE:
            updateTrackedTouchPoint(event, false, frameState);
            if (m_isPinching) {
                updatePinch();
                break;
            }
            if (m_isDragging && matchesActivePointer(event, m_activeTouchID, m_activeButton, m_activeDeviceType)) {
                const Math::Vector2 currentPointer(event.positionX, event.positionY);
                const Math::Vector2 delta = currentPointer - m_lastPointer;
                if (m_rotateEnabled && shouldRotateForActivePointer(m_activeButton, m_activeDeviceType) && delta.lengthSq() > 0.0f) {
                    applyRotate(delta);
                }
                m_lastPointer = currentPointer;
            }
            break;

        case TOUCH_EVENT_TYPE_RELEASE:
            removeTrackedTouchPoint(event.touchID);
            if (m_isPinching) {
                endPinch();
                resumeDragFromRemainingTouch();
                break;
            }
            if (m_isDragging && event.touchID == m_activeTouchID) {
                m_isDragging = false;
                m_activeTouchID = -1;
                m_activeDeviceType = TOUCH_DEVICE_TYPE_UNKNOWN;
                m_activeButton = TOUCH_MOUSE_BUTTON_NONE;
                if (!m_trackedTouchPoints.empty()) {
                    resumeDragFromRemainingTouch();
                } else {
                    m_lastPointer.set(0.0f, 0.0f);
                }
            }
            break;

        case TOUCH_EVENT_TYPE_WHEEL:
            if (m_zoomEnabled && isEventInsideView(event, frameState) && std::fabs(event.wheelDeltaY) > 1e-6f) {
                applyZoom(event.wheelDeltaY);
            }
            break;

        default:
            break;
    }
}

void OrbitController::applyRotate(const Math::Vector2& delta) {
    if (!m_camera) {
        return;
    }

    // Horizontal drag keeps the existing orbit convention.
    // Vertical drag uses direct-manipulation style: dragging up makes the
    // model appear to rotate up, so the camera pitch delta is inverted here.
    m_camera->orbit(-delta.x * m_rotateSpeed, delta.y * m_rotateSpeed);
}

void OrbitController::applyZoom(float wheelDeltaY) {
    if (!m_camera) {
        return;
    }

    const Math::Vector3 offset = m_camera->getPosition() - m_camera->getTarget();
    const float currentDistance = std::max(offset.length(), 0.001f);
    const float zoomScale = std::exp(-wheelDeltaY * m_zoomSpeed);
    const float nextDistance = std::clamp(currentDistance * zoomScale, m_minDistance, m_maxDistance);
    m_camera->setDistance(nextDistance);
}
} // namespace morrow
