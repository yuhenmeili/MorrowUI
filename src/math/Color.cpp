//
// Created by lance on 2026/9/4.
//

#include <algorithm>
#include <cmath>

#include "Color.h"

namespace
{

int hexDigitValue(char character)
{
    if (character >= '0' && character <= '9')
        return character - '0';
    if (character >= 'a' && character <= 'f')
        return character - 'a' + 10;
    if (character >= 'A' && character <= 'F')
        return character - 'A' + 10;
    return -1;
}

float clampUnit(float value)
{
    return std::min(std::max(value, 0.0f), 1.0f);
}

}

namespace morrow
{
namespace Math
{

Color::Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f)
{}

Color::Color(float red, float green, float blue, float alpha) : r(clampUnit(red)), g(clampUnit(green)), b(clampUnit(blue)), a(clampUnit(alpha))
{}

Color::Color(uint32_t packed)
{
    if (packed > 0xFFFFFFu)
        setRGB8(static_cast<int>((packed >> 24) & 0xFFu), static_cast<int>((packed >> 16) & 0xFFu), static_cast<int>((packed >> 8) & 0xFFu),
                static_cast<int>(packed & 0xFFu));
    else
        setRGB8(static_cast<int>((packed >> 16) & 0xFFu), static_cast<int>((packed >> 8) & 0xFFu), static_cast<int>(packed & 0xFFu));
}

Color::Color(const std::string& hex) : r(1.0f), g(1.0f), b(1.0f), a(1.0f)
{
    setHex(hex);
}

bool Color::setHex(const std::string& hex)
{
    const auto first = hex.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return false;
    const auto last = hex.find_last_not_of(" \t\r\n");
    std::string text = hex.substr(first, last - first + 1);
    if (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0)
        text = text.substr(2);
    else if (!text.empty() && text[0] == '#')
        text = text.substr(1);

    int channels[4] = {0, 0, 0, 255};
    if (text.size() == 3 || text.size() == 4)
    {
        for (size_t index = 0; index < text.size(); ++index)
        {
            const int value = hexDigitValue(text[index]);
            if (value < 0)
                return false;
            channels[index] = value * 17;
        }
    }
    else if (text.size() == 6 || text.size() == 8)
    {
        for (size_t index = 0; index < text.size() / 2; ++index)
        {
            const int high = hexDigitValue(text[index * 2]);
            const int low = hexDigitValue(text[index * 2 + 1]);
            if (high < 0 || low < 0)
                return false;
            channels[index] = high * 16 + low;
        }
    }
    else
    {
        return false;
    }

    r = clampUnit(channels[0] / 255.0f);
    g = clampUnit(channels[1] / 255.0f);
    b = clampUnit(channels[2] / 255.0f);
    a = clampUnit(channels[3] / 255.0f);
    return true;
}

void Color::setRGB8(int red, int green, int blue, int alpha)
{
    const auto clamp8 = [](int value) { return std::min(std::max(value, 0), 255); };
    r = clamp8(red) / 255.0f;
    g = clamp8(green) / 255.0f;
    b = clamp8(blue) / 255.0f;
    a = clamp8(alpha) / 255.0f;
}

void Color::set(float red, float green, float blue, float alpha)
{
    r = clampUnit(red);
    g = clampUnit(green);
    b = clampUnit(blue);
    a = clampUnit(alpha);
}

Color Color::lerp(const Color& target, float t) const
{
    const float k = clampUnit(t);
    return Color(r + (target.r - r) * k, g + (target.g - g) * k, b + (target.b - b) * k, a + (target.a - a) * k);
}

Vector4 Color::toVector4() const
{
    return {r, g, b, a};
}

Color::operator Vector4() const
{
    return {r, g, b, a};
}

bool Color::operator==(const Color& other) const
{
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator!=(const Color& other) const
{
    return !(*this == other);
}

Color Color::fromRGB8(int red, int green, int blue, int alpha)
{
    Color color;
    color.setRGB8(red, green, blue, alpha);
    return color;
}

const Color Color::WHITE(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::BLACK(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::TRANSPARENT(0.0f, 0.0f, 0.0f, 0.0f);
const Color Color::RED(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::GREEN(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::BLUE(0.0f, 0.0f, 1.0f, 1.0f);

}
}
