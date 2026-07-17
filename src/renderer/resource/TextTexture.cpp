//
// Created by lance on 2023/10/17.
//

#include "TextTexture.h"

#include "FontManager.h"
#include "GlobalObject.h"

namespace morrow
{
TextTextureSharedPtr TextTexture::create(TextTextureInfoSharedPtr textTextureInfo)
{
    TextTextureSharedPtr textTexturePtr(new TextTexture(textTextureInfo));
    return textTexturePtr;
}

TextTexture::TextTexture(TextTextureInfoSharedPtr textTextureInfo) : Texture(ImageType::TEXT)
{
    m_textInfo = textTextureInfo;
}

void TextTexture::update()
{
    if (m_textInfo->attributeNeedUpdate) {
        m_textInfo->attributeNeedUpdate = false;
        int32_t imageWidth = GlobalObject::getInstance().getFontManager()->getTextWidth(m_textInfo);
        int32_t imageHeight = GlobalObject::getInstance().getFontManager()->getTextHeight(m_textInfo);
        // std::shared_ptr<unsigned char> bitmap = GlobalObject::getInstance().getFontManager()->getTextBitmap(m_textInfo, imageWidth, imageHeight);
        // if (bitmap) {
        //     setTextureData(bitmap, imageWidth, imageHeight, PixelDataFormat::ALPHA, 0, false);
        // }
    }
}

TextTexture& TextTexture::setFontName(const std::string& fontName)
{
    if (m_textInfo->fontName != fontName) {
        m_textInfo->fontName = fontName;
        m_textInfo->attributeNeedUpdate = true;
    }
    return *this;
}

TextTexture& TextTexture::setText(const std::string& text)
{
    if (m_textInfo->text != text) {
        m_textInfo->text = text;
        m_textInfo->attributeNeedUpdate = true;
    }
    return *this;
}

TextTexture& TextTexture::setFontSize(int32_t fontSize)
{
    if (m_textInfo->fontSize != fontSize) {
        m_textInfo->fontSize = fontSize;
        m_textInfo->attributeNeedUpdate = true;
    }
    return *this;
}

TextTexture& TextTexture::setLineSpacing(int32_t lineSpacing)
{
    if (m_textInfo->lineSpacing != lineSpacing) {
        m_textInfo->lineSpacing = lineSpacing;
        m_textInfo->attributeNeedUpdate = true;
    }
    return *this;
}

TextTexture& TextTexture::setWrapEnabled(bool enableWrap)
{
    if (m_textInfo->enableWrap != enableWrap) {
        m_textInfo->enableWrap = enableWrap;
        m_textInfo->attributeNeedUpdate = true;
    }
    return *this;
}

int32_t TextTexture::getTextWidth()
{
    return GlobalObject::getInstance().getFontManager()->getTextWidth(m_textInfo);
}

int32_t TextTexture::getTextHeight()
{
    return GlobalObject::getInstance().getFontManager()->getTextHeight(m_textInfo);
}

int32_t TextTexture::getTextWidthNoWrap()
{
    return GlobalObject::getInstance().getFontManager()->getTextWidthNoWrap(m_textInfo);
}

bool TextTexture::isAttributeNeedUpdate()
{
    return m_textInfo->attributeNeedUpdate;
}

bool TextTexture::isWrapEnabled()
{
    return m_textInfo->enableWrap;
}

TextTexture& TextTexture::setDestWidth(int32_t destWidth)
{
    m_textInfo->destWidth = destWidth;
    return *this;
}

TextTexture& TextTexture::setDestHeight(int32_t destHeight)
{
    m_textInfo->destHeight = destHeight;
    return *this;
}

int32_t TextTexture::getFontSize()
{
    return m_textInfo->fontSize;
}

int32_t TextTexture::getLineSpacing()
{
    return m_textInfo->lineSpacing;
}

const std::string& TextTexture::getText() const
{
    return m_textInfo->text;
}
} // MORROWGUI