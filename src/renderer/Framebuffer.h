//
// Created by lance on 2023/7/25.
//

#ifndef MORROW_CORE_FRAMEBUFFER_H_
#define MORROW_CORE_FRAMEBUFFER_H_

#include <memory>

#ifdef OPENGL_GLFW
    #include "../platform/wgl/OpenglHeader.h"
#else
    #include "../platform/egl/GLESHeader.h"
#endif
namespace morrow
{
class FrameBuffer;
using FrameBufferSharedPtr = std::shared_ptr<FrameBuffer>;
/**
 *暂时废弃
 */
class FrameBuffer
{
public:
    FrameBuffer(int32_t w, int32_t h);
    ~FrameBuffer();

    void Bind() const;
    void Unbind() const;
    bool IsCompleted();

public:
    GLuint m_ID = 0;
    GLuint m_renderedTexture = 0;

};

} // MORROWGUI

#endif //MORROW_CORE_FRAMEBUFFER_H_
