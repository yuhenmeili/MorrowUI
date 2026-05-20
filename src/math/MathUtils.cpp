//
// Created by lance on 2023/1/19.
//

#include <sstream>
#include <random>
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
    struct timespec current_timespec;
    clock_gettime(CLOCK_MONOTONIC, &current_timespec);
    double current_time = (double) (current_timespec.tv_sec)
        + (current_timespec.tv_nsec / 1000000000.0);
    return current_time;
}

long long getCurrentRealTime()
{
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
}
} // MORROWGUI