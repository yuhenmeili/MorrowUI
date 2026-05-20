//
// Created by lance on 2022/10/24.
//

#include "Quaternion.h"

namespace morrow
{
namespace Math
{
Quaternion::Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w)
{}

Quaternion::Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
{}

Quaternion::Quaternion(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar)
{}

Quaternion::Quaternion(const Quaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w)
{}

Quaternion::Quaternion(const Vector3& axis, float angle)
{
    set(axis, angle);
}

Quaternion& Quaternion::set(const Vector3& axis, float angle)
{
    float halfAngle = angle / 2.0f, s = std::sin(halfAngle);

    x = axis.x * s;
    y = axis.y * s;
    z = axis.z * s;
    w = std::cos(halfAngle);

    return *this;
}

Quaternion& Quaternion::set(float _x, float _y, float _z, float _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;
    return *this;
}

Quaternion::Quaternion(const Matrix4& m)
{
    set(m);
}

Quaternion& Quaternion::set(const Matrix4& m)
{
    const float* te = m.elements,

            m11 = te[0], m12 = te[4], m13 = te[8],
            m21 = te[1], m22 = te[5], m23 = te[9],
            m31 = te[2], m32 = te[6], m33 = te[10],

            trace = m11 + m22 + m33;

    if (trace > 0) {

        float s = 0.5f / std::sqrt(trace + 1.0f);

        w = 0.25f / s;
        x = (m32 - m23) * s;
        y = (m13 - m31) * s;
        z = (m21 - m12) * s;

    } else if (m11 > m22 && m11 > m33) {

        float s = 2.0f * std::sqrt(1.0f + m11 - m22 - m33);

        w = (m32 - m23) / s;
        x = 0.25f * s;
        y = (m12 + m21) / s;
        z = (m13 + m31) / s;

    } else if (m22 > m33) {

        float s = 2.0f * std::sqrt(1.0f + m22 - m11 - m33);

        w = (m13 - m31) / s;
        x = (m12 + m21) / s;
        y = 0.25f * s;
        z = (m23 + m32) / s;

    } else {

        float s = 2.0f * std::sqrt(1.0f + m33 - m11 - m22);

        w = (m21 - m12) / s;
        x = (m13 + m31) / s;
        y = (m23 + m32) / s;
        z = 0.25f * s;
    }
    return *this;
}

Quaternion& Quaternion::setFromUnitVectors(const Vector3& vFrom, const Vector3& vTo)
{
    // assumes direction vectors vFrom and vTo are normalized
    Vector3 v1;

    float r = Math::dot(vFrom, vTo) + 1;

    if (r < EPS) {
        r = 0;

        if (std::abs(vFrom.x) > std::abs(vFrom.z)) {
            v1.set(-vFrom.y, vFrom.x, 0);
        } else {
            v1.set(0, -vFrom.z, vFrom.y);
        }
    } else {
        v1 = cross(vFrom, vTo);
    }

    x = v1.x;
    y = v1.y;
    z = v1.z;
    w = r;

    return normalize();
}

Quaternion& Quaternion::setFromAxisAngle(const Vector3& axis, float angle)
{
    // assumes axis is normalized
    float halfAngle = angle / 2, s = std::sin(halfAngle);
    x = axis.x * s;
    y = axis.y * s;
    z = axis.z * s;
    w = std::cos(halfAngle);
    return *this;
}

Quaternion Quaternion::fromUnitVectors(const Vector3& vFrom, const Vector3& vTo)
{
    return Quaternion().setFromUnitVectors(vFrom, vTo);
}

Quaternion Quaternion::fromAxisAngle(const Vector3& axis, float angle)
{
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/index.htm
    float halfAngle = angle / 2;
    float s = sin(halfAngle);

    return Quaternion(axis.x * s, axis.y * s, axis.z * s, cos(halfAngle));
}

Quaternion& Quaternion::operator=(const Quaternion& q)
{
    return set(q);
}

float Quaternion::operator[](uint32_t index) const
{
    return elements[index];
}

Quaternion& Quaternion::inverse()
{
    return conjugate().normalize();
}

Quaternion& Quaternion::conjugate()
{
    x *= -1;
    y *= -1;
    z *= -1;

    return *this;
}

Quaternion Quaternion::conjugated() const
{
    return {x * -1, y * -1, z * -1, w};
}

float Quaternion::dot(const Quaternion& v) const
{
    return x * v.x + y * v.y + z * v.z + w * v.w;
}

float Quaternion::lengthSq() const
{
    return x * x + y * y + z * z + w * w;
}

float Quaternion::length() const
{
    return std::sqrt(x * x + y * y + z * z + w * w);
}

Quaternion& Quaternion::normalize()
{
    float l = length();
    if (l == 0) {
        x = 0;
        y = 0;
        z = 0;
        w = 1;
    } else {
        l = 1 / l;
        x = x * l;
        y = y * l;
        z = z * l;
        w = w * l;

    }
    return *this;
}

Quaternion& Quaternion::operator*=(const Quaternion& b)
{
    float qax = x, qay = y, qaz = z, qaw = w;
    float qbx = b.x, qby = b.y, qbz = b.z, qbw = b.w;

    x = qax * qbw + qaw * qbx + qay * qbz - qaz * qby;
    y = qay * qbw + qaw * qby + qaz * qbx - qax * qbz;
    z = qaz * qbw + qaw * qbz + qax * qby - qay * qbx;
    w = qaw * qbw - qax * qbx - qay * qby - qaz * qbz;

    return *this;
}

Quaternion& Quaternion::slerp(const Quaternion& qb, float t, bool emitSignal)
{
    if(t == 0) return *this;
    if(t == 1) return set(qb);

    float lastX = x, lastY = y, lastZ = z, lastW = w;

    // http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/
    float cosHalfTheta = w * qb.w + x * qb.x + y * qb.y + z * qb.z;

    if ( cosHalfTheta < 0 ) {
        w = -qb.w;
        x = -qb.x;
        y = -qb.y;
        z = -qb.z;

        cosHalfTheta = -cosHalfTheta;
    }
    else {
        *this = qb;
    }

    if ( cosHalfTheta >= 1.0f ) {
        w = lastW;
        x = lastX;
        y = lastY;
        z = lastZ;
        return *this;
    }

    float sinHalfTheta = std::sqrt(1.0f - cosHalfTheta * cosHalfTheta);

    if (std::abs( sinHalfTheta ) < 0.001 ) {
        w = 0.5f * (lastW + w );
        x = 0.5f * (lastX + x );
        y = 0.5f * (lastY + y );
        z = 0.5f * (lastZ + z );

        return *this;
    }

    float halfTheta = std::atan2( sinHalfTheta, cosHalfTheta );
    float ratioA = std::sin( ( 1 - t ) * halfTheta ) / sinHalfTheta,
            ratioB = std::sin( t * halfTheta ) / sinHalfTheta;

    w = (lastW * ratioA + w * ratioB );
    x = (lastX * ratioA + x * ratioB );
    y = (lastY * ratioA + y * ratioB );
    z = (lastZ * ratioA + z * ratioB );

    return *this;
}

bool Quaternion::operator==(const Quaternion& quaternion) const
{
    return quaternion.x == x && quaternion.y == y && quaternion.z == z && quaternion.w == w;
}

Quaternion Quaternion::fromArray(const float* array, uint32_t offset)
{
    return Quaternion(array[offset],
                      array[offset + 1],
                      array[offset + 2],
                      array[offset + 3]);
}

void Quaternion::writeTo(float* array, uint32_t offset) const
{
    array[offset] = x;
    array[offset + 1] = y;
    array[offset + 2] = z;
    array[offset + 3] = w;
}
}
}