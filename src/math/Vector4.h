//
// Created by lance on 2022/10/31.
//

#ifndef MORROW_VECTOR4_H
#define MORROW_VECTOR4_H

#include <cstring>
#include <cmath>
#include <algorithm>
#include "Matrix4.h"
#include "Quaternion.h"

namespace morrow
{
namespace Math
{

class Matrix4;

class Quaternion;

class Vector4
{
public:
    union
    {
        struct
        {
            float x{}, y{}, z{}, w{};
        };
        float elements[4];
    };

    Vector4();

    Vector4(float _x, float _y, float _z, float _w);

    Vector4(const Vector4& vector);

    Vector4(const float* array, uint32_t offset);

    bool isNull() const;

    void setNull();

    Vector4& set(float _x, float _y, float _z, float _w);

    Vector4& setScalar(float scalar);

    Vector4& copy(const Vector4& v);

    Vector4& set(uint32_t index, float value);

    float get(uint32_t index) const;

    float dot(const Vector4& v) const;

    float lengthSq() const;

    float length() const;

    float manhattanLength() const;

    Vector4& add(const Vector4& v);

    Vector4& addScalar(float s);

    Vector4& addVectors(const Vector4& a, const Vector4& b);

    Vector4& addScaledVector(const Vector4& v, float s);

    Vector4& sub(const Vector4& v);

    Vector4& sub(const Vector4& a, const Vector4& b);

    Vector4& sub(float s);

    Vector4& multiply(float scalar);

    Vector4& divide(float scalar);

    Vector4& operator*=(const Matrix4& m);

    Vector4& setAxisAngleFromQuaternion(const Quaternion& q);

    Vector4& setAxisAngleFromRotationMatrix(const Matrix4& m);

    Vector4& min(const Vector4& v);

    Vector4& max(const Vector4& v);

    Vector4& clamp(const Vector4& min, const Vector4& max);

    Vector4& clampScalar(float minVal, float maxVal);

    Vector4& clampLength(float min, float max);

    Vector4& floor();

    Vector4& ceil();

    Vector4& round();

    Vector4& roundToZero();

    Vector4& negate();

    Vector4& normalize();

    Vector4& setLength(float length);

    Vector4& lerp(const Vector4& v, float alpha);

    Vector4& lerp(const Vector4& v1, const Vector4& v2, float alpha);

    bool operator==(const Vector4& v) const;

    bool operator!=(const Vector4& v) const;

    void writeTo(float* array, uint32_t offset = 0);

    static const Vector4 ZERO;
    static const Vector4 ONE;
    static const Vector4 UNIT_X;
    static const Vector4 UNIT_Y;
    static const Vector4 UNIT_Z;
    static const Vector4 UNIT_W;
};

inline Vector4 operator*(const Vector4& vector, float scalar)
{
    Vector4 value(vector);
    value.multiply(scalar);
    return value;
}

inline Vector4 operator-(const Vector4& a, const Vector4& b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;
    float w = a.w - b.w;

    return {x, y, z, w};
}

}
}


#endif //MORROW_VECTOR4_H
