#ifndef MORROW_GUI_ORBIT_CONTROLLER_H
#define MORROW_GUI_ORBIT_CONTROLLER_H

#include <memory>
#include <unordered_map>

#include "FrameState.h"
#include "Vector2.h"
#include "base/TouchEvent.h"

namespace morrow {
class MR3DSceneView;
class OrbitCamera;
class UIWidget;
struct TouchEvent;

class OrbitController {
public:
    OrbitController(std::shared_ptr<MR3DSceneView> view);

    void update(const FrameStateSharedPtr& frameState);

    void setEnabled(bool enabled);

    bool isEnabled() const;

    void setRotateEnabled(bool enabled);

    bool isRotateEnabled() const;

    void setZoomEnabled(bool enabled);

    bool isZoomEnabled() const;

    void setRotateSpeed(float speed);

    float getRotateSpeed() const;

    void setZoomSpeed(float speed);

    float getZoomSpeed() const;

    void setMinDistance(float distance);

    float getMinDistance() const;

    void setMaxDistance(float distance);

    float getMaxDistance() const;

    void reset();

private:
    bool isEventInsideView(const TouchEvent& event, const FrameStateSharedPtr& frameState) const;

    void handleEvent(const TouchEvent& event, const FrameStateSharedPtr& frameState);

    void updateTrackedTouchPoint(const TouchEvent& event, bool requireInsideView, const FrameStateSharedPtr& frameState);

    void removeTrackedTouchPoint(int32_t touchID);

    bool canParticipateInPinch(const TouchEvent& event) const;

    bool canStartPinch() const;

    void beginPinch();

    void updatePinch();

    void endPinch();

    void resumeDragFromRemainingTouch();

    void applyRotate(const Math::Vector2& delta);

    void applyZoom(float wheelDeltaY);

private:
    std::shared_ptr<MR3DSceneView> m_view = nullptr;
    OrbitCamera* m_camera = nullptr;
    bool m_enabled = true;
    bool m_rotateEnabled = true;
    bool m_zoomEnabled = true;
    float m_rotateSpeed = 0.008f;
    float m_zoomSpeed = 0.18f;
    float m_minDistance = 0.1f;
    float m_maxDistance = 2000.0f;
    bool m_isDragging = false;
    bool m_isPinching = false;
    int32_t m_activeTouchID = -1;
    int32_t m_pinchTouchID0 = -1;
    int32_t m_pinchTouchID1 = -1;
    TouchDeviceType m_activeDeviceType = TOUCH_DEVICE_TYPE_UNKNOWN;
    TouchMouseButton m_activeButton = TOUCH_MOUSE_BUTTON_NONE;
    float m_lastPinchDistance = 0.0f;
    Math::Vector2 m_lastPointer = {0.0f, 0.0f};
    std::unordered_map<int32_t, Math::Vector2> m_trackedTouchPoints;
};

using OrbitControllerSharedPtr = std::shared_ptr<OrbitController>;
} // namespace morrow

#endif // MORROW_GUI_ORBIT_CONTROLLER_H
