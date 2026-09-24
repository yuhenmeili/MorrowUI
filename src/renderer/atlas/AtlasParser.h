//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_ATLASPARSER_H_
#define MORROW_RENDERER_ATLASPARSER_H_

#include <cstddef>
#include <istream>
#include <memory>
#include <string>
#include <vector>
#ifdef OPENGL_GLFW
#include "platform/wgl/OpenglHeader.h"
#else
#include "platform/egl/GLESHeader.h"
#endif
#include "DriverEnums.h"

namespace morrow {
/// .atlas 文件中一页图集纹理的描述。
struct AtlasPage {
    std::string name;
    int32_t width = 0, height = 0;
    bool useMipMaps = false;
    PixelDataFormat format = PixelDataFormat::RGBA;
    SamplerMinFilter minFilter = SamplerMinFilter::NEAREST;
    SamplerMagFilter magFilter = SamplerMagFilter::NEAREST;
    GLint uWrap = GL_CLAMP_TO_EDGE, vWrap = GL_CLAMP_TO_EDGE;
    bool pma = false;
};

using AtlasPageSharedPtr = std::shared_ptr<AtlasPage>;

/// .atlas 文件中一帧的描述，与纹理无关的纯数据；由装配层转换为纹理区域。
struct AtlasFrame {
    AtlasPageSharedPtr page;
    std::string name;
    int32_t index = -1;
    int32_t left = 0, top = 0, width = 0, height = 0;
    float offsetX = 0.0f, offsetY = 0.0f;
    int32_t originalWidth = 0, originalHeight = 0;
    int32_t degrees = 0;
    bool rotate = false;
    std::vector<std::string> names;
    std::vector<std::vector<int32_t>> values;
    bool flip = false;

    std::vector<int32_t> findValue(const std::string& name) const;
};

using AtlasFrameSharedPtr = std::shared_ptr<AtlasFrame>;

/// .atlas 文本文件解析器：读取文件并产出页与帧的纯数据，不接触 GL 与纹理。
class AtlasParser {
public:
    /// 解析指定 .atlas 文件；flip 为真时标记所有帧在装配时做垂直翻转。
    void parse(const std::string& atlasFilePath, bool flip);

    /// 从内存中的 .atlas 文件字节解析（如网络下载或打包资源），语义同 parse。
    void parseBuffer(const unsigned char* atlasData, size_t size, bool flip);

    std::vector<AtlasPageSharedPtr>& getPages() {
        return m_pages;
    }

    std::vector<AtlasFrameSharedPtr>& getFrames() {
        return m_frames;
    }

private:
    /// 逐行解析核心，供 parse/parseBuffer 复用；sourceName 仅用于日志。
    void parseStream(std::istream& stream, bool flip, const std::string& sourceName);

    std::vector<AtlasPageSharedPtr> m_pages;
    std::vector<AtlasFrameSharedPtr> m_frames;
};
} // MORROWGUI

#endif //MORROW_RENDERER_ATLASPARSER_H_