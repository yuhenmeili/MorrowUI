//
// Created by 0060328 on 25-10-11.
//

#ifndef GLOBALDEFINE_H
#define GLOBALDEFINE_H
#include <cstdint>

// CMake: MORROW_ENABLE_BASISU（target 编译定义，PUBLIC）。
// 此处兜底默认值服务于直接编译部分源文件的测试目标，语义与 CMake 默认一致。
#ifndef MORROW_ENABLE_BASISU
#define MORROW_ENABLE_BASISU 1
#endif

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
