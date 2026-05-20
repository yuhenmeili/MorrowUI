//
// Created by lance on 24-6-28.
//

#include <fstream>
#include "BasisTextureLoader.h"
#include "GlobalObject.h"

namespace morrow{
BasisTextureLoader::BasisTextureLoader()
{
    basisu_transcoder_init();
}

void BasisTextureLoader::load(const std::string& fileUrl, basisu::vector<uint8_t>& result, PixelDataFormat& glFormat)
{
    std::FILE* file = std::fopen(fileUrl.c_str(), "rb");
    if (!file) {
        LOG_I("Unable to open the file {}", fileUrl.c_str());
        return;
    }
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    m_file.resize(fileSize);
    fread(m_file.data(), 1, fileSize, file);
    fclose(file);

    if (!m_transcoder.validate_header(m_file.data(), m_file.size())) {
        LOG_I("basis_file::basis_file: m_transcoder.validate_header() failed!\n");
        m_file.clear();
        return;
    }
    if (!startTranscoding()) {
        LOG_I("Unable to start transcoding");
        return;
    }
//    LOG_I("GL_COMPRESSED_R11_EAC supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_R11_EAC));
//    LOG_I("GL_COMPRESSED_SIGNED_R11_EAC supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_SIGNED_R11_EAC));
//    LOG_I("GL_COMPRESSED_RG11_EAC supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_RG11_EAC));
//    LOG_I("GL_COMPRESSED_SIGNED_RG11_EAC supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_SIGNED_RG11_EAC));
//    LOG_I("GL_COMPRESSED_RGB8_ETC2 supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_RGB8_ETC2));
//    LOG_I("GL_COMPRESSED_SRGB8_ETC2 supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_SRGB8_ETC2));
//    LOG_I("GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2));
//    LOG_I("GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2));
//    LOG_I("GL_COMPRESSED_RGBA8_ETC2_EAC supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_RGBA8_ETC2_EAC));
//    LOG_I("GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC supported %d", RENDERINGTHREAD->isTextureFormatSupported(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC));

    transcoder_texture_format format = resolveTextureFormat(); // Or another supported format
    if (!transcodeImage(result, 0, 0, int(format), 0, 0)) {
        LOG_I("transcodeImage failed");
        return;
    }
    glFormat = toGlTextureFormat(format);
    m_transcoder.stop_transcoding();
}

transcoder_texture_format BasisTextureLoader::resolveTextureFormat()
{
    auto fileDesc = getFileDesc();
    auto btf = static_cast<basis_tex_format>(fileDesc.m_tex_format);
    auto imageDesc = getImageDesc(0);
    if (imageDesc.m_alpha_flag) {
        if (isBasisuFormatSupported(transcoder_texture_format::cTFETC2_RGBA, btf)) {
            return transcoder_texture_format::cTFETC2_RGBA;
        }
        return transcoder_texture_format::cTFRGBA32;
    } else {
        if (isBasisuFormatSupported(transcoder_texture_format::cTFETC1_RGB, btf)) {
            return transcoder_texture_format::cTFETC1_RGB;
        }
        return transcoder_texture_format::cTFRGB565;
    }
}

bool BasisTextureLoader::isBasisuFormatSupported(transcoder_texture_format transcoderTextureFormat, basis_tex_format textureFormat)
{
    PixelDataFormat glTextureFormat = toGlTextureFormat(transcoderTextureFormat);
    return RENDERINGTHREAD->isTextureFormatSupported(glTextureFormat) && isTranscoderTextureFormatSupported(transcoderTextureFormat, textureFormat);
}

bool BasisTextureLoader::isTranscoderTextureFormatSupported(transcoder_texture_format transcoderTexFormat, basis_tex_format textureFormat)
{
    return basist::basis_is_format_supported(transcoderTexFormat, textureFormat);
}

PixelDataFormat BasisTextureLoader::toGlTextureFormat(transcoder_texture_format transcoderTextureFormat)
{
    switch (transcoderTextureFormat) {
        case transcoder_texture_format::cTFETC1_RGB:return PixelDataFormat::COMPRESSED_RGB8_ETC2;
        case transcoder_texture_format::cTFETC2_RGBA:return PixelDataFormat::COMPRESSED_RGBA8_ETC2_EAC;
        case transcoder_texture_format::cTFRGB565:return PixelDataFormat::RGB;
        case transcoder_texture_format::cTFRGBA32:
        case transcoder_texture_format::cTFRGBA4444:return PixelDataFormat::RGBA;
        default:return PixelDataFormat::RGBA;
    }
}

void BasisTextureLoader::close()
{
    m_file.clear();
}

uint32_t BasisTextureLoader::getHasAlpha()
{
    basisu_image_level_info li{};
    if (!m_transcoder.get_image_level_info(m_file.data(), m_file.size(), li, 0, 0))
        return 0;

    return li.m_alpha_flag;
}

uint32_t BasisTextureLoader::getNumImages()
{
    return m_transcoder.get_total_images(m_file.data(), m_file.size());
}

uint32_t BasisTextureLoader::getNumLevels(uint32_t image_index)
{
    basisu_image_info ii{};
    if (!m_transcoder.get_image_info(m_file.data(), m_file.size(), ii, image_index))
        return 0;

    return ii.m_total_levels;
}

uint32_t BasisTextureLoader::getImageWidth(uint32_t image_index, uint32_t level_index)
{
    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file.data(), m_file.size(), image_index, level_index, orig_width, orig_height, total_blocks))
        return 0;

    return orig_width;
}

uint32_t BasisTextureLoader::getImageHeight(uint32_t image_index, uint32_t level_index)
{
    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file.data(), m_file.size(), image_index, level_index, orig_width, orig_height, total_blocks))
        return 0;

    return orig_height;
}

basis_file_desc BasisTextureLoader::getFileDesc()
{
    basis_file_desc result{};
    memset(&result, 0, sizeof(result));

    basisu_file_info file_info;

    if (!m_transcoder.get_file_info(m_file.data(), m_file.size(), file_info)) {
        return result;
    }

    result.m_version = file_info.m_version;
    result.m_us_per_frame = file_info.m_us_per_frame;
    result.m_total_images = file_info.m_total_images;
    result.m_userdata0 = file_info.m_userdata0;
    result.m_userdata1 = file_info.m_userdata1;
    result.m_tex_format = static_cast<uint32_t>(file_info.m_tex_format);
    result.m_y_flipped = file_info.m_y_flipped;
    result.m_has_alpha_slices = file_info.m_has_alpha_slices;

    result.m_num_endpoints = file_info.m_total_endpoints;
    result.m_endpoint_palette_ofs = file_info.m_endpoint_codebook_ofs;
    result.m_endpoint_palette_len = file_info.m_endpoint_codebook_size;

    result.m_num_selectors = file_info.m_total_selectors;
    result.m_selector_palette_ofs = file_info.m_selector_codebook_ofs;
    result.m_selector_palette_len = file_info.m_selector_codebook_size;

    result.m_tables_ofs = file_info.m_tables_ofs;
    result.m_tables_len = file_info.m_tables_size;

    return result;
}

basis_image_desc BasisTextureLoader::getImageDesc(uint32_t image_index)
{
    basis_image_desc result{};
    memset(&result, 0, sizeof(result));

    basisu_image_info image_info{};

    // bool get_image_info(const void *pData, uint32_t data_size, basisu_image_info &image_info, uint32_t image_index) const;
    if (!m_transcoder.get_image_info(m_file.data(), m_file.size(), image_info, image_index)) {
        return result;
    }

    result.m_orig_width = image_info.m_orig_width;
    result.m_orig_height = image_info.m_orig_height;
    result.m_width = image_info.m_width;
    result.m_height = image_info.m_height;
    result.m_num_blocks_x = image_info.m_num_blocks_x;
    result.m_num_blocks_y = image_info.m_num_blocks_y;
    result.m_num_levels = image_info.m_total_levels;
    result.m_alpha_flag = image_info.m_alpha_flag;
    result.m_iframe_flag = image_info.m_iframe_flag;

    return result;
}

basis_image_level_desc BasisTextureLoader::getImageLevelDesc(uint32_t image_index, uint32_t level_index)
{
    basis_image_level_desc result{};
    memset(&result, 0, sizeof(result));

    basisu_image_level_info image_info{};

    if (!m_transcoder.get_image_level_info(m_file.data(), m_file.size(), image_info, image_index, level_index)) {
        return result;
    }

    result.m_rgb_file_ofs = image_info.m_rgb_file_ofs;
    result.m_rgb_file_len = image_info.m_rgb_file_len;
    result.m_alpha_file_ofs = image_info.m_alpha_file_ofs;
    result.m_alpha_file_len = image_info.m_alpha_file_len;

    return result;
}

uint32_t BasisTextureLoader::getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format)
{
    if (format >= (int) transcoder_texture_format::cTFTotalTextureFormats)
        return 0;

    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file.data(), m_file.size(), image_index, level_index, orig_width, orig_height, total_blocks))
        return 0;

    const auto transcoder_format = static_cast<transcoder_texture_format>(format);

    if (basis_transcoder_format_is_uncompressed(transcoder_format)) {
        // Uncompressed formats are just plain raster images.
        const uint32_t bytes_per_pixel = basis_get_uncompressed_bytes_per_pixel(transcoder_format);
        const uint32_t bytes_per_line = orig_width * bytes_per_pixel;
        const uint32_t bytes_per_slice = bytes_per_line * orig_height;
        return bytes_per_slice;
    } else {
        // Compressed formats are 2D arrays of blocks.
        const uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(transcoder_format);

        if (transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGB || transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGBA) {
            // For PVRTC1, Basis only writes (or requires) total_blocks * bytes_per_block. But GL requires extra padding for very small textures:
            // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
            const uint32_t width = (orig_width + 3) & ~3;
            const uint32_t height = (orig_height + 3) & ~3;
            const uint32_t size_in_bytes = (std::max(8U, width) * std::max(8U, height) * 4 + 7) / 8;
            return size_in_bytes;
        }

        return total_blocks * bytes_per_block;
    }
}

bool BasisTextureLoader::isUASTC()
{
    return m_transcoder.get_tex_format(m_file.data(), m_file.size()) == basis_tex_format::cUASTC4x4;
}

uint32_t BasisTextureLoader::startTranscoding()
{
    return m_transcoder.start_transcoding(m_file.data(), m_file.size());
}

uint32_t BasisTextureLoader::transcodeImage(basisu::vector<uint8_t>& dst_data, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t unused, uint32_t get_alpha_for_opaque_formats)
{
    (void) unused;

    if (format >= (int) transcoder_texture_format::cTFTotalTextureFormats)
        return 0;

    const auto transcoder_format = static_cast<transcoder_texture_format>(format);

    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file.data(), m_file.size(), image_index, level_index, orig_width, orig_height, total_blocks))
        return 0;

    uint32_t flags = get_alpha_for_opaque_formats ? cDecodeFlagsTranscodeAlphaDataToOpaqueFormats : 0;

    uint32_t status;

    if (basis_transcoder_format_is_uncompressed(transcoder_format)) {
        const uint32_t bytes_per_pixel = basis_get_uncompressed_bytes_per_pixel(transcoder_format);
        const uint32_t bytes_per_line = orig_width * bytes_per_pixel;
        const uint32_t bytes_per_slice = bytes_per_line * orig_height;

        dst_data.resize(bytes_per_slice);

        status = m_transcoder.transcode_image_level(
            m_file.data(), m_file.size(), image_index, level_index,
            dst_data.data(), orig_width * orig_height,
            transcoder_format,
            flags,
            orig_width,
            nullptr,
            orig_height);
    } else {
        uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(transcoder_format);

        uint32_t required_size = total_blocks * bytes_per_block;

        if (transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGB || transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGBA) {
            // For PVRTC1, Basis only writes (or requires) total_blocks * bytes_per_block. But GL requires extra padding for very small textures:
            // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
            // The transcoder will clear the extra bytes followed the used blocks to 0.
            const uint32_t width = (orig_width + 3) & ~3;
            const uint32_t height = (orig_height + 3) & ~3;
            required_size = (std::max(8U, width) * std::max(8U, height) * 4 + 7) / 8;
            assert(required_size >= total_blocks * bytes_per_block);
        }

        dst_data.resize(required_size);

        status = m_transcoder.transcode_image_level(
            m_file.data(), m_file.size(), image_index, level_index,
            dst_data.data(), dst_data.size() / bytes_per_block,
            static_cast<basist::transcoder_texture_format>(format),
            flags);
    }
    return status;
}
}
