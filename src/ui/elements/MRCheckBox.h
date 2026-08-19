#ifndef MORROW_GUI_MRCHECKBOX_H
#define MORROW_GUI_MRCHECKBOX_H

#include "MRColor.h"
#include "MRSelectableButton.h"

namespace morrow {

class MRCheckBox : public MRSelectableButton {
public:
    static std::shared_ptr<MRCheckBox> create();

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
};

using MRCheckBoxSharedPtr = std::shared_ptr<MRCheckBox>;

}  // namespace morrow

#endif  // MORROW_GUI_MRCHECKBOX_H
