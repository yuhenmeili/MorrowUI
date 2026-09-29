//
// Created by lance on 2023/10/17.
//

#include "PixelDatatype.h"

namespace morrow
{
bool PixelDatatype::isPacked(PixelDataType pixelDatatype)
{
    return (
        pixelDatatype == PixelDataType::USHORT_565 ||
            pixelDatatype == PixelDataType::UINT_2_10_10_10_REV ||
            pixelDatatype == PixelDataType::COMPRESSED
    );
};

int32_t PixelDatatype::sizeInBytes(PixelDataType pixelDatatype)
{
    switch (pixelDatatype) {
        case PixelDataType::UBYTE:
        case PixelDataType::BYTE:
            return 1;
        case PixelDataType::USHORT:
        case PixelDataType::SHORT:
        case PixelDataType::HALF:
        case PixelDataType::USHORT_565:
            return 2;
        case PixelDataType::UINT:
        case PixelDataType::INT:
        case PixelDataType::FLOAT:
        case PixelDataType::UINT_10F_11F_11F_REV:
        case PixelDataType::UINT_2_10_10_10_REV:
            return 4;
        case PixelDataType::COMPRESSED:
            // 压缩像素的尺寸由压缩格式单独决定，不在此通用换算。
            return 0;
    }
    return 0;
};

} // MORROWGUI
