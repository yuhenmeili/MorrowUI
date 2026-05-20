//
// Created by 0060328 on 25-10-9.
//

#ifndef MRTEXTRENDERER_H
#define MRTEXTRENDERER_H
#include <memory>

#include "../fonts/DynamicFont.h"
#include "base/UIWidget.h"

namespace morrow {
struct TextLine {
    std::vector<wchar_t> characters;
    float width = 0.0f;
    float x_offset = 0.0f;
    float y_offset = 0.0f;
};

class MRLabel : public UIWidget {
public:
    MRLabel();

    void setText(const std::wstring& text, const std::string& fontName = "default");

    void setFontColor(float r, float g, float b, float a);

    void setFontColor(const Vector4& color);

    void setAlign(HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAlignment);

    // 新增功能
    void setAutoWrap(bool enable);

    void setCharacterSpacing(float spacing);

    void setLineSpacing(float spacing);

    void setMaxLines(int maxLines);

    // 文本测量
    Vector2 getTextExtents() const;

    void update(FrameStateSharedPtr frameState) override;

private:
    void processTextLayout();

    void applyAlignment();

    void createTextMesh();

    void renderGlyphQuad(const FontGlyph* glyph, float x, float baselineY, std::vector<Vector3>& gearsVertices, std::vector<Vector2>& gearsUVs, std::vector<int16_t>& gearsIndices);

    std::shared_ptr<DynamicFont> m_font;
    std::wstring m_text;
    std::string m_fontName;
    std::vector<TextLine> m_lines;
    // 布局相关
    float m_textWidth = 0.0f;
    float m_textHeight = 0.0f;
    bool m_autoWrap = false;
    float m_characterSpacing = 0.5f;
    float m_lineSpacing = 1.0f;
    int m_maxLines = 0;
    bool m_isTextLayoutDirty = true;
    bool m_isAlignDirty = true;
    // 原始顶点数据，用于对齐调整
    std::vector<Vector3> m_originalVertices;

    HorizontalAlignment m_horizontalAlignment = HorizontalAlignment::LEFT;
    VerticalAlignment m_verticalAlignment = VerticalAlignment::TOP;
};
} // morrow

#endif //MRTEXTRENDERER_H
