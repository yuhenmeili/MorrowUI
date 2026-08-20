//
// Created by 0060328 on 25-10-9.
//

#ifndef MRTEXTRENDERER_H
#define MRTEXTRENDERER_H
#include <memory>
#include <cstdint>

#include "fonts/DynamicFont.h"
#include "base/UIWidget.h"

namespace morrow {
/// 单行文本布局结果。
struct TextLine {
    /// 当前行包含的字符。
    std::vector<wchar_t> characters;
    /// 当前行的测量宽度。
    float width = 0.0f;
    /// 当前行经过对齐计算后的水平偏移。
    float x_offset = 0.0f;
    /// 当前行经过布局计算后的垂直偏移。
    float y_offset = 0.0f;
};

/// 支持对齐、换行、字符间距和行数限制的文本组件。
class MRLabel : public UIWidget {
public:
    /// 创建一个空文本组件。
    MRLabel();

    /// 设置显示文字和字体名称。
    void setText(const std::wstring& text, const std::string& fontName = "default");

    /// 设置文字字号。
    void setFontSize(float fontSize);

    /// 获取当前文字字号。
    float getFontSize() const;

    /// 使用 RGBA 分量设置文字颜色。
    void setFontColor(float r, float g, float b, float a);

    /// 设置文字颜色。
    void setFontColor(const Vector4& color);

    /// 设置文字的水平和垂直对齐方式。
    void setAlign(HorizontalAlignment horizontalAlignment, VerticalAlignment verticalAlignment);

    // 新增功能
    /// 设置文字是否根据组件宽度自动换行。
    void setAutoWrap(bool enable);

    /// 设置字符间距。
    void setCharacterSpacing(float spacing);

    /// 设置行间距。
    void setLineSpacing(float spacing);

    /// 设置允许显示的最大行数；0 表示不限制。
    void setMaxLines(int maxLines);

    // 文本测量
    /// 获取当前文本布局占用的宽度和高度。
    Vector2 getTextExtents() const;

    /// 每帧按需更新文本布局、对齐和字形网格。
    void update(FrameStateSharedPtr frameState) override;

private:
    void processTextLayout();

    void applyAlignment();

    void createTextMesh();

    void renderGlyphQuad(const FontGlyph* glyph, float x, float baselineY, std::vector<Vector3>& gearsVertices, std::vector<Vector2>& gearsUVs, std::vector<int16_t>& gearsIndices);

    std::shared_ptr<DynamicFont> m_font;
    // DynamicFont 扩容后会替换 FontTexture 并重新计算字形 UV。
    uint64_t m_fontAtlasVersion = 0;
    std::wstring m_text;
    std::string m_fontName;
    float m_fontSize = 32.0f;
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
