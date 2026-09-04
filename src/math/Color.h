//
// Created by lance on 2026/9/4.
//

#ifndef MORROW_COLOR_H
#define MORROW_COLOR_H

#include <cstdint>
#include <string>

#include "Vector4.h"

namespace morrow
{
namespace Math
{

/// 以归一化 RGBA 存储的颜色值，支持从 Hex 字符串、0-255 整数和归一化小数构造。
class Color
{
public:
    /// 默认为不透明白色。
    Color();

    /// 使用归一化 RGBA 分量构造，分量会被截断到 [0, 1]。
    Color(float red, float green, float blue, float alpha = 1.0f);

    /// 使用打包整数构造：高于 0xFFFFFF 时按 0xRRGGBBAA 解析，否则按 0xRRGGBB 解析（不透明）。
    explicit Color(uint32_t packed);

    /// 使用 Hex 字符串构造，规则见 setHex；解析失败时为不透明白色。
    explicit Color(const std::string& hex);

    /// 解析 Hex 字符串，支持 3/4/6/8 位（"#F80"、"#F80F"、"FF8800"、"FF8800AA"），可选 "#" 或 "0x" 前缀；成功返回 true。
    bool setHex(const std::string& hex);

    /// 使用 0-255 整数分量设置颜色，分量会被截断到 [0, 255]。
    void setRGB8(int red, int green, int blue, int alpha = 255);

    /// 使用归一化分量设置颜色，分量会被截断到 [0, 1]。
    void set(float red, float green, float blue, float alpha = 1.0f);

    /// 与目标颜色按 t 插值，t 会被截断到 [0, 1]。
    Color lerp(const Color& target, float t) const;

    /// 转换为渲染用的 Vector4。
    Vector4 toVector4() const;

    operator Vector4() const;

    bool operator==(const Color& other) const;

    bool operator!=(const Color& other) const;

    /// 使用 0-255 整数分量构造颜色。
    static Color fromRGB8(int red, int green, int blue, int alpha = 255);

    static const Color WHITE;
    static const Color BLACK;
    static const Color TRANSPARENT;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;

    float r, g, b, a;
};

}
}

#endif //MORROW_COLOR_H
