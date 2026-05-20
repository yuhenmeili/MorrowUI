//
// Created by lance on 2023/1/19.
//

#ifndef MORROW_UTILITY_H
#define MORROW_UTILITY_H

#include <string>
#include <sys/types.h>
#include <vector>
#include <sys/time.h>
#include <cmath>
#include <cfloat>
#include <cstdint>

namespace morrow
{
#define MR_PI 3.14159265358979323846f

namespace Math
{
static const float EPS = 0.000001f;
static constexpr double DEG2RAD = MR_PI / 180.0;
static constexpr double RAD2DEG = 180.0 / MR_PI;

std::string generate_uuid();

std::string generate_hex(int32_t len);

uint32_t random_int(uint32_t min = 0, uint32_t max = 1);

float random_float(float min = 0.0f, float max = 1.0f);

double getCurrentMonotonicTime();

long long getCurrentRealTime();

template<typename T>
inline T clamp(T x, T min, T max)
{
    return std::max(std::min(x, max), min);
}

// compute euclidian modulo of m % n
// https://en.wikipedia.org/wiki/Modulo_operation
template<typename T>
T euclideanModulo(T n, T m)
{
    return ((n % m) + m) % m;
}

// Linear mapping from range <a1, a2> to range <b1, b2>
template<typename T>
T mapLinear(T x, T a1, T a2, T b1, T b2)
{
    return b1 + (x - a1) * (b2 - b1) / (a2 - a1);
}

// https://en.wikipedia.org/wiki/Linear_interpolation
template<typename T>
T lerp(T x, T y, T t)
{

    return (1 - t) * x + t * y;

}

// http://en.wikipedia.org/wiki/Smoothstep
template<typename T>
T smoothstep(T x, T min, T max)
{

    if (x <= min) return 0;
    if (x >= max) return 1;

    x = (x - min) / (max - min);

    return x * x * (3 - 2 * x);
}

template<typename T>
T smootherstep(T x, T min, T max)
{
    if (x <= min) return 0;
    if (x >= max) return 1;

    x = (x - min) / (max - min);

    return x * x * x * (x * (x * 6 - 15) + 10);
}

// Random integer from <low, high> interval
inline int32_t rand(int32_t low, int32_t high)
{
    return low + (int32_t) std::floor(std::rand() * (high - low + 1));
}

// Random float from <low, high> interval

inline float rand(float low, float high)
{
    return low + std::rand() * (high - low);
}

// Random float from <-range/2, range/2> interval
inline float randSpread(float range)
{
    return range * (0.5f - std::rand());
}

inline double degToRad(unsigned short degrees)
{
    return degrees * DEG2RAD;
}

inline unsigned short radToDeg(double radians)
{
    return (unsigned short) (radians * RAD2DEG);
}

inline bool isPowerOfTwo(int32_t value)
{
    return (value & (value - 1)) == 0 && value != 0;
}

inline int32_t nearestPowerOfTwo(int32_t value)
{
    return (int32_t) std::pow(2, round(std::log(value) / M_LN2));
}

inline int32_t ceilPowerOfTwo(float value)
{
    return (int32_t) std::pow(2, std::ceil(std::log(value) / M_LN2));
}

inline int32_t floorPowerOfTwo(float value)
{
    return (int32_t) std::pow(2, std::floor(std::log(value) / M_LN2));
}

inline int32_t nextPowerOfTwo(int32_t value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

template<typename T>
int32_t sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

inline bool equalsEpsilon(float a, float b, float epsilon = FLT_EPSILON)
{
    return std::abs(a - b) < epsilon;
}

inline bool greaterThanOrEquals(float a, float b, float epsilon = FLT_EPSILON)
{
    return a - b > -epsilon;
}

inline bool greaterThan(float a, float b, float epsilon = FLT_EPSILON)
{
    return a - b > epsilon;
}

inline bool lessThanOrEquals(float a, float b, float epsilon = FLT_EPSILON)
{
    return a - b < epsilon;
}

inline bool lessThan(float a, float b, float epsilon = FLT_EPSILON)
{
    return a - b < -epsilon;
}

}

struct TextAttribute
{
    std::vector<float> vertices;
    std::vector<unsigned short> indices;

    void clear(){
        vertices.clear();
        indices.clear();
    }
};

}// MORROWGUI

#endif //MORROW_UTILITY_H
