//
// Created by lance on 2022/10/31.
//

#include "Vector2.h"

namespace morrow
{
namespace Math
{
Vector2& Vector2::apply(const Matrix3& m)
{
    const float* e = m.elements;

    x = e[0] * x + e[3] * y + e[6];
    y = e[1] * x + e[4] * y + e[7];

    return *this;
}

Vector2::Vector2() : x(0.0f), y(0.0f)
{}

Vector2::Vector2(float _x, float _y) : x(_x), y(_y)
{}

Vector2::Vector2(const Vector2& vector) : x(vector.x), y(vector.y)
{}

Vector2::Vector2(float scalar) : x(scalar), y(scalar)
{}

float Vector2::width() const
{
    return x;
}

float Vector2::height() const
{
    return y;
}

float Vector2::length() const
{
    return std::sqrt(x * x + y * y);
}

float Vector2::lengthSq() const
{
    return x * x + y * y;
}

float Vector2::manhattanLength() const
{
    return std::abs(x) + std::abs(y);
}

float& Vector2::operator[](uint32_t index)
{
    return elements[index];
}

float Vector2::operator[](uint32_t index) const
{
    return elements[index];
}

Vector2& Vector2::operator=(float scalar)
{
    x = scalar;
    y = scalar;
    return *this;
}

Vector2& Vector2::operator+=(const Vector2& vector)
{
    x += vector.x;
    y += vector.y;
    return *this;
}

Vector2& Vector2::operator-=(const Vector2& vector)
{
    x -= vector.x;
    y -= vector.y;
    return *this;
}

Vector2& Vector2::operator*=(const Vector2& vector)
{
    x *= vector.x;
    y *= vector.y;
    return *this;
}

Vector2& Vector2::operator/=(const Vector2& vector)
{
    x /= vector.x;
    y /= vector.y;
    return *this;
}

Vector2& Vector2::operator+=(float scalar)
{
    x += scalar;
    y += scalar;
    return *this;
}

Vector2& Vector2::operator-=(float scalar)
{
    x -= scalar;
    y -= scalar;
    return *this;
}

Vector2& Vector2::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2& Vector2::operator/=(float scalar)
{
    x /= scalar;
    y /= scalar;
    return *this;
}

Vector2& Vector2::set(float _x, float _y)
{
    x = _x;
    y = _y;
    return *this;
}

Vector2& Vector2::clamp(const Vector2& min, const Vector2& max)
{
    // assumes min < max, componentwise
    x = std::max(min.x, std::min(max.x, x));
    y = std::max(min.y, std::min(max.y, y));
    return *this;
}

Vector2& Vector2::clampScalar(float min, float max)
{

    Vector2 vmin{min, min};
    Vector2 vmax{max, max};

    return clamp(vmin, vmax);
}

Vector2& Vector2::clampLength(float min, float max)
{
    float len = length();
    *this /= (len > 0 ? len : 1);
    *this *= std::max(min, std::min(max, len));
    return *this;
}

bool Vector2::isNull() const
{
    return x == 0.0f && y == 0.0f;
}

Vector2& Vector2::floor()
{
    x = std::floor(x);
    y = std::floor(y);

    return *this;
}

Vector2& Vector2::ceil()
{
    x = std::ceil(x);
    y = std::ceil(y);
    return *this;
}

Vector2& Vector2::round()
{
    x = std::round(x);
    y = std::round(y);
    return *this;
}

Vector2& Vector2::roundToZero()
{
    x = x < 0 ? std::ceil(x) : std::floor(x);
    y = y < 0 ? std::ceil(y) : std::floor(y);
    return *this;
}

float Vector2::dot(const Vector2& vector) const
{
    return x * vector.x + y * vector.y;
}

Vector2& Vector2::normalize()
{
    float len = length();
    return *this /= len > 0 ? len : 1;
}

float Vector2::angle()
{
    float angle = std::atan2(y, x);
    if (angle < 0) angle += 2 * MR_PI;
    return angle;
}

float Vector2::squaredDistance(const Vector2& vector)
{
    float dx = x - vector.x, dy = y - vector.y;
    return dx * dx + dy * dy;
}

float Vector2::distanceTo(const Vector2& vector)
{
    return std::sqrt(squaredDistance(vector));
}

float Vector2::manhattanDistance(const Vector2& vector)
{
    return std::abs(x - vector.x) + std::abs(y - vector.y);
}

Vector2& Vector2::setLength(float length)
{
    return normalize() *= length;
}

Vector2& Vector2::lerp(const Vector2& vector, float alpha)
{
    x += (vector.x - x) * alpha;
    y += (vector.y - y) * alpha;

    return *this;
}

Vector2& Vector2::lerpVectors(const Vector2& v1, const Vector2& v2, float alpha)
{
    x = v1.x - v2.x;
    y = v1.y - v2.y;

    return (*this *= alpha) += v1;

}

bool Vector2::operator==(const Vector2& vector) const
{
    return vector.x == x && vector.y == y;
}

bool Vector2::operator!=(const Vector2& vector) const
{
    return !operator==(vector);
}

void Vector2::put(float* array, uint32_t offset)
{
    array[offset] = x;
    array[offset + 1] = y;
}

Vector2& Vector2::rotateAround(const Vector2& center, float angle)
{
    float c = std::cos(angle), s = std::sin(angle);

    float _x = x - center.x;
    float _y = y - center.y;

    x = _x * c - _y * s + center.x;
    y = _x * s + _y * c + center.y;

    return *this;
}

Vector2& Vector2::copy(Vector2& v)
{
    x = v.x;
    y = v.y;
    return *this;
}

Vector2::Vector2(const float* array, uint32_t offset)
{
    x = array[offset];
    y = array[offset + 1];
}
}
}