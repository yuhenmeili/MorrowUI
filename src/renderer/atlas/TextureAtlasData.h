//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_TEXTUREATLASDATA_H_
#define MORROW_RENDERER_TEXTUREATLASDATA_H_

#include <array>
#include <sstream>
#include <memory>
#include <vector>
#ifdef OPENGL_GLFW
    #include "platform/wgl/OpenglHeader.h"
#else
    #include "platform/egl/GLESHeader.h"
#endif
#include "DriverEnums.h"

namespace morrow
{
struct Page
{
    std::string name;
    int32_t width, height;
    bool useMipMaps;
    PixelDataFormat format = PixelDataFormat::RGBA;
    SamplerMinFilter minFilter = SamplerMinFilter::NEAREST;
    SamplerMagFilter magFilter = SamplerMagFilter::NEAREST;
    GLint uWrap = GL_CLAMP_TO_EDGE, vWrap = GL_CLAMP_TO_EDGE;
    bool pma;
};

using PageSharedPtr = std::shared_ptr<Page>;

struct Region
{
    PageSharedPtr page;
    std::string name;
    int32_t index = -1;
    int32_t left, top, width, height;

    float offsetX, offsetY;
    int32_t originalWidth, originalHeight;
    int32_t degrees;
    bool rotate;
    std::vector<std::string> names;
    std::vector<std::vector<int32_t>> values;
    bool flip;

    std::vector<int32_t> findValue(std::string& name);
};

using RegionSharedPtr = std::shared_ptr<Region>;

class TextureAtlasData
{
public:
    ~TextureAtlasData();

    void load(const std::string& packFileUrl, bool flip);

    std::vector<PageSharedPtr>& getPages();

    std::vector<RegionSharedPtr>& getRegions();

    std::basic_string<char> trimString(std::basic_string<char> str);

    int32_t readEntry(std::vector<std::string>& entry, std::string& line);

private:
    std::vector<PageSharedPtr> pages;
    std::vector<RegionSharedPtr> regions;
};

} // MORROWGUI

#endif //MORROW_RENDERER_TEXTUREATLASDATA_H_
