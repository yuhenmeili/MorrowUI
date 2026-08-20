//
// Created by 0060328 on 25-10-11.
//

#ifndef MRRICHTEXTLABEL_H
#define MRRICHTEXTLABEL_H

#include <memory>
#include <string>
#include <vector>

#include "MRLabel.h"
#include "base/UIWidget.h"

namespace morrow {

class MRRichTextLabel;
using MRRichTextLabelSharedPtr = std::shared_ptr<MRRichTextLabel>;

/// 支持混合颜色、字号、换行、自动换行和对齐的富文本标签。
///
/// 当前支持 [color=#RRGGBB]、[color=#RRGGBBAA]、[font_size=N]、
/// [size=N]、[br]、[lb] 和 [rb] 标记，颜色与字号标记可以嵌套。
class MRRichTextLabel : public UIWidget {
public:
    /// 创建一个富文本标签。
    static MRRichTextLabelSharedPtr create();

    /// 设置富文本内容和字体名称。
    void setText(const std::wstring& text, const std::string& fontName = "default");

    /// 获取包含格式标记的原始文本。
    const std::wstring& getText() const {
        return m_text;
    }

    /// 获取移除已识别格式标记后的纯文本。
    const std::wstring& getPlainText() const {
        return m_plainText;
    }

    /// 在当前内容末尾追加富文本。
    void appendText(const std::wstring& text);

    /// 清空全部文本内容。
    void clear();

    /// 设置文本使用的字体名称。
    void setFontName(const std::string& fontName);

    /// 设置未指定字号标记时使用的默认字号。
    void setFontSize(float fontSize);

    /// 获取默认字号。
    float getFontSize() const {
        return m_fontSize;
    }

    /// 设置未指定颜色标记时使用的默认文字颜色。
    void setFontColor(const Vector4& color);

    /// 使用 RGBA 分量设置默认文字颜色。
    void setFontColor(float r, float g, float b, float a);

    /// 设置是否解析富文本标记；关闭时全部内容按普通文本显示。
    void setBbcodeEnabled(bool enabled);

    /// 获取当前是否解析富文本标记。
    bool isBbcodeEnabled() const {
        return m_bbcodeEnabled;
    }

    /// 设置文本是否根据组件宽度自动换行。
    void setAutoWrap(bool enabled);

    /// 设置字符间距。
    void setCharacterSpacing(float spacing);

    /// 设置行间距倍率。
    void setLineSpacing(float spacing);

    /// 设置允许显示的最大行数；0 表示不限制。
    void setMaxLines(int maxLines);

    /// 设置文本的水平和垂直对齐方式。
    void setAlign(HorizontalAlignment horizontal, VerticalAlignment vertical);

    /// 获取当前富文本布局占用的宽度和高度。
    Vector2 getTextExtents() const {
        return Vector2(m_textWidth, m_textHeight);
    }

    /// 每帧按需解析、布局并更新内部文本片段。
    void update(FrameStateSharedPtr frameState) override;

private:
    struct TextStyle {
        Vector4 color;
        float fontSize = 32.0f;
    };

    struct ParsedRun {
        std::wstring text;
        TextStyle style;
    };

    struct LayoutRun {
        std::wstring text;
        TextStyle style;
        float width = 0.0f;
        float ascent = 0.0f;
        float lineHeight = 0.0f;
    };

    struct LayoutLine {
        std::vector<LayoutRun> runs;
        float width = 0.0f;
        float ascent = 0.0f;
        float belowBaseline = 0.0f;
        float height = 0.0f;
    };

    MRRichTextLabel();

    static bool stylesEqual(const TextStyle& left, const TextStyle& right);

    void parseText();

    void rebuildLayout();

    void clearRunLabels();

    void markTextDirty(const char* caller);

    std::wstring m_text;
    std::wstring m_plainText;
    std::string m_fontName = "default";
    Vector4 m_fontColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    float m_fontSize = 32.0f;
    float m_characterSpacing = 0.5f;
    float m_lineSpacing = 1.0f;
    int m_maxLines = 0;
    bool m_bbcodeEnabled = true;
    bool m_autoWrap = false;
    bool m_layoutDirty = true;
    float m_textWidth = 0.0f;
    float m_textHeight = 0.0f;
    HorizontalAlignment m_horizontalAlignment = HorizontalAlignment::LEFT;
    VerticalAlignment m_verticalAlignment = VerticalAlignment::TOP;
    std::vector<ParsedRun> m_parsedRuns;
    std::vector<std::shared_ptr<MRLabel>> m_runLabels;
};

} // morrow

#endif //MRRICHTEXTLABEL_H
