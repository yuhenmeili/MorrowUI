#ifndef MORROW_GUI_MRTOGGLE_H
#define MORROW_GUI_MRTOGGLE_H

#include "MRColor.h"
#include "MRSelectableButton.h"

namespace morrow {

class MRToggle : public MRSelectableButton {
public:
    static std::shared_ptr<MRToggle> create();

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
