//
// Created by lance on 2025/10/2.
//

#ifndef MORROW_GUI_MRTEXTBUTTON_H
#define MORROW_GUI_MRTEXTBUTTON_H
#include <memory>
#include <string>

#include "MRLabel.h"
#include "Texture.h"
#include "base/BaseButton.h"

namespace morrow {
/// 支持文字、背景、边框和交互状态样式的按钮组件。
class MRButton : public BaseButton {
public:
    /// 创建一个文字按钮。
    static std::shared_ptr<MRButton> create();

    // 文本相关方法
    /// 设置按钮文字和字体名称。
    void setText(const std::wstring& text, const std::string& fontName);

    /// 设置按钮文字字号。
    void setTextFontSize(float fontSize);

    /// 设置按钮文字颜色。
    void setTextColor(const Vector4& color);

    /// 使用 RGBA 分量设置按钮文字颜色。
    void setTextColor(float r, float g, float b, float a);

    // 背景颜色相关方法
    /// 设置按钮背景图片。
    void setBackgroundImage(TextureSharedPtr texture);

    /// 设置按钮默认状态的背景颜色。
    void setBackgroundColor(const Vector4& color);

    /// 使用 RGBA 分量设置按钮默认状态的背景颜色。
    void setBackgroundColor(float r, float g, float b, float a);

    /// 设置按钮悬停状态的背景颜色。
    void setHoverColor(const Vector4& color);

    /// 设置按钮按下状态的背景颜色。
    void setPressedColor(const Vector4& color);

    /// 设置按钮禁用状态的背景颜色。
    void setDisabledColor(const Vector4& color);

    // 边框相关方法
    /// 设置按钮边框颜色。
    void setBorderColor(const Vector4& color);

    /// 设置按钮边框宽度。
    void setBorderWidth(float width);

    /// 设置按钮圆角半径。
    void setCornerRadius(float radius);

    // 文本布局相关方法 (代理到MRLabel)
    /// 设置按钮文字的水平和垂直对齐方式。
    void setTextAlign(HorizontalAlignment horizontal, VerticalAlignment vertical);

    /// 设置按钮文字是否自动换行。
    void setAutoWrap(bool enable);

    /// 设置按钮文字的字符间距。
    void setCharacterSpacing(float spacing);

    /// 设置按钮文字的行间距。
    void setLineSpacing(float spacing);

    /// 设置按钮文字允许显示的最大行数；0 表示不限制。
    void setMaxLines(int maxLines);

    // 尺寸相关
    /// 获取根据文字内容计算出的首选尺寸。
    Vector2 getPreferredSize() const;

    /// 每帧更新按钮背景、边框和文字绘制状态。
    void update(FrameStateSharedPtr frameState) override;

protected:
    MRButton();

    void updateVisualState() override;

private:
    void createBackgroundMesh();

    void createBorderMesh();

    void addBorderQuad(float left, float top, float right, float bottom);

    void applyCurrentColors();

protected:
    std::shared_ptr<MRLabel> m_label;

    // 底图
    TextureSharedPtr m_backgroundTexture;
    bool m_hasBackgroundImage = false;

    // 颜色配置
    Vector4 m_backgroundColorNormal = Vector4(0.2f, 0.4f, 0.8f, 1.0f);    // 蓝色
    Vector4 m_backgroundColorHover = Vector4(0.3f, 0.5f, 0.9f, 1.0f);     // 亮蓝色;
    Vector4 m_backgroundColorPressed = Vector4(0.1f, 0.3f, 0.7f, 1.0f);   // 暗蓝色;
    Vector4 m_backgroundColorDisabled = Vector4(0.5f, 0.5f, 0.5f, 0.5f);  // 灰色半透明;
    Vector4 m_borderColor = Vector4(0.1f, 0.1f, 0.1f, 1.0f);

    // 当前状态颜色
    Vector4 m_currentBackgroundColor = m_backgroundColorNormal;
    Vector4 m_currentTextColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

    // 边框和圆角
    float m_borderWidth = 0.0f;
    float m_cornerRadius = 0.0f;

    // 网格数据
    std::vector<Vector3> m_backgroundVertices;
    std::vector<Vector2> m_backgroundUVs;
    std::vector<int16_t> m_backgroundIndices;

    std::vector<Vector3> m_borderVertices;
    std::vector<Vector2> m_borderUVs;
    std::vector<int16_t> m_borderIndices;

    // 脏标记
    bool m_isBackgroundDirty = true;
    bool m_isBorderDirty = true;
};
}  // namespace morrow
#endif  // MORROW_GUI_MRTEXTBUTTON_H
