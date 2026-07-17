//
// Created by lance on 2023/10/17.
//

#include "PixelDatatype.h"

namespace morrow
{
bool PixelDatatype::isPacked(GLenum pixelDatatype)
{
    return (
        pixelDatatype == GL_UNSIGNED_INT_24_8 ||
            pixelDatatype == GL_UNSIGNED_SHORT_4_4_4_4 ||
            pixelDatatype == GL_UNSIGNED_SHORT_5_5_5_1 ||
            pixelDatatype == GL_UNSIGNED_SHORT_5_6_5
    );
};

int32_t PixelDatatype::sizeInBytes(GLenum pixelDatatype)
{
    switch (pixelDatatype) {
        case GL_UNSIGNED_BYTE:
            return 1;
        case GL_UNSIGNED_SHORT:
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_HALF_FLOAT:
            return 2;
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_UNSIGNED_INT_24_8:
            return 4;
    }
};

} // MORROWGUI