//
// Created by lance on 2022/10/31.
//

#ifndef MORROW_VECTOR2_H
#define MORROW_VECTOR2_H

#include <valarray>
#include <Matrix3.h>

namespace morrow
{
namespace Math
{

class Matrix3;

class Vector2
{
public:
    union
    {
        struct
        {
            float x, y;
        };
        float elements[2];
    };

    Vector2();

    Vector2(float _x, float _y);

    Vector2(const Vector2& vector);

    explicit Vector2(float scalar);

    float width() const;

    float height() const;

    float length() const;

    float lengthSq() const;

    float manhattanLength() const;

    float& operator[](uint32_t index);

    float operator[](uint32_t index) const;

    Vector2& operator=(float scalar);

    Vector2& operator+=(const Vector2& vector);

    Vector2& operator-=(const Vector2& vector);

    Vector2& operator*=(const Vector2& vector);

    Vector2& operator/=(const Vector2& vector);

    Vector2& operator+=(float scalar);

    Vector2& operator-=(float scalar);

    Vector2& operator*=(float scalar);

    Vector2& operator/=(float scalar);

    Vector2& set(float _x, float _y);

    Vector2& clamp(const Vector2& min, const Vector2& max);

    Vector2& clampScalar(float min, float max);

    Vector2& clampLength(float min, float max);

    bool isNull() const;

    Vector2& floor();

    Vector2& ceil();

    Vector2& round();

    Vector2& roundToZero();

    float dot(const Vector2& vector) const;

    Vector2& normalize();

    // computes the angle in radians with respect to the positive x-axis
    float angle();

    float squaredDistance(const Vector2& vector);

    float distanceTo(const Vector2& vector);

    float manhattanDistance(const Vector2& vector);

    Vector2& setLength(float length);

    Vector2& lerp(const Vector2& vector, float alpha);

    Vector2& lerpVectors(const Vector2& v1, const Vector2& v2, float alpha);

    bool operator==(const Vector2& vector) const;

    bool operator!=(const Vector2& vector) const;

    explicit Vector2(const float* array, uint32_t offset = 0);

    Vector2& apply(const Matrix3& matrix);

    void put(float* array, uint32_t offset = 0);

    Vector2& rotateAround(const Vector2& center, float angle);

    Vector2& copy(Vector2& v);
};

inline Vector2 operator+(const Vector2& left, const Vector2& right)
{
    Vector2 result{left};
    result += right;
    return result;
}

inline Vector2 operator-(const Vector2& left, const Vector2& right)
{
    Vector2 result{left};
    result -= right;
    return result;
}

inline Vector2 operator*(const Vector2& left, const Vector2& right)
{
    Vector2 result{left};
    result *= right;
    return result;
}

inline Vector2 operator*(const Vector2& left, float right)
{
    Vector2 result{left};
    result *= right;
    return result;
}

inline Vector2 operator/(const Vector2& left, float right)
{
    Vector2 result{left};
    result /= right;
    return result;
}

inline Vector2 operator/(const Vector2& left, const Vector2& right)
{
    Vector2 result{left};
    result /= right;
    return result;
}

inline Vector2 min(const Vector2& v1, const Vector2& v2)
{
    return Vector2(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
}

inline Vector2 max(const Vector2& v1, const Vector2& v2)
{
    return Vector2(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
}

}
}


#endif //MORROW_VECTOR2_H
