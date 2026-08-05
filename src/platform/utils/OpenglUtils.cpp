//
// Created by 0060328 on 25-9-18.
//
#include "OpenglUtils.h"

#include "Log.h"

namespace morrow {
namespace OpenglUtils {
GLenum getFormat(PixelDataFormat pixelDataFormat) {
    switch (pixelDataFormat) {
        case PixelDataFormat::ALPHA:
            return GL_ALPHA;
        case PixelDataFormat::R:
            return GL_RED;
        case PixelDataFormat::RG:
            return GL_RG;
        case PixelDataFormat::RGB:
            return GL_RGB;
        case PixelDataFormat::RGBA:
            return GL_RGBA;
        case PixelDataFormat::COMPRESSED_RGB8_ETC2:
            return GL_COMPRESSED_RGB8_ETC2;
        case PixelDataFormat::COMPRESSED_RGBA8_ETC2_EAC:
            return GL_COMPRESSED_RGBA8_ETC2_EAC;
        default:
            LOG_INFO("Unsupported pixel data format {}", pixelDataFormat);
            return GL_RGBA;
    }
}

GLenum getInternalFormat(PixelDataFormat pixelDataFormat) {
    // 版本问题，暂时不支持GL_RGBA8， GL_RGB8等
    switch (pixelDataFormat) {
        case PixelDataFormat::ALPHA:
            return GL_ALPHA;
        case PixelDataFormat::R:
            return GL_RED;
        case PixelDataFormat::RG:
            return GL_RG;
        case PixelDataFormat::RGB:
            return GL_RGB;
        case PixelDataFormat::RGBA:
            return GL_RGBA;
        case PixelDataFormat::COMPRESSED_RGB8_ETC2:
            return GL_COMPRESSED_RGB8_ETC2;
        case PixelDataFormat::COMPRESSED_RGBA8_ETC2_EAC:
            return GL_COMPRESSED_RGBA8_ETC2_EAC;
        default:
            LOG_INFO("Unsupported pixel Internal format {}", pixelDataFormat);
            return GL_RGBA;
    }
}

GLint getMinFilterType(SamplerMinFilter minFilterType) {
    switch (minFilterType) {
        case SamplerMinFilter::NEAREST:
            return GL_NEAREST;
        case SamplerMinFilter::LINEAR:
            return GL_LINEAR;
        default:
            return GL_LINEAR;
    }
}

GLint getMagFilterType(SamplerMagFilter magFilterType) {
    switch (magFilterType) {
        case SamplerMagFilter::NEAREST:
            return GL_NEAREST;
        case SamplerMagFilter::LINEAR:
            return GL_LINEAR;
        default:
            return GL_LINEAR;
    }
}

GLenum toGLBlendFactor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::ZERO:
            return GL_ZERO;
        case BlendFactor::ONE:
            return GL_ONE;
        case BlendFactor::SRC_COLOR:
            return GL_SRC_COLOR;
        case BlendFactor::ONE_MINUS_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DST_COLOR:
            return GL_DST_COLOR;
        case BlendFactor::ONE_MINUS_DST_COLOR:
            return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SRC_ALPHA:
            return GL_SRC_ALPHA;
        case BlendFactor::ONE_MINUS_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DST_ALPHA:
            return GL_DST_ALPHA;
        case BlendFactor::ONE_MINUS_DST_ALPHA:
            return GL_ONE_MINUS_DST_ALPHA;
    }
    return GL_ONE;
}
}  // namespace OpenglUtils
}  // namespace morrow
