//
// Created by 0060328 on 25-9-18.
//
#include "OpenglUtils.h"

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
    }
}

GLenum getInternalFormat(PixelDataFormat pixelDataFormat) {
    //版本问题，暂时不支持GL_RGBA8， GL_RGB8等
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
}
}
