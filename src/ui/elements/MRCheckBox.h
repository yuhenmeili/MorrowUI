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

protected:
    MRCheckBox();

    void updateVisualState() override;

private:
    void initializeIndicator();

    void layoutIndicator();

    MRColorSharedPtr m_indicator;
    MRColorSharedPtr m_indicatorFill;
};

using MRCheckBoxSharedPtr = std::shared_ptr<MRCheckBox>;

}  // namespace morrow

#endif  // MORROW_GUI_MRCHECKBOX_H
