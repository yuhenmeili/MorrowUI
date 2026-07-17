//
// Created by lance on 2023/10/17.
//

#ifndef MORROW_CORE_TEXTTEXTURE_H_
#define MORROW_CORE_TEXTTEXTURE_H_

#include "Texture.h"
#include "TextTextureInfo.h"

namespace morrow
{
class TextTexture;
using TextTextureSharedPtr = std::shared_ptr<TextTexture>;
class TextTexture : public Texture
{
public:
    static TextTextureSharedPtr create(TextTextureInfoSharedPtr textTextureInfo);

    void update();

    TextTexture& setFontName(const std::string& fontName);

    TextTexture& setText(const std::string& text);

    TextTexture& setFontSize(int32_t fontSize);

    TextTexture& setLineSpacing(int32_t lineSpacing);

    TextTexture& setWrapEnabled(bool enableWrap);

    TextTexture& setDestWidth(int32_t destWidth);

    TextTexture& setDestHeight(int32_t destHeight);

    int32_t getTextWidth();

    int32_t getTextHeight();

    int32_t getTextWidthNoWrap();

    bool isAttributeNeedUpdate();

    bool isWrapEnabled();

    int32_t getFontSize();

    int32_t getLineSpacing();

    const std::string& getText() const;

private:
    TextTexture(TextTextureInfoSharedPtr textTextureInfo);

private:
    TextTextureInfoSharedPtr m_textInfo;
};

} // MORROWGUI

#endif //MORROW_CORE_TEXTTEXTURE_H_
