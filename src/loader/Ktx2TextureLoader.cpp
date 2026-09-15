//
// Created by lance on 26-9-15.
//

#include <cstdio>
#include "Ktx2TextureLoader.h"
#include "GlobalObject.h"
#include "utils/Log.h"

namespace morrow {
Ktx2TextureLoader::Ktx2TextureLoader()
{
    basist::basisu_transcoder_init();
}

bool Ktx2TextureLoader::load(const std::string& fileUrl, basisu::vector<uint8_t>& result, PixelDataFormat& glFormat)
{
    std::FILE* file = std::fopen(fileUrl.c_str(), "rb");
    if (!file) {
        LOG_I("Unable to open the file {}", fileUrl.c_str());
        return false;
    }
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    m_file.resize(fileSize);
    fread(m_file.data(), 1, fileSize, file);
    fclose(file);

    if (!m_transcoder.init(m_file.data(), static_cast<uint32_t>(m_file.size()))) {
        LOG_I("ktx2 init failed: {} (only BasisU compressed KTX2 files are supported)", fileUrl.c_str());
        m_file.clear();
        return false;
    }
    m_width = m_transcoder.get_width();
    m_height = m_transcoder.get_height();

    if (!m_transcoder.start_transcoding()) {
        LOG_I("ktx2 start_transcoding failed: {}", fileUrl.c_str());
        m_file.clear();
        return false;
    }

    auto format = resolveTextureFormat();
    m_compressed = !basist::basis_transcoder_format_is_uncompressed(format);

    uint32_t outputSizeInBlocksOrPixels;
    uint32_t requiredSize;
    if (m_compressed) {
        outputSizeInBlocksOrPixels = ((m_width + 3) / 4) * ((m_height + 3) / 4);
        requiredSize = outputSizeInBlocksOrPixels * basist::basis_get_bytes_per_block_or_pixel(format);
    } else {
        outputSizeInBlocksOrPixels = m_width * m_height;
        requiredSize = outputSizeInBlocksOrPixels * basist::basis_get_uncompressed_bytes_per_pixel(format);
    }
    result.resize(requiredSize);

    if (!m_transcoder.transcode_image_level(0, 0, 0, result.data(), outputSizeInBlocksOrPixels, format, 0)) {
        LOG_I("ktx2 transcode_image_level failed: {}", fileUrl.c_str());
        m_file.clear();
        result.clear();
        return false;
    }

    glFormat = toGlTextureFormat(format);
    m_transcoder.clear();
    return true;
}

bool Ktx2TextureLoader::isCompressed() const
{
    return m_compressed;
}

int32_t Ktx2TextureLoader::getWidth() const
{
    return static_cast<int32_t>(m_width);
}

int32_t Ktx2TextureLoader::getHeight() const
{
    return static_cast<int32_t>(m_height);
}

basist::transcoder_texture_format Ktx2TextureLoader::resolveTextureFormat()
{
    auto sourceFormat = m_transcoder.get_format();
    // 首选 GPU 压缩格式：带 alpha 走 ETC2_RGBA，不带 alpha 走 ETC1_RGB。
    auto preferred = m_transcoder.get_has_alpha() ? basist::transcoder_texture_format::cTFETC2_RGBA
                                                  : basist::transcoder_texture_format::cTFETC1_RGB;
    if (isBasisuFormatSupported(preferred, sourceFormat)) {
        return preferred;
    }
    // 硬件不支持压缩时按源格式回退到未压缩格式；UASTC 源不支持 RGB565。
    if (!m_transcoder.is_uastc() && isBasisuFormatSupported(basist::transcoder_texture_format::cTFRGB565, sourceFormat)) {
        return basist::transcoder_texture_format::cTFRGB565;
    }
    return basist::transcoder_texture_format::cTFRGBA32;
}

bool Ktx2TextureLoader::isBasisuFormatSupported(basist::transcoder_texture_format transcoderTextureFormat, basist::basis_tex_format textureFormat)
{
    PixelDataFormat glTextureFormat = toGlTextureFormat(transcoderTextureFormat);
    return RENDERINGTHREAD->isTextureFormatSupported(glTextureFormat)
        && basist::basis_is_format_supported(transcoderTextureFormat, textureFormat);
}

PixelDataFormat Ktx2TextureLoader::toGlTextureFormat(basist::transcoder_texture_format transcoderTextureFormat)
{
    switch (transcoderTextureFormat) {
        case basist::transcoder_texture_format::cTFETC1_RGB:return PixelDataFormat::COMPRESSED_RGB8_ETC2;
        case basist::transcoder_texture_format::cTFETC2_RGBA:return PixelDataFormat::COMPRESSED_RGBA8_ETC2_EAC;
        case basist::transcoder_texture_format::cTFRGB565:return PixelDataFormat::RGB;
        case basist::transcoder_texture_format::cTFRGBA32:
        case basist::transcoder_texture_format::cTFRGBA4444:return PixelDataFormat::RGBA;
        default:return PixelDataFormat::RGBA;
    }
}
}  // namespace morrow
