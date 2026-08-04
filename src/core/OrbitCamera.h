#ifndef MORROW_GUI_ORBIT_CAMERA_H
#define MORROW_GUI_ORBIT_CAMERA_H

#include <functional>

#include "PerspectiveCamera.h"

namespace morrow {
// Orbit camera tailored for model viewing: keeps a target point and updates
// position/direction from yaw/pitch/distance each frame.
class OrbitCamera : public PerspectiveCamera {
public:
    explicit OrbitCamera(float fov = 45.0f, float aspect = 1.0f, float near = 0.1f, float far = 2000.0f);

    void update(float x, float y, float displayWidth, float displayHeight) override;

    void setPosition(float x, float y, float z);

    void setPosition(const Vector3& position);

    void setTarget(float x, float y, float z);

    void setTarget(const Vector3& target);

    // Look at a world-space point while keeping current camera position.
    void lookAt(float x, float y, float z);

    void lookAt(const Vector3& target);

    // Orbit angles in radians.
    void setAngles(float yaw, float pitch);

    void orbit(float deltaYaw, float deltaPitch);

    // Distance from camera to target.
    void setDistance(float distance);

    void dolly(float deltaDistance);

    [[nodiscard]] const Vector3& getTarget() const;

    void setChangeCallback(std::function<void()> callback);

private:
    void syncSphericalFromPosition();

    void updatePositionFromSpherical();

    void notifyChanged();

    Vector3 m_target = {0.0f, 0.0f, 0.0f};
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_distance = 5.0f;
    bool m_sphericalDirty = true;
    std::function<void()> m_changeCallback;
};
} // namespace morrow

#endif // MORROW_GUI_ORBIT_CAMERA_H
