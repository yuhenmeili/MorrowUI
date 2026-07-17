//
// Created by lance on 2023/10/17.
//

#include "PixelFormat.h"

#include "DriverEnums.h"
#include "PixelDatatype.h"

namespace morrow {
int32_t PixelFormat::alignmentInBytes(PixelDataFormat pixelFormat, GLenum pixelDatatype, int32_t width) {
    const int32_t mod = textureSizeInBytes(pixelFormat, pixelDatatype, width, 1) % 4;
    return mod == 0 ? 4 : mod == 2 ? 2 : 1;
};

int32_t PixelFormat::textureSizeInBytes(PixelDataFormat pixelFormat, GLenum pixelDatatype, int32_t width, int32_t height) {
    auto componentsLength = PixelFormat::componentsLength(pixelFormat);
    if (PixelDatatype::isPacked(pixelDatatype)) {
        componentsLength = 1;
    }
    return componentsLength * PixelDatatype::sizeInBytes(pixelDatatype) * width * height;
};

int32_t PixelFormat::componentsLength(PixelDataFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelDataFormat::R: return 1;
        case PixelDataFormat::RG: return 2;
        case PixelDataFormat::RGB: return 3;
        case PixelDataFormat::RGBA: return 4;
        case PixelDataFormat::ALPHA: return 1;
        default: return 1;
    }
};
} // MORROWGUI
