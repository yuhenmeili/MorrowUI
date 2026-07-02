// #define STB_TRUETYPE_IMPLEMENTATION

#include <fstream>
#include "FontManager.h"
#include "core/ToolUtils.h"
#include "utils/Log.h"

namespace morrow
{
void FontManager::initialize() {
    FontInfo fontInfo = {
        .name = "MorrowSansCN1.1-Regular.otf",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
        .fontSize = 32.0f
    };
    addFonts({fontInfo});
}

void FontManager::addFonts(const std::vector<FontInfo>& fontsConfig)
{
    for (const auto& info : fontsConfig) {
        if (!info.path.empty()) {
            if (m_fontsConfig.find(info.name) != m_fontsConfig.end()) {
                LOG_W("font {} already exists, skip loading", info.name);
                continue;
            }
            DynamicFontSharedPtr dynamicFont = std::make_shared<DynamicFont>();
            if (!dynamicFont->LoadFromFile(info.path, info.fontSize)) {
                LOG_E("load font {} error", info.path);
                continue;
            }
            dynamicFont->SetAntialiasingQuality(2);
            dynamicFont->SetCharacterSpacing(0.5f);
            m_fontsConfig[info.name] = std::move(dynamicFont);
        } else {
            LOG_I("fontUrl is empty");
        }
    }
}

DynamicFontSharedPtr FontManager::getFont(const std::string& fontName) {
    if (!fontName.empty()) {
        auto it = m_fontsConfig.find(fontName);
        if (it != m_fontsConfig.end()) {
            return it->second;
        }
    }

    if (m_fontsConfig.empty()) {
        return nullptr;
    }

    return m_fontsConfig.begin()->second;
}

// std::shared_ptr<unsigned char> FontManager::getTextBitmap(const TextTextureInfoSharedPtr& textInfo, int32_t textWidth, int32_t textHeight)
// {
//     if (m_fontsConfig.find(textInfo->fontName) == m_fontsConfig.end()) {
//         return nullptr;
//     }
//     const stbtt_fontinfo* info = m_fontsConfig[textInfo->fontName];
//     if (!info) {
//         return nullptr;
//     }
//     int32_t b_w = textWidth; /* bitmap width */
//     int32_t b_h = textHeight; /* bitmap height */
//     int32_t l_h = textInfo->fontSize; /* line height */
//     if(b_w <= 0 || b_h <= 0 || l_h <= 0) {
//         return nullptr;
//     }
//
//     /* create a bitmap for the phrase */
//     unsigned char* bitmap = (unsigned char*) calloc(b_w * b_h, sizeof(unsigned char));
//
//     /* calculate font scaling */
//     float scale = stbtt_ScaleForPixelHeight(info, l_h);
//     int32_t rowCount = 1;
//     int32_t x = textInfo->enableWrap ? (b_w - textInfo->textWarpWidthMap[rowCount])/2 : 1;
//     int32_t ascent, descent, lineGap;
//     stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
//     ascent = roundf(ascent * scale);
//     descent = roundf(descent * scale);
//
//     std::wstring letters = ToolUtils::s2ws(textInfo->text);
//     int32_t lettersLen = letters.size();
//     for (int32_t i = 0; i < lettersLen; ++i) {
//         /* how wide is this character */
//         int32_t ax;
//         int32_t lsb;
//         stbtt_GetCodepointHMetrics(info, letters[i], &ax, &lsb);
//         int32_t letterWidth = 0;
//         letterWidth += roundf(ax * scale);
//
//         /* add kerning */
//         int32_t kern = stbtt_GetCodepointKernAdvance(info, letters[i], letters[i + 1]);
//         letterWidth += roundf(kern * scale);
//
//         int32_t remainingWordWidth = 0;
//         if (m_language == Language::CHINESE) {
//             //do nothing
//         }
//         else if (m_language == Language::ENGLISH) {
//             if (!isspace(letters[i])) {
//                 int32_t j = i + 1;
//                 int32_t advanceWidth;
//                 int32_t leftSideBearing;
//                 while (!isspace(letters[j]) && j < lettersLen) {
//                     stbtt_GetCodepointHMetrics(info, letters[j], &advanceWidth, &leftSideBearing);
//
//                     /* advance x */
//                     remainingWordWidth += roundf(advanceWidth * scale);
//
//                     /* add kerning */
//                     kern = stbtt_GetCodepointKernAdvance(info, letters[j], letters[j + 1]);
//                     remainingWordWidth += roundf(kern * scale);
//                     ++j;
//                 }
//             }
//         }
//
//         if (textInfo->enableWrap && (x + letterWidth + remainingWordWidth + 1 > b_w)) {
//             rowCount++;
//             //居中
//             x = (b_w - textInfo->textWarpWidthMap[rowCount])/2;
//         }
//         /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character word[i].) */
//         /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
//         int32_t c_x1, c_y1, c_x2, c_y2;
//         stbtt_GetCodepointBitmapBox(info, letters[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
//         /* compute y (different characters have different heights) */
//         int32_t y = ascent + c_y1;
//         /* render character (stride and offset is important here) */
//         int32_t byteOffset = x + roundf(lsb * scale) + ((y + (rowCount - 1) * (textInfo->fontSize + textInfo->lineSpacing)) * b_w);
//         stbtt_MakeCodepointBitmap(info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, letters[i]);
//
//         x += letterWidth;
//     }
// //    TextureFromFile::save("/data/log/STB.png", b_w, b_h, 1, bitmap, 0);
//
//     return std::shared_ptr<unsigned char>(bitmap, std::default_delete<unsigned char[]>());
// }

int32_t FontManager::getTextWidth(const TextTextureInfoSharedPtr& textInfo)
{
    return 0;
    // if (m_fontsConfig.find(textInfo->fontName) == m_fontsConfig.end()) {
    //     return 0;
    // }
    // const stbtt_fontinfo* info = m_fontsConfig[textInfo->fontName];
    // if (!info) {
    //     return 0;
    // }
    // if (textInfo->enableWrap) {
    //     return textInfo->destWidth;
    // }
    // return getTextWidthNoWrap(textInfo);
}

int32_t FontManager::getTextHeight(const TextTextureInfoSharedPtr& textInfo)
{
    return 0;
    // if (m_fontsConfig.find(textInfo->fontName) == m_fontsConfig.end()) {
    //     return 0;
    // }
    // const stbtt_fontinfo* info = m_fontsConfig[textInfo->fontName];
    // if (!info) {
    //     return 0;
    // }
    // /* line height */
    // int32_t l_h = textInfo->fontSize;
    // /* calculate font scaling */
    // float scale = stbtt_ScaleForPixelHeight(info, l_h);
    // int32_t ascent, descent, lineGap;
    // stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
    // ascent = roundf(ascent * scale);
    // descent = roundf(descent * scale);
    // int32_t letterHeight = ascent - descent + lineGap;
    //
    // if (textInfo->enableWrap) {
    //     textInfo->textWarpWidthMap.clear();
    //     //max width
    //     int32_t destWidth = textInfo->destWidth;
    //
    //     //边缘预留两个长度单位
    //     int32_t x = 1;
    //     int32_t rowCount = 1;
    //
    //     std::wstring letters = ToolUtils::s2ws(textInfo->text);
    //     int32_t lettersLen = letters.size();
    //     for (int32_t i = 0; i < lettersLen; ++i) {
    //         /* how wide is this character */
    //         int32_t letterWidth = 0;
    //         int32_t ax;
    //         int32_t lsb;
    //         stbtt_GetCodepointHMetrics(info, letters[i], &ax, &lsb);
    //
    //         /* advance x */
    //         letterWidth += roundf(ax * scale);
    //
    //         /* add kerning */
    //         int32_t kern = stbtt_GetCodepointKernAdvance(info, letters[i], letters[i + 1]);
    //         letterWidth += roundf(kern * scale);
    //
    //         if(m_language == CHINESE) {
    //             //do nothing
    //         }
    //         else if(m_language == Language::ENGLISH) {
    //             if(!isspace(letters[i])) {
    //                 int32_t j = i + 1;
    //                 while (!isspace(letters[j]) && j < lettersLen) {
    //                     stbtt_GetCodepointHMetrics(info, letters[j], &ax, &lsb);
    //
    //                     /* advance x */
    //                     letterWidth += roundf(ax * scale);
    //
    //                     /* add kerning */
    //                     kern = stbtt_GetCodepointKernAdvance(info, letters[j], letters[j + 1]);
    //                     letterWidth += roundf(kern * scale);
    //                     ++j;
    //                 }
    //                 i = j - 1;
    //             }
    //         }
    //
    //         if (x + letterWidth + 1 > destWidth) {
    //             textInfo->textWarpWidthMap.insert(std::pair<int32_t, int32_t>(rowCount, x + 1));
    //             x = 1;
    //             ++rowCount;
    //         }
    //         x += letterWidth;
    //     }
    //     textInfo->textWarpWidthMap.insert(std::pair<int32_t, int32_t>(rowCount, x + 1));
    //     return rowCount * l_h + textInfo->lineSpacing * (rowCount - 1);
    // } else {
    //     return letterHeight;
    // }
}

int32_t FontManager::getTextWidthNoWrap(const TextTextureInfoSharedPtr& textInfo)
{
    return 0;
    // if (m_fontsConfig.find(textInfo->fontName) == m_fontsConfig.end()) {
    //     return 0;
    // }
    // const stbtt_fontinfo* info = m_fontsConfig[textInfo->fontName];
    // if (!info) {
    //     return 0;
    // }
    // int32_t l_h = textInfo->fontSize; /* line height */
    // /* calculate font scaling */
    // float scale = stbtt_ScaleForPixelHeight(info, l_h);
    //
    // //边缘预留两个长度单位
    // int32_t x = 1;
    // int32_t ascent, descent, lineGap;
    // stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
    // ascent = roundf(ascent * scale);
    // descent = roundf(descent * scale);
    //
    // std::wstring word = ToolUtils::s2ws(textInfo->text);
    // int32_t wordLen = word.size();
    // for (int32_t i = 0; i < wordLen; ++i) {
    //     /* how wide is this character */
    //     int32_t ax;
    //     int32_t lsb;
    //     stbtt_GetCodepointHMetrics(info, word[i], &ax, &lsb);
    //
    //     /* advance x */
    //     x += roundf(ax * scale);
    //
    //     /* add kerning */
    //     int32_t kern;
    //     kern = stbtt_GetCodepointKernAdvance(info, word[i], word[i + 1]);
    //     x += roundf(kern * scale);
    // }
    // return x + 1;
}
}
