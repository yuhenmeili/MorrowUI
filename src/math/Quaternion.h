//
// Created by lance on 2022/10/24.
//

#ifndef MORROW_QUATERNION_H
#define MORROW_QUATERNION_H

#include "Vector3.h"
#include "Matrix4.h"
#include <cstdint>

namespace morrow
{
namespace Math
{
class Vector3;
class Matrix4;
class Quaternion
{
public:
    union
    {
        struct
        {
            float x, y, z, w;
        };
        float elements[4];
    };

    Quaternion& set(const Quaternion& q)
    {
        x = q.x;
        y = q.y;
        z = q.z;
        w = q.w;
        return *this;
    }

    static Quaternion fromUnitVectors(const Vector3& vFrom, const Vector3& vTo);

    // assumes axis is normalized
    static Quaternion fromAxisAngle(const Vector3& axis, float angle);

    Quaternion(float _x, float _y, float _z, float _w);

    Quaternion();

    explicit Quaternion(float scalar);

    Quaternion(const Quaternion& q);

    Quaternion& operator=(const Quaternion& q);

    Quaternion& setFromUnitVectors(const Vector3& from, const Vector3& to);

    Quaternion& setFromAxisAngle(const Vector3& axis, float angle );

    float operator[](uint32_t index) const;

    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/index.htm
    // assumes axis is normalized
    Quaternion(const Vector3& axis, float angle);

    Quaternion& set(const Vector3& axis, float angle);

    Quaternion& set(float _x, float _y, float _z, float _w);
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
    // assumes the upper 3x3 of m is a pure rotation matrix (i.e, unscaled)
    explicit Quaternion(const Matrix4& m);

    Quaternion& set(const Matrix4& m);

    // assumes direction vectors vFrom and vTo are normalized
    //Quaternion(const Vector3 &vFrom, const Vector3 &vTo);

    Quaternion& inverse();

    Quaternion& conjugate();

    Quaternion conjugated() const;

    float dot(const Quaternion& v) const;

    float lengthSq() const;

    float length() const;

    Quaternion& normalize();

    Quaternion& operator*=(const Quaternion& b);

    Quaternion& slerp(const Quaternion& qb, float t, bool emitSignal = true);

    bool operator==(const Quaternion& quaternion) const;

    static Quaternion fromArray(const float* array, uint32_t offset = 0);

    void writeTo(float* array, uint32_t offset = 0) const;
};

inline Quaternion operator*(const Quaternion& q1, const Quaternion& q2)
{
    Quaternion result(q1);
    result *= q2;
    return result;
}
}
}


#endif //MORROW_QUATERNION_H
