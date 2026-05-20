//
// Created by 0060328 on 25-10-11.
//

#ifndef GLOBALDEFINE_H
#define GLOBALDEFINE_H
#include <cstdint>

enum class ImageType : uint8_t {
    IMAGE = 0, //默认就是图片
    OES = 1,
    TEXT = 2
};

enum class HorizontalAlignment : uint8_t {
    LEFT = 0,
    CENTER = 1,
    RIGHT = 2,
    JUSTIFY = 3
};

enum class VerticalAlignment : uint8_t {
    TOP = 0,
    CENTER = 1,
    BOTTOM = 2,
    JUSTIFY = 3
};
#endif //GLOBALDEFINE_H
