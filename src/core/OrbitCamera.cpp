#include "OrbitCamera.h"

#include <algorithm>
#include <cmath>

namespace morrow {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinPitch = -1.55334306f; // about -89 deg
constexpr float kMaxPitch = 1.55334306f;  // about  89 deg
constexpr float kMinDistance = 0.01f;
}

OrbitCamera::OrbitCamera(float fov, float aspect, float near, float far)
    : PerspectiveCamera(fov, aspect, near, far) {
    syncSphericalFromPosition();
}

void OrbitCamera::update(float x, float y, float displayWidth, float displayHeight) {
    if (m_sphericalDirty) {
        updatePositionFromSpherical();
        m_sphericalDirty = false;
    }

    Vector3 dir;
    dir.set(m_target.x - m_position.x, m_target.y - m_position.y, m_target.z - m_position.z);
    if (dir.lengthSq() > 1e-8f) {
        dir.normalize();
        setDirection(dir);
    }

    PerspectiveCamera::update(x, y, displayWidth, displayHeight);
}

void OrbitCamera::setPosition(float x, float y, float z) {
    PerspectiveCamera::setPosition(x, y, z);
    syncSphericalFromPosition();
}

void OrbitCamera::setPosition(const Vector3& position) {
    PerspectiveCamera::setPosition(position.x, position.y, position.z);
    syncSphericalFromPosition();
}

void OrbitCamera::setTarget(float x, float y, float z) {
    m_target.set(x, y, z);
    syncSphericalFromPosition();
}

void OrbitCamera::setTarget(const Vector3& target) {
    m_target.set(target.x, target.y, target.z);
    syncSphericalFromPosition();
}

void OrbitCamera::lookAt(float x, float y, float z) {
    setTarget(x, y, z);
}

void OrbitCamera::lookAt(const Vector3& target) {
    setTarget(target);
}

void OrbitCamera::setAngles(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = std::max(kMinPitch, std::min(kMaxPitch, pitch));
    m_sphericalDirty = true;
}

void OrbitCamera::orbit(float deltaYaw, float deltaPitch) {
    setAngles(m_yaw + deltaYaw, m_pitch + deltaPitch);
}

void OrbitCamera::setDistance(float distance) {
    m_distance = std::max(kMinDistance, distance);
    m_sphericalDirty = true;
}

void OrbitCamera::dolly(float deltaDistance) {
    setDistance(m_distance + deltaDistance);
}

const Vector3& OrbitCamera::getTarget() const {
    return m_target;
}

void OrbitCamera::syncSphericalFromPosition() {
    const float dx = m_position.x - m_target.x;
    const float dy = m_position.y - m_target.y;
    const float dz = m_position.z - m_target.z;

    m_distance = std::max(kMinDistance, std::sqrt(dx * dx + dy * dy + dz * dz));

    const float xzLen = std::sqrt(dx * dx + dz * dz);
    m_pitch = std::max(kMinPitch, std::min(kMaxPitch, std::atan2(dy, xzLen)));
    m_yaw = std::atan2(dx, dz);
    if (m_yaw > kPi) m_yaw -= 2.0f * kPi;
    if (m_yaw < -kPi) m_yaw += 2.0f * kPi;

    m_sphericalDirty = true;
}

void OrbitCamera::updatePositionFromSpherical() {
    const float cosPitch = std::cos(m_pitch);
    const float sinPitch = std::sin(m_pitch);
    const float sinYaw = std::sin(m_yaw);
    const float cosYaw = std::cos(m_yaw);

    m_position.set(
        m_target.x + m_distance * sinYaw * cosPitch,
        m_target.y + m_distance * sinPitch,
        m_target.z + m_distance * cosYaw * cosPitch);
}

} // namespace morrow

