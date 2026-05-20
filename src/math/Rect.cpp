//
// Created by lance on 2023/10/10.
//

#include "Rect.h"

namespace morrow
{
namespace Math
{
Rect::Rect() : Min(0.0f, 0.0f), Max(0.0f, 0.0f)
{}

Rect::Rect(const Vector2& min, const Vector2& max) : Min(min), Max(max)
{}

Rect::Rect(const Vector4& v) : Min(v.x, v.y), Max(v.z, v.w)
{}

Rect::Rect(float x1, float y1, float x2, float y2) : Min(x1, y1), Max(x2, y2)
{}

Rect::Rect(const Vector3& min, const Vector3& max) : Min(min.x, min.y), Max(max.x, max.y)
{}

Vector2 Rect::GetCenter() const
{
    return Vector2((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f);
}

Vector2 Rect::GetSize() const
{
    return Vector2(Max.x - Min.x, Max.y - Min.y);
}

float Rect::GetWidth() const
{
    return Max.x - Min.x;
}

float Rect::GetHeight() const
{
    return Max.y - Min.y;
}

float Rect::GetArea() const
{
    return (Max.x - Min.x) * (Max.y - Min.y);
}

Vector2 Rect::GetTL() const
{
    return Min;
}

Vector2 Rect::GetTR() const
{
    return Vector2(Max.x, Min.y);
}

Vector2 Rect::GetBL() const
{
    return Vector2(Min.x, Max.y);
}

Vector2 Rect::GetBR() const
{
    return Max;
}

bool Rect::Contains(const Vector2& p) const
{
    return p.x >= Min.x && p.y >= Min.y && p.x < Max.x && p.y < Max.y;
}

bool Rect::Contains(const Rect& r) const
{
    return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y;
}

bool Rect::Contains(float x, float y) const
{
    return x >= Min.x && y >= Min.y && x < Max.x && y < Max.y;
}

bool Rect::Overlaps(const Rect& r) const
{
    return r.Min.y < Max.y && r.Max.y > Min.y && r.Min.x < Max.x && r.Max.x > Min.x;
}

void Rect::Add(const Vector2& p)
{
    if (Min.x > p.x) Min.x = p.x;
    if (Min.y > p.y) Min.y = p.y;
    if (Max.x < p.x) Max.x = p.x;
    if (Max.y < p.y) Max.y = p.y;
}

void Rect::Add(const Rect& r)
{
    if (Min.x > r.Min.x) Min.x = r.Min.x;
    if (Min.y > r.Min.y) Min.y = r.Min.y;
    if (Max.x < r.Max.x) Max.x = r.Max.x;
    if (Max.y < r.Max.y) Max.y = r.Max.y;
}

void Rect::Expand(const float amount)
{
    Min.x -= amount;
    Min.y -= amount;
    Max.x += amount;
    Max.y += amount;
}

void Rect::Expand(const Vector2& amount)
{
    Min.x -= amount.x;
    Min.y -= amount.y;
    Max.x += amount.x;
    Max.y += amount.y;
}

void Rect::Translate(const Vector2& d)
{
    Min.x += d.x;
    Min.y += d.y;
    Max.x += d.x;
    Max.y += d.y;
}

void Rect::TranslateX(float dx)
{
    Min.x += dx;
    Max.x += dx;
}

void Rect::TranslateY(float dy)
{
    Min.y += dy;
    Max.y += dy;
}

bool Rect::IsInverted() const
{
    return Min.x > Max.x || Min.y > Max.y;
}

void Rect::set(float x, float y, float width, float height)
{
    Min.x = x;
    Min.y = y;
    Max.x = x + width;
    Max.y = y + height;
}
}
} // MORROWGUI