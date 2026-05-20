#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "DynamicFont.h"
#include "TextTextureInfo.h"
#include "../core/Language.h"

namespace morrow
{
struct FontInfo {
    std::string name;
    std::string path;
    float fontSize = 32.0f;
};

class FontManager
{
public:
    FontManager() = default;

    void initialize();

    void addFonts(const std::vector<FontInfo>& fontsConfig);

    DynamicFontSharedPtr getFont(const std::string& fontName);

    // std::shared_ptr<unsigned char> getTextBitmap(const TextTextureInfoSharedPtr& textInfo, int32_t textWidth, int32_t textHeight);

    int32_t getTextWidth(const TextTextureInfoSharedPtr& textInfo);

    int32_t getTextHeight(const TextTextureInfoSharedPtr& textInfo);

    int32_t getTextWidthNoWrap(const TextTextureInfoSharedPtr& textInfo);

private:
    std::unordered_map<std::string, DynamicFontSharedPtr> m_fontsConfig;
    Language m_language = Language::CHINESE;
};

using FontManagerSharedPtr = std::shared_ptr<FontManager>;

}

#endif /* FONT_MANAGER_H_ */
