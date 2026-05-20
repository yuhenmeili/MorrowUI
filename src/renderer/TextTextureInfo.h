//
// Created by lance on 24-8-14.
//

#ifndef MORROW_RENDERER_TEXTTEXTUREINFO_H_
#define MORROW_RENDERER_TEXTTEXTUREINFO_H_

#include <map>
#include "Vector4.h"

namespace morrow
{
using namespace Math;
enum class TEXT_HORIZONTAL_ALIGNMENT
{
    LEFT = 0,
    MIDDLE = 1,
    RIGHT = 2
};

enum class TEXT_VERTICAL_ALIGNMENT
{
    TOP = 0,
    MIDDLE = 1,
    BOTTOM = 2
};

struct TextTextureInfo
{
    std::string fontName;
    std::string text;
    int32_t fontSize = 32;
    int32_t lineSpacing = 0;
    bool enableWrap = false;
    int32_t destWidth = 0;
    int32_t destHeight = 0;
    std::map<int32_t, int32_t> textWarpWidthMap;

    bool attributeNeedUpdate = true;
};
using TextTextureInfoSharedPtr = std::shared_ptr<TextTextureInfo>;

struct TextTextureRenderInfo : public TextTextureInfo
{
    Vector4 textColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 textBackgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};

    TEXT_HORIZONTAL_ALIGNMENT horizontalAlignment = TEXT_HORIZONTAL_ALIGNMENT::LEFT;
    TEXT_VERTICAL_ALIGNMENT verticalAlignment = TEXT_VERTICAL_ALIGNMENT::TOP;
    bool alignmentNeedUpdate = true;
};

using TextTextureRenderInfoSharedPtr = std::shared_ptr<TextTextureRenderInfo>;
}
#endif //MORROW_RENDERER_TEXTTEXTUREINFO_H_
