//
// Created by lance on 2022/10/31.
//

#include <cstdio>
#include "Vector4.h"

namespace morrow
{
namespace Math
{
Vector4::Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
{}

Vector4::Vector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w)
{}

Vector4::Vector4(const Vector4& vector)
{
    memcpy(elements, vector.elements, sizeof(elements));
}

Vector4::Vector4(const float* array, uint32_t offset)
{
    memcpy(elements, array, sizeof(elements));
}

bool Vector4::isNull() const
{
    return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f;
}

void Vector4::setNull()
{
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    w = 0.0f;
}

Vector4& Vector4::set(float _x, float _y, float _z, float _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;

    return *this;
}

Vector4& Vector4::setScalar(float scalar)
{
    x = scalar;
    y = scalar;
    z = scalar;
    w = scalar;

    return *this;
}

Vector4& Vector4::copy(const Vector4& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
    return *this;
}

Vector4& Vector4::set(uint32_t index, float value)
{
    elements[index] = value;
    return *this;
}

float Vector4::get(uint32_t index) const
{
    return elements[index];
}

float Vector4::dot(const Vector4& v) const
{
    return x * v.x + y * v.y + z * v.z + w * v.w;
}

float Vector4::lengthSq() const
{
    return x * x + y * y + z * z + w * w;
}

float Vector4::length() const
{
    return std::sqrt(x * x + y * y + z * z + w * w);
}

float Vector4::manhattanLength() const
{
    return std::abs(x) + std::abs(y) + std::abs(z) + std::abs(w);
}

Vector4& Vector4::add(const Vector4& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;

    return *this;
}

Vector4& Vector4::addScalar(float s)
{
    x += s;
    y += s;
    z += s;
    w += s;

    return *this;
}

Vector4& Vector4::addVectors(const Vector4& a, const Vector4& b)
{
    x = a.x + b.x;
    y = a.y + b.y;
    z = a.z + b.z;
    w = a.w + b.w;

    return *this;
}

Vector4& Vector4::addScaledVector(const Vector4& v, float s)
{
    x += v.x * s;
    y += v.y * s;
    z += v.z * s;
    w += v.w * s;

    return *this;
}

Vector4& Vector4::sub(const Vector4& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;

    return *this;
}

Vector4& Vector4::sub(const Vector4& a, const Vector4& b)
{
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
    w = a.w - b.w;

    return *this;
}

Vector4& Vector4::sub(float s)
{
    x -= s;
    y -= s;
    z -= s;
    w -= s;

    return *this;
}

Vector4& Vector4::multiply(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;

    return *this;
}

Vector4& Vector4::divide(float scalar)
{
    return multiply(1 / scalar);
}

Vector4& Vector4::min(const Vector4& v)
{
    x = std::min(x, v.x);
    y = std::min(y, v.y);
    z = std::min(z, v.z);
    w = std::min(w, v.w);

    return *this;
}

Vector4& Vector4::max(const Vector4& v)
{
    x = std::max(x, v.x);
    y = std::max(y, v.y);
    z = std::max(z, v.z);
    w = std::max(w, v.w);

    return *this;
}

Vector4& Vector4::clamp(const Vector4& min, const Vector4& max)
{
    // assumes min < max, componentwise

    x = std::max(min.x, std::min(max.x, x));
    y = std::max(min.y, std::min(max.y, y));
    z = std::max(min.z, std::min(max.z, z));
    w = std::max(min.w, std::min(max.w, w));

    return *this;
}

Vector4& Vector4::clampScalar(float minVal, float maxVal)
{
    Vector4 min(minVal, minVal, minVal, minVal);
    Vector4 max(maxVal, maxVal, maxVal, maxVal);

    return clamp(min, max);
}

Vector4& Vector4::clampLength(float min, float max)
{
    float len = length();

    return divide(len > 0.0f ? len : 1.0f).multiply(std::max(min, std::min(max, len)));
}

Vector4& Vector4::floor()
{
    x = std::floor(x);
    y = std::floor(y);
    z = std::floor(z);
    w = std::floor(w);

    return *this;
}

Vector4& Vector4::ceil()
{
    x = std::ceil(x);
    y = std::ceil(y);
    z = std::ceil(z);
    w = std::ceil(w);

    return *this;
}

Vector4& Vector4::round()
{
    x = std::round(x);
    y = std::round(y);
    z = std::round(z);
    w = std::round(w);

    return *this;
}

Vector4& Vector4::roundToZero()
{
    x = (x < 0.0f) ? std::ceil(x) : std::floor(x);
    y = (y < 0.0f) ? std::ceil(y) : std::floor(y);
    z = (z < 0.0f) ? std::ceil(z) : std::floor(z);
    w = (w < 0.0f) ? std::ceil(w) : std::floor(w);

    return *this;
}

Vector4& Vector4::negate()
{
    x = -x;
    y = -y;
    z = -z;
    w = -w;

    return *this;
}

Vector4& Vector4::normalize()
{
    float len = length();
    return divide(len > 0.0f ? len : 1.0f);
}

Vector4& Vector4::setLength(float length)
{
    return normalize().multiply(length);
}

Vector4& Vector4::lerp(const Vector4& v, float alpha)
{
    x += (v.x - x) * alpha;
    y += (v.y - y) * alpha;
    z += (v.z - z) * alpha;
    w += (v.w - w) * alpha;

    return *this;
}

Vector4& Vector4::lerp(const Vector4& v1, const Vector4& v2, float alpha)
{
    return sub(v2, v1).multiply(alpha).add(v1);
}

bool Vector4::operator==(const Vector4& v) const
{
    return ((v.x == x) && (v.y == y) && (v.z == z) && (v.w == w));
}

bool Vector4::operator!=(const Vector4& v) const
{
    return !((*this) == v);
}

void Vector4::writeTo(float* array, uint32_t offset)
{
    memcpy(array + offset, elements, sizeof(elements));
}

const Vector4 Vector4::ZERO(0.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::ONE(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4 Vector4::UNIT_X(1.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::UNIT_Y(0.0f, 1.0f, 0.0f, 0.0f);
const Vector4 Vector4::UNIT_Z(0.0f, 0.0f, 1.0f, 0.0f);
const Vector4 Vector4::UNIT_W(0.0f, 0.0f, 0.0f, 1.0f);

Vector4& Vector4::operator*=(const Matrix4& m)
{
    float lastX = x, lastY = y, lastZ = z, lastW = w;
    const float* e = m.elements;

    x = e[0] * lastX + e[4] * lastY + e[8] * lastZ + e[12] * lastW;
    y = e[1] * lastX + e[5] * lastY + e[9] * lastZ + e[13] * lastW;
    z = e[2] * lastX + e[6] * lastY + e[10] * lastZ + e[14] * lastW;
    w = e[3] * lastX + e[7] * lastY + e[11] * lastZ + e[15] * lastW;

    return *this;
}

Vector4& Vector4::setAxisAngleFromQuaternion(const Quaternion& q)
{
    w = 2.0f * std::acos(q.w);

    float s = std::sqrt(1.0f - q.w * q.w);

    if (s < 0.0001f) {
        x = 1.0f;
        y = 0.0f;
        z = 0.0f;
    } else {
        x = q.x / s;
        y = q.y / s;
        z = q.z / s;
    }

    return *this;
}

Vector4& Vector4::setAxisAngleFromRotationMatrix(const Matrix4& m)
{
    const float* te = m.elements;

    float angle, _x, _y, _z,        // variables for result
    epsilon = 0.01f,        // margin to allow for rounding errors
    epsilon2 = 0.1f,        // margin to distinguish between 0 and 180 degrees

    m11 = te[0], m12 = te[4], m13 = te[8],
        m21 = te[1], m22 = te[5], m23 = te[9],
        m31 = te[2], m32 = te[6], m33 = te[10];

    if ((std::abs(m12 - m21) < epsilon) &&
        (std::abs(m13 - m31) < epsilon) &&
        (std::abs(m23 - m32) < epsilon)) {

        // singularity found
        // first check for identity matrix which must have +1 for all terms
        // in leading diagonal and zero in other terms

        if ((std::abs(m12 + m21) < epsilon2) &&
            (std::abs(m13 + m31) < epsilon2) &&
            (std::abs(m23 + m32) < epsilon2) &&
            (std::abs(m11 + m22 + m33 - 3.0f) < epsilon2)) {

            // this singularity is identity matrix so angle = 0

            set(1.0f, 0.0f, 0.0f, 0.0f);

            return *this; // zero angle, arbitrary axis
        }

        // otherwise this singularity is angle = 180

        angle = MR_PI;

        float xx = (m11 + 1.0f) / 2.0f;
        float yy = (m22 + 1.0f) / 2.0f;
        float zz = (m33 + 1.0f) / 2.0f;
        float xy = (m12 + m21) / 4.0f;
        float xz = (m13 + m31) / 4.0f;
        float yz = (m23 + m32) / 4.0f;

        if ((xx > yy) && (xx > zz)) {

            // m11 is the largest diagonal term

            if (xx < epsilon) {

                _x = 0.0f;
                _y = 0.707106781f;
                _z = 0.707106781f;

            } else {

                _x = std::sqrt(xx);
                _y = xy / _x;
                _z = xz / _x;

            }

        } else if (yy > zz) {

            // m22 is the largest diagonal term

            if (yy < epsilon) {

                _x = 0.707106781f;
                _y = 0.0f;
                _z = 0.707106781f;

            } else {

                _y = std::sqrt(yy);
                _x = xy / _y;
                _z = yz / _y;

            }

        } else {

            // m33 is the largest diagonal term so base result on this

            if (zz < epsilon) {

                _x = 0.707106781f;
                _y = 0.707106781f;
                _z = 0.0f;

            } else {

                _z = std::sqrt(zz);
                _x = xz / _z;
                _y = yz / _z;

            }

        }

        set(_x, _y, _z, angle);

        return *this; // return 180 deg rotation
    }

    // as we have reached here there are no singularities so we can handle normally

    float s = std::sqrt((m32 - m23) * (m32 - m23) +
                        (m13 - m31) * (m13 - m31) +
                        (m21 - m12) * (m21 - m12)); // used to normalize

    if (std::abs(s) < 0.001f) {
        s = 1.0f;
    }

    // prevent divide by zero, should not happen if matrix is orthogonal and should be
    // caught by singularity test above, but I've left it in just in case

    x = (m32 - m23) / s;
    y = (m13 - m31) / s;
    z = (m21 - m12) / s;
    w = std::acos((m11 + m22 + m33 - 1.0f) / 2.0f);

    return *this;
}
}
}