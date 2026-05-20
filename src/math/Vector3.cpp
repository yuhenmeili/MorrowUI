//
// Created by lance on 2022/10/23.
//

#include <cstdio>
#include "Vector3.h"

namespace morrow
{
namespace Math
{

Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f)
{}

Vector3::Vector3(const Vector2& vector2) : x(vector2.x), y(vector2.y), z(0.0f)
{}

Vector3::Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
{}

Vector3::Vector3(float scalar) : x(scalar), y(scalar), z(scalar)
{}

Vector3::Vector3(const Vector3& v) : x(v.x), y(v.y), z(v.z)
{}

Vector3& Vector3::apply(const Matrix4& m)
{
    float lastX = x, lastY = y, lastZ = z;
    const float* e = m.elements;
    float w = 1.0f / (e[3] * lastX + e[7] * lastY + e[11] * lastZ + e[15]);
    x = (e[0] * lastX + e[4] * lastY + e[8] * lastZ + e[12]) * w;
    y = (e[1] * lastX + e[5] * lastY + e[9] * lastZ + e[13]) * w;
    z = (e[2] * lastX + e[6] * lastY + e[10] * lastZ + e[14]) * w;
    return *this;
}

//apply quaternion
Vector3& Vector3::apply(const Quaternion& q)
{
    float lastX = x, lastY = y, lastZ = z;
    float qx = q.x, qy = q.y, qz = q.z, qw = q.w;

    // calculate quat * vector

    float ix = qw * lastX + qy * lastZ - qz * lastY;
    float iy = qw * lastY + qz * lastX - qx * lastZ;
    float iz = qw * lastZ + qx * lastY - qy * lastX;
    float iw = -qx * lastX - qy * lastY - qz * lastZ;

    // calculate result * inverse quat

    x = ix * qw + iw * -qx + iy * -qz - iz * -qy;
    y = iy * qw + iw * -qy + iz * -qx - ix * -qz;
    z = iz * qw + iw * -qz + ix * -qy - iy * -qx;

    return *this;
}

Vector3& Vector3::apply(const Vector3& axis, float angle)
{
    return apply(Quaternion::fromAxisAngle(axis, angle));
}

//Vector3& Vector3::project(const Camera& camera)
//{
//    apply(camera.projection * camera.matrixWorld.inverted());
//    return *this;
//}
//
//Vector3 Vector3::project(const Camera& camera) const
//{
//    Vector3 ret(*this);
//    ret.apply(camera.projection * camera.matrixWorld.inverted());
//    return ret;
//}
//
//Vector3& Vector3::unproject(const Camera& camera)
//{
//    apply(camera.matrixWorld * camera.projection.inverted());
//    return *this;
//}
//
//Vector3 Vector3::unproject(const Camera& camera) const
//{
//    Vector3 ret(*this);
//    ret.apply(camera.matrixWorld * camera.projection.inverted());
//    return ret;
//}

Vector3& Vector3::unproject(const Matrix4& world, const Matrix4& projection)
{
    apply(world * projection.inverted());
    return *this;
}

Vector3& Vector3::transformDirection(const Matrix4& m)
{
    float lastX = x, lastY = y, lastZ = z;
    const float* e = m.elements;

    x = e[0] * lastX + e[4] * lastY + e[8] * lastZ;
    y = e[1] * lastX + e[5] * lastY + e[9] * lastZ;
    z = e[2] * lastX + e[6] * lastY + e[10] * lastZ;

    normalize();
    return *this;
}

Vector3 Vector3::fromMatrixColumn(const Matrix4& m, uint32_t index)
{
    return fromArray(m.elements, index * 4);
}

Vector3 Vector3::fromMatrixScale(const Matrix4& m)
{
    float sx = fromMatrixColumn(m, 0).length();
    float sy = fromMatrixColumn(m, 1).length();
    float sz = fromMatrixColumn(m, 2).length();

    return Vector3(sx, sy, sz);
}

Vector3& Vector3::apply(const Matrix3& m)
{
    const float* e = m.elements;

    x = e[0] * x + e[3] * y + e[6] * z;
    y = e[1] * x + e[4] * y + e[7] * z;
    z = e[2] * x + e[5] * y + e[8] * z;

    return *this;
}

Vector3& Vector3::set(float _x, float _y, float _z)
{
    x = _x;
    y = _y;
    z = _z;
    return *this;
}

float Vector3::operator[](uint32_t index) const
{
    return elements[index];
}

float& Vector3::operator[](uint32_t index)
{
    return elements[index];
}

bool Vector3::operator!()
{
    return x == 0.0f && y == 0.0f && z == 0.0f;
}

Vector3::const_iterator Vector3::cbegin() const
{
    return &elements[0];
}

Vector3::const_iterator Vector3::cend() const
{
    return &elements[3];
}

float* Vector3::end()
{
    return std::end(elements);
}

Vector3& Vector3::operator=(float scalar)
{
    x = y = z = scalar;
    return *this;
}

Vector3& Vector3::operator+=(const Vector3& vector)
{
    x += vector.x;
    y += vector.y;
    z += vector.z;

    return *this;
}

Vector3& Vector3::operator+=(float scalar)
{
    x += scalar;
    y += scalar;
    z += scalar;

    return *this;
}

Vector3& Vector3::operator-=(const Vector3& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;

    return *this;
}

Vector3& Vector3::operator-=(float scalar)
{
    x -= scalar;
    y -= scalar;
    z -= scalar;

    return *this;
}

Vector3& Vector3::operator*=(const Vector3& v)
{
    x *= v.x;
    y *= v.y;
    z *= v.z;

    return *this;
}

Vector3& Vector3::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;
}

Vector3& Vector3::operator/=(const Vector3& v)
{
    x /= v.x;
    y /= v.y;
    z /= v.z;

    return *this;
}

Vector3& Vector3::operator/=(float scalar)
{
    return *this *= (1.0f / scalar);
}

Vector3& Vector3::min(const Vector3& v)
{
    x = std::min(x, v.x);
    y = std::min(y, v.y);
    z = std::min(z, v.z);

    return *this;
}

Vector3& Vector3::max(const Vector3& v)
{
    x = std::max(x, v.x);
    y = std::max(y, v.y);
    z = std::max(z, v.z);

    return *this;
}

Vector3& Vector3::clamp(const Vector3& min, const Vector3& max)
{
    // assumes min < max, componentwise
    x = std::max(min.x, std::min(max.x, x));
    y = std::max(min.y, std::min(max.y, y));
    z = std::max(min.z, std::min(max.z, z));

    return *this;
}

Vector3& Vector3::clamp(float minVal, float maxVal)
{
    Vector3 min(minVal, minVal, minVal);
    Vector3 max(maxVal, maxVal, maxVal);

    return this->clamp(min, max);
}

Vector3& Vector3::clampLength(float min, float max)
{
    float len = length();

    float div = (len > 0.0f ? len : 1.0f) * std::max(min, std::min(max, len));
    return *this /= div;
}

Vector3& Vector3::floor()
{
    x = std::floor(x);
    y = std::floor(y);
    z = std::floor(z);

    return *this;
}

Vector3& Vector3::ceil()
{
    x = std::ceil(x);
    y = std::ceil(y);
    z = std::ceil(z);

    return *this;
}

Vector3& Vector3::round()
{
    x = std::round(x);
    y = std::round(y);
    z = std::round(z);

    return *this;
}

Vector3& Vector3::roundToZero()
{
    x = x < 0.0f ? std::ceil(x) : std::floor(x);
    y = y < 0.0f ? std::ceil(y) : std::floor(y);
    z = z < 0.0f ? std::ceil(z) : std::floor(z);

    return *this;
}

Vector3& Vector3::negate()
{
    x = -x;
    y = -y;
    z = -z;
    return *this;
}

Vector3 Vector3::negated() const
{
    return {-x, -y, -z};
}

float Vector3::lengthSq() const
{
    return x * x + y * y + z * z;
}

float Vector3::length() const
{
    return std::sqrt(x * x + y * y + z * z);
}

float Vector3::manhattanLength() const
{
    return std::abs(x) + std::abs(y) + std::abs(z);
}

Vector3& Vector3::normalize()
{
    float l = length();
    if (l) {
        *this /= l;
    }
    return *this;
}

Vector3 Vector3::normalized() const
{
    float len = length();
    return *this / (len > 0.0f ? len : 1.0f);
}

Vector3& Vector3::setLength(float length)
{
    return normalize() *= length;
}

Vector3& Vector3::lerp(const Vector3& v, float alpha)
{
    x += (v.x - x) * alpha;
    y += (v.y - y) * alpha;
    z += (v.z - z) * alpha;

    return *this;
}

Vector3 Vector3::lerped(const Vector3& v, float alpha) const
{
    float _x = x + (v.x - x) * alpha;
    float _y = y + (v.y - y) * alpha;
    float _z = z + (v.z - z) * alpha;

    return {_x, _y, _z};
}

Vector3& Vector3::lerpVectors(const Vector3& v1, const Vector3& v2, float alpha)
{
    *this = (v2 - v1) * alpha + v1;
    return *this;
}

Vector3& Vector3::project(const Vector3& vector)
{
    float scalar = dot(vector, *this) / vector.lengthSq();
    *this = vector * scalar;

    return *this;
}

Vector3& Vector3::projectOnPlane(const Vector3& planeNormal)
{
    Vector3 v1(*this);
    v1.project(planeNormal);

    return *this -= v1;
}

Vector3& Vector3::reflect(const Vector3& normal)
{
    const Vector3& v1(normal);
    return *this -= (v1 * (2.0f * dot(*this, normal)));
}

float Vector3::angleTo(const Vector3& v) const
{
    float theta = dot(*this, v) / (std::sqrt(lengthSq() * v.lengthSq()));

    // clamp, to handle numerical problems
    return std::acos(Math::clamp(theta, -1.0f, 1.0f));
}

float Vector3::distanceToSquared(const Vector3& v) const
{
    float dx = x - v.x, dy = y - v.y, dz = z - v.z;

    return dx * dx + dy * dy + dz * dz;
}

float Vector3::distanceTo(const Vector3& v) const
{
    return std::sqrt(distanceToSquared(v));
}

float Vector3::manhattanDistance(const Vector3& v) const
{
    return std::abs(x - v.x) + std::abs(y - v.y) + std::abs(z - v.z);
}

float Vector3::azimuth() const
{
    return std::atan2(z, -x);
}

float Vector3::inclination() const
{
    return std::atan2(-y, std::sqrt((x * x) + (z * z)));
}

bool Vector3::isNull() const
{
    return x == 0.0f && y == 0.0f && z == 0.0f;
}

bool Vector3::operator==(const Vector3& v) const
{
    return ((v.x == x) && (v.y == y) && (v.z == z));
}

bool Vector3::operator!=(const Vector3& v) const
{
    return !operator==(v);
}

Vector3& Vector3::subVectors(Vector3& a, Vector3& b)
{
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
    return *this;
}

Vector3& Vector3::crossVectors(const Vector3& a, const Vector3& b)
{
    float ax = a.x, ay = a.y, az = a.z;
    float bx = b.x, by = b.y, bz = b.z;

    x = ay * bz - az * by;
    y = az * bx - ax * bz;
    z = ax * by - ay * bx;
    return *this;
}

Vector3& Vector3::divideScalar(float scalar)
{
    return multiplyScalar(1.0f / scalar);
}

Vector3& Vector3::multiplyScalar(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::add(Vector3& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

Vector3& Vector3::copy(Vector3& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
}

Vector3& Vector3::sub(Vector3& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

Vector3& Vector3::cross(const Vector3& v)
{
    return crossVectors(*this, v);
}

Vector3& Vector3::applyMatrix4(const Matrix4& m)
{
    const float _x = x, _y = y, _z = z;
    const float* e = m.elements;
    const float w = 1 / (e[3] * _x + e[7] * _y + e[11] * _z + e[15]);

    x = (e[0] * _x + e[4] * _y + e[8] * _z + e[12]) * w;
    y = (e[1] * _x + e[5] * _y + e[9] * _z + e[13]) * w;
    z = (e[2] * _x + e[6] * _y + e[10] * _z + e[14]) * w;

    return *this;
}
}
}
