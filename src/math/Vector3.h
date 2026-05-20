//
// Created by lance on 2022/10/23.
//

#ifndef MORROW_VECTOR3_H
#define MORROW_VECTOR3_H

#include<cmath>
#include "Matrix4.h"
#include "Vector2.h"
#include "MathUtils.h"
//#include "camera.h"
#include "Matrix3.h"

namespace morrow
{
namespace Math
{
class Matrix4;

class Quaternion;

class Matrix3;

class Vector2;

class Vector3
{
public:
    union
    {
        struct
        {
            float x, y, z;
        };
        float elements[3];
    };

    Vector3();

    explicit Vector3(float scalar);

    explicit Vector3(const Vector2& vector2);

    Vector3(float _x, float _y, float _z);

    Vector3(const Vector3& v);

    template<typename T>
    static Vector3 fromArray(const T* array, uint32_t offset)
    {
        float _x = array[offset];
        float _y = array[offset + 1];
        float _z = array[offset + 2];

        return {_x, _y, _z};
    }

    static Vector3 fromMatrixColumn(const Matrix4& m, uint32_t index);

    static Vector3 fromMatrixScale(const Matrix4& m);

    Vector3& set(float _x, float _y, float _z);

    float operator[](uint32_t index) const;

    float& operator[](uint32_t index);

    bool operator!();

    using const_iterator = const float*;

    const_iterator cbegin() const;

    const_iterator cend() const;

    float* end();

    Vector3& operator=(float scalar);

    Vector3& operator+=(const Vector3& vector);

    Vector3& operator+=(float scalar);

    Vector3& operator-=(const Vector3& v);

    Vector3& operator-=(float scalar);

    Vector3& operator*=(const Vector3& v);

    Vector3& operator*=(float scalar);

    //apply matrix3
    Vector3& apply(const Matrix3& m);

    //apply matrix4
    Vector3& apply(const Matrix4& m);

    //apply quaternion
    Vector3& apply(const Quaternion& q);

    Vector3& apply(const Vector3& axis, float angle);

//    Vector3& project(const Camera& camera);
//
//    Vector3 project(const Camera& camera) const;
//
//    Vector3& unproject(const Camera& camera);
//
//    Vector3 unproject(const Camera& camera) const;

    Vector3& unproject(const Matrix4& world, const Matrix4& projection);

    // input: THREE.Matrix4 affine matrix
    // vector interpreted as a direction
    Vector3& transformDirection(const Matrix4& m);

    Vector3& operator/=(const Vector3& v);

    Vector3& operator/=(float scalar);

    Vector3& min(const Vector3& v);

    Vector3& max(const Vector3& v);

    Vector3& clamp(const Vector3& min, const Vector3& max);

    Vector3& clamp(float minVal, float maxVal);

    Vector3& clampLength(float min, float max);

    Vector3& floor();

    Vector3& ceil();

    Vector3& round();

    Vector3& roundToZero();

    Vector3& negate();

    Vector3 negated() const;

    float lengthSq() const;

    float length() const;

    float manhattanLength() const;

    Vector3& normalize();

    Vector3 normalized() const;

    Vector3& setLength(float length);

    Vector3& lerp(const Vector3& v, float alpha);

    Vector3 lerped(const Vector3& v, float alpha) const;

    Vector3& lerpVectors(const Vector3& v1, const Vector3& v2, float alpha);

    Vector3& project(const Vector3& vector);

    Vector3& projectOnPlane(const Vector3& planeNormal);

    // reflect incident vector off plane orthogonal to normal
    // normal is assumed to have unit length
    Vector3& reflect(const Vector3& normal);

    float angleTo(const Vector3& v) const;

    float distanceToSquared(const Vector3& v) const;

    float distanceTo(const Vector3& v) const;

    float manhattanDistance(const Vector3& v) const;

    // Angle around the Y axis, counter-clockwise when looking from above.
    float azimuth() const;

    // Angle above the XZ plane.
    float inclination() const;

    template<typename Array>
    void toArray(Array array, uint32_t offset = 0) const
    {
        array[offset] = x;
        array[offset + 1] = y;
        array[offset + 2] = z;
    }

    bool isNull() const;

    bool operator==(const Vector3& v) const;

    bool operator!=(const Vector3& v) const;

    Vector3& subVectors(Vector3& a, Vector3& b);

    Vector3& crossVectors(const Vector3& a, const Vector3& b);

    Vector3& divideScalar(float scalar);

    Vector3& multiplyScalar(float scalar);

    Vector3& add(Vector3& v);

    Vector3& copy(Vector3& v);

    Vector3& sub(Vector3& v);

    Vector3& cross(const Vector3& v);

    Vector3& applyMatrix4(const Matrix4& m);
};

inline Vector3 operator+(const Vector3& left, const Vector3& right)
{
    Vector3 result{left};
    result += right;
    return result;
}

inline Vector3 operator-(const Vector3& left, const Vector3& right)
{
    Vector3 result{left};
    result -= right;
    return result;
}


inline Vector3 operator*(const Vector3& left, const Vector3& right)
{
    Vector3 result{left};
    result *= right;
    return result;
}

inline Vector3 operator*(const Vector3& vector, float scalar)
{
    Vector3 result{vector};
    result *= scalar;
    return result;
}

inline Vector3 operator*(const Vector3& vector, const Matrix4& matrix)
{
    return Vector3(vector).apply(matrix);
}

inline Vector3 operator/(const Vector3& v1, const Vector3& v2)
{
    Vector3 vector(v1);
    vector /= v2;

    return vector;
}

inline Vector3 operator/(const Vector3& vector, float scalar)
{
    return vector * (1.0f / scalar);
}

inline Vector3 cross(const Vector3& a, const Vector3& b)
{
    float ax = a.x, ay = a.y, az = a.z;
    float bx = b.x, by = b.y, bz = b.z;

    return {ay * bz - az * by,
            az * bx - ax * bz,
            ax * by - ay * bx};
}

inline float dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 operator+(const Vector3& vector, float scalar)
{
    Vector3 v(vector);
    return v += scalar;
}

inline Vector3 operator*(float scalar, const Vector3& vector)
{
    Vector3 v(vector);
    return v *= scalar;
}
}

}

#endif //MORROW_VECTOR3_H
