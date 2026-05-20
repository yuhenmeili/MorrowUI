//
// Created by lance on 24-6-28.
//

#ifndef MORROW_CORE_BASISTEXTURELOADER_H_
#define MORROW_CORE_BASISTEXTURELOADER_H_

#include <cstdio>
#include "DriverEnums.h"
#include "basis_universal/transcoder/basisu_transcoder.h"
using namespace basist;

namespace morrow {
struct basis_file_desc {
    uint32_t m_version;

    uint32_t m_us_per_frame;

    uint32_t m_total_images;

    uint32_t m_userdata0;
    uint32_t m_userdata1;

    // Type of texture (cETC1S or cUASTC4x4)
    uint32_t m_tex_format; // basis_tex_format

    bool m_y_flipped;
    bool m_has_alpha_slices;

    // ETC1S endpoint codebook
    uint32_t m_num_endpoints;
    uint32_t m_endpoint_palette_ofs;
    uint32_t m_endpoint_palette_len;

    // ETC1S selector codebook
    uint32_t m_num_selectors;
    uint32_t m_selector_palette_ofs;
    uint32_t m_selector_palette_len;

    // Huffman codelength tables
    uint32_t m_tables_ofs;
    uint32_t m_tables_len;
};

struct basis_image_desc {
    uint32_t m_orig_width;
    uint32_t m_orig_height;
    uint32_t m_num_blocks_x;
    uint32_t m_num_blocks_y;
    uint32_t m_num_levels;
    uint32_t m_width;
    uint32_t m_height;

    // Will be true if the image has alpha (for UASTC this may vary per-image)
    bool m_alpha_flag;
    bool m_iframe_flag;
};

struct basis_image_level_desc {
    // File offset/length of the compressed ETC1S or UASTC texture data.
    uint32_t m_rgb_file_ofs;
    uint32_t m_rgb_file_len;

    // Optional alpha data file offset/length - will be 0's for UASTC or opaque ETC1S files.
    uint32_t m_alpha_file_ofs;
    uint32_t m_alpha_file_len;
};

class BasisTextureLoader {
public:
    BasisTextureLoader();

    transcoder_texture_format resolveTextureFormat();

    bool isBasisuFormatSupported(transcoder_texture_format transcoderTextureFormat, basis_tex_format textureFormat);

    bool isTranscoderTextureFormatSupported(transcoder_texture_format transcoderTexFormat, basis_tex_format textureFormat);

    PixelDataFormat toGlTextureFormat(transcoder_texture_format transcoderTextureFormat);

    void load(const std::string& fileUrl, basisu::vector<uint8_t>& result, PixelDataFormat& glFormat);

    void close();

    uint32_t getHasAlpha();

    uint32_t getNumImages();

    uint32_t getNumLevels(uint32_t image_index);

    uint32_t getImageWidth(uint32_t image_index, uint32_t level_index);

    uint32_t getImageHeight(uint32_t image_index, uint32_t level_index);

    basis_file_desc getFileDesc();

    basis_image_desc getImageDesc(uint32_t image_index);

    basis_image_level_desc getImageLevelDesc(uint32_t image_index, uint32_t level_index);

    uint32_t getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format);

    bool isUASTC();

    uint32_t startTranscoding();

    uint32_t transcodeImage(basisu::vector<uint8_t>& dst_data, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t unused, uint32_t get_alpha_for_opaque_formats);

private:
    basisu_transcoder m_transcoder;
    basisu::vector<uint8_t> m_file;
};
}
#endif //MORROW_CORE_BASISTEXTURELOADER_H_
