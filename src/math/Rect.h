//
// Created by lance on 2023/10/10.
//

#ifndef MORROW_MATH_RECT_H_
#define MORROW_MATH_RECT_H_

#include "Vector2.h"
#include "Vector4.h"
namespace morrow
{
namespace Math
{
class Rect
{
public:
    Rect();
    Rect(const Vector2& min, const Vector2& max);
    Rect(const Vector4& v);
    Rect(float x1, float y1, float x2, float y2);
    Rect(const Vector3& min, const Vector3& max);

    Vector2 GetCenter() const;
    Vector2 GetSize() const;
    float GetWidth() const;
    float GetHeight() const;
    float GetArea() const;
    Vector2 GetTL() const;                   // Top-left
    Vector2 GetTR() const;  // Top-right
    Vector2 GetBL() const;  // Bottom-left
    Vector2 GetBR() const;                   // Bottom-right
    bool Contains(const Vector2& p) const;
    bool Contains(const Rect& r) const;
    bool Contains(float x, float y) const;
    bool Overlaps(const Rect& r) const;
    void Add(const Vector2& p);
    void Add(const Rect& r);
    void Expand(const float amount);
    void Expand(const Vector2& amount);
    void Translate(const Vector2& d);
    void TranslateX(float dx);
    void TranslateY(float dy);
//    void        ClipWith(const Rect& r)           { Min = ImMax(Min, r.Min); Max = ImMin(Max, r.Max); }                   // Simple version, may lead to an inverted rectangle, which is fine for Contains/Overlaps test but not for display.
//    void        ClipWithFull(const Rect& r)       { Min = ImClamp(Min, r.Min, r.Max); Max = ImClamp(Max, r.Min, r.Max); } // Full version, ensure both points are fully clipped.
//    void        Floor()                             { Min.x = IM_FLOOR(Min.x); Min.y = IM_FLOOR(Min.y); Max.x = IM_FLOOR(Max.x); Max.y = IM_FLOOR(Max.y); }
    bool IsInverted() const;
//    ImVec4      ToVec4() const                      { return ImVec4(Min.x, Min.y, Max.x, Max.y); }
    void set(float x, float y, float width, float height);
public:
    Vector2 Min;    // Upper-left
    Vector2 Max;    // Lower-right

};
}
} // MORROWGUI

#endif //MORROW_MATH_RECT_H_
