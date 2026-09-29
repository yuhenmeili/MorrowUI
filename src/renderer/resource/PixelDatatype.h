//
// Created by lance on 2023/10/17.
//

#ifndef MORROW_RENDERER_PIXELDATATYPE_H_
#define MORROW_RENDERER_PIXELDATATYPE_H_

#include <cstdint>

#include "morrow/DriverEnums.h"

namespace morrow
{

class PixelDatatype
{
public:
    static bool isPacked(PixelDataType pixelDatatype);

    static int32_t sizeInBytes(PixelDataType pixelDatatype);
};

} // MORROWGUI

#endif //MORROW_RENDERER_PIXELDATATYPE_H_
