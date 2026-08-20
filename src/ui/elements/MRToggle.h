#ifndef MORROW_GUI_MRTOGGLE_H
#define MORROW_GUI_MRTOGGLE_H

#include "MRColor.h"
#include "MRSelectableButton.h"

namespace morrow {

class MRToggle : public MRSelectableButton {
public:
    /// 创建一个开关样式的切换按钮。
    static std::shared_ptr<MRToggle> create();

    /// 设置开关文字，并在文字变化后重新布局轨道和滑块。
    void setText(const std::wstring& text, const std::string& fontName) {
        MRButton::setText(text, fontName);
        layoutSwitch();
    }

protected:
    MRToggle();

    void updateVisualState() override;

private:
    void initializeSwitch();

    void layoutSwitch();

    MRColorSharedPtr m_track;
    MRColorSharedPtr m_thumb;
};

using MRToggleSharedPtr = std::shared_ptr<MRToggle>;

}  // namespace morrow

#endif  // MORROW_GUI_MRTOGGLE_H
