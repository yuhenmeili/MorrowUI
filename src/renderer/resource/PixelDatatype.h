//
// Created by lance on 2023/10/17.
//

#ifndef MORROW_RENDERER_PIXELDATATYPE_H_
#define MORROW_RENDERER_PIXELDATATYPE_H_

#ifdef OPENGL_GLFW
    #include "platform/wgl/OpenglHeader.h"
#else
    #include "platform/egl/GLESHeader.h"
#endif
namespace morrow
{

class PixelDatatype
{
public:
    static bool isPacked(GLenum pixelDatatype);

    static int32_t sizeInBytes(GLenum pixelDatatype);
};

} // MORROWGUI

#endif //MORROW_RENDERER_PIXELDATATYPE_H_
