//
// Created by lance on 2023/1/19.
//

#include <chrono>
#include <random>
#include <sstream>
#include "MathUtils.h"

namespace morrow
{
namespace Math
{
std::string generate_uuid()
{
    std::stringstream hexstream;
    hexstream << generate_hex(4) << "-" << generate_hex(2) << "-"
              << generate_hex(2) << "-" << generate_hex(2) << "-"
              << generate_hex(6);
    return hexstream.str();
}

std::string generate_hex(int32_t len)
{
    std::stringstream ss;
    for (auto i = 0; i < len; i++) {
        const auto rc = random_int(0, 255);
        std::stringstream hexstream;
        hexstream << std::hex << rc;
        auto hex = hexstream.str();
        ss << (hex.length() < 2 ? '0' + hex : hex);
    }
    return ss.str();
}

uint32_t random_int(uint32_t min, uint32_t max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution dis(min, max);
    return dis(gen);
}


float random_float(float min, float max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

double getCurrentMonotonicTime()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

long long getCurrentRealTime()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}
}
} // MORROWGUI
