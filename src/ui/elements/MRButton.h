//
// Created by lance on 2025/10/2.
//

#ifndef MORROW_GUI_MRTEXTBUTTON_H
#define MORROW_GUI_MRTEXTBUTTON_H
#include "MRLabel.h"
#include <memory>
#include <string>

#include "base/BaseButton.h"

namespace morrow {

class MRButton : public BaseButton {
public:
    static std::shared_ptr<MRButton> create();
    // 文本相关方法
    void setText(const std::wstring& text, const std::string& fontName);

    void setTextColor(const Vector4& color);

    void setTextColor(float r, float g, float b, float a);

    // 背景颜色相关方法
    void setBackgroundColor(const Vector4& color);

    void setBackgroundColor(float r, float g, float b, float a);

    void setHoverColor(const Vector4& color);

    void setPressedColor(const Vector4& color);

    void setDisabledColor(const Vector4& color);

    // 边框相关方法
    void setBorderColor(const Vector4& color);

    void setBorderWidth(float width);

    void setCornerRadius(float radius);

    // 文本布局相关方法 (代理到MRLabel)
    void setTextAlign(HorizontalAlignment horizontal, VerticalAlignment vertical);

    void setAutoWrap(bool enable);

    void setCharacterSpacing(float spacing);

    void setLineSpacing(float spacing);

    void setMaxLines(int maxLines);

    // 尺寸相关
    Vector2 getPreferredSize() const;

    void update(FrameStateSharedPtr frameState) override;
private:
    MRButton();

    void updateVisualState() override;

    void createBackgroundMesh();

    void createBorderMesh();

    void addBorderQuad(float left, float top, float right, float bottom);

    void applyCurrentColors();

private:
    std::shared_ptr<MRLabel> m_label;

    // 颜色配置
    Vector4 m_backgroundColorNormal = Vector4(0.2f, 0.4f, 0.8f, 1.0f); // 蓝色
    Vector4 m_backgroundColorHover = Vector4(0.3f, 0.5f, 0.9f, 1.0f); // 亮蓝色;
    Vector4 m_backgroundColorPressed = Vector4(0.1f, 0.3f, 0.7f, 1.0f); // 暗蓝色;
    Vector4 m_backgroundColorDisabled = Vector4(0.5f, 0.5f, 0.5f, 0.5f); // 灰色半透明;
    Vector4 m_borderColor = Vector4(0.1f, 0.1f, 0.1f, 1.0f);;

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
} // namespace morrow
#endif //MORROW_GUI_MRTEXTBUTTON_H
