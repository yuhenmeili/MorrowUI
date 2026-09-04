#ifndef MORROW_GUI_MRCHECKBOX_H
#define MORROW_GUI_MRCHECKBOX_H

#include "MRColor.h"
#include "MRSelectableButton.h"

namespace morrow {

class MRCheckBox : public MRSelectableButton {
public:
    /// 创建一个带勾选指示器的复选框。
    static std::shared_ptr<MRCheckBox> create();

    /// 设置复选框文字，并在文字变化后重新布局指示器。
    void setText(const std::wstring& text, const std::string& fontName) {
        MRButton::setText(text, fontName);
        layoutIndicator();
    }

    /// 设置指示器方块的颜色。
    void setIndicatorColor(const Vector4& color);

    /// 使用 RGBA 分量设置指示器方块的颜色。
    void setIndicatorColor(float r, float g, float b, float a);

    /// 设置选中时指示器填充的颜色。
    void setIndicatorCheckedColor(const Vector4& color);

    /// 使用 RGBA 分量设置选中时指示器填充的颜色。
    void setIndicatorCheckedColor(float r, float g, float b, float a);

protected:
    MRCheckBox();

    void updateVisualState() override;

private:
    void initializeIndicator();

    void layoutIndicator();

    MRColorSharedPtr m_indicator;
    MRColorSharedPtr m_indicatorFill;
    Vector4 m_indicatorColor = Vector4(0.68f, 0.72f, 0.78f, 1.0f);
    Vector4 m_indicatorCheckedColor = Vector4(0.08f, 0.62f, 0.42f, 1.0f);
};

using MRCheckBoxSharedPtr = std::shared_ptr<MRCheckBox>;

}  // namespace morrow

#endif  // MORROW_GUI_MRCHECKBOX_H
