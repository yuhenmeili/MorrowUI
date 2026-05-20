//
// Created by lance on 2023/7/25.
//

#include <cstdio>
#include "Framebuffer.h"

namespace morrow
{
FrameBuffer::FrameBuffer(int32_t w, int32_t h)
{
    glGenFramebuffers(1, &m_ID);
    Bind();
    glGenTextures(1, &m_renderedTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_renderedTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderedTexture, 0);
    Unbind();
}
FrameBuffer::~FrameBuffer()
{
    if (m_ID > 0){
        glDeleteFramebuffers(1, &m_ID);
    }
    if(m_renderedTexture > 0){
        glDeleteTextures(1, &m_renderedTexture);
    }
}

void FrameBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
}

void FrameBuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool FrameBuffer::IsCompleted()
{
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}
} // MORROWGUI