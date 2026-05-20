//
// Created by lance on 2023/10/17.
//

#ifndef MORROW_RENDERER_PIXELFORMAT_H_
#define MORROW_RENDERER_PIXELFORMAT_H_

#include <cstdint>

#include "DriverEnums.h"

#ifdef OPENGL_GLFW
    #include "../platform/wgl/OpenglHeader.h"
#else
    #include "../platform/egl/GLESHeader.h"
#endif
namespace morrow
{

class PixelFormat
{
public:
    static int32_t alignmentInBytes(PixelDataFormat pixelFormat, GLenum pixelDatatype, int32_t width);

    static int32_t textureSizeInBytes(PixelDataFormat pixelFormat, GLenum pixelDatatype, int32_t width, int32_t height);

    static int32_t componentsLength(PixelDataFormat pixelFormat);
};

} // MORROWGUI

#endif //MORROW_RENDERER_PIXELFORMAT_H_
