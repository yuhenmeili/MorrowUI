//
// Created by lance on 26-9-15.
//

#ifndef MORROW_CORE_KTX2TEXTURELOADER_H_
#define MORROW_CORE_KTX2TEXTURELOADER_H_

#include <cstddef>
#include <string>
#include "DriverEnums.h"
#include "basis_universal/transcoder/basisu_transcoder.h"

namespace morrow {
/// KTX2 纹理加载器：解析 BasisU 压缩的 KTX2 文件。
///
/// 优先转码为硬件支持的 GPU 压缩格式（带 alpha 走 ETC2_RGBA、不带 alpha
/// 走 ETC1_RGB），硬件不支持时回退到未压缩格式，由 isCompressed() 区分。
class Ktx2TextureLoader {
public:
    Ktx2TextureLoader();

    /// 加载并转码 level0；像素数据写入 result，像素格式写回 glFormat。
    bool load(const std::string& fileUrl, basisu::vector<uint8_t>& result, PixelDataFormat& glFormat);

    /// 从内存中的 KTX2 编码字节加载并转码，语义同 load。
    bool loadFromMemory(const uint8_t* fileData, size_t fileSize, basisu::vector<uint8_t>& result, PixelDataFormat& glFormat);

    /// 当前解析出的目标格式是否为 GPU 压缩格式（决定上传走哪个 GL 入口）。
    bool isCompressed() const;

    int32_t getWidth() const;

    int32_t getHeight() const;

private:
    /// 对 m_file 中已就位的编码字节做初始化与转码，load/loadFromMemory 的公共后段。
    bool decode(basisu::vector<uint8_t>& result, PixelDataFormat& glFormat);

    /// 按硬件支持与源格式选择转码目标格式。
    basist::transcoder_texture_format resolveTextureFormat();

    bool isBasisuFormatSupported(basist::transcoder_texture_format transcoderTextureFormat, basist::basis_tex_format textureFormat);

    PixelDataFormat toGlTextureFormat(basist::transcoder_texture_format transcoderTextureFormat);

    basist::ktx2_transcoder m_transcoder;
    basisu::vector<uint8_t> m_file;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_compressed = false;
};
}
#endif //MORROW_CORE_KTX2TEXTURELOADER_H_
