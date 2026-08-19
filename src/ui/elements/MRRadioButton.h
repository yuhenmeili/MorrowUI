#ifndef MORROW_GUI_MRRADIOBUTTON_H
#define MORROW_GUI_MRRADIOBUTTON_H

#include <memory>
#include <vector>

#include "MRColor.h"
#include "MRSelectableButton.h"

namespace morrow {

class MRRadioButton;

class MRRadioGroup {
public:
    void add(const std::shared_ptr<MRRadioButton>& button);

    void remove(const std::shared_ptr<MRRadioButton>& button);

    void select(const std::shared_ptr<MRRadioButton>& button);

    std::shared_ptr<MRRadioButton> getSelected() const;

private:
    std::vector<std::weak_ptr<MRRadioButton>> m_buttons;
};

using MRRadioGroupSharedPtr = std::shared_ptr<MRRadioGroup>;

class MRRadioButton : public MRSelectableButton {
public:
    static std::shared_ptr<MRRadioButton> create();

    void setText(const std::wstring& text, const std::string& fontName) {
        MRButton::setText(text, fontName);
        layoutIndicator();
    }

    void setGroup(const MRRadioGroupSharedPtr& group);

    MRRadioGroupSharedPtr getGroup() const {
        return m_group;
    }

protected:
    MRRadioButton();

    bool canUncheck() const override {
        return false;
    }
    void onCheckedChanged(bool checked) override;

    void updateVisualState() override;

private:
    void initializeIndicator();

    void layoutIndicator();

    MRRadioGroupSharedPtr m_group;
    MRColorSharedPtr m_indicator;
};

using MRRadioButtonSharedPtr = std::shared_ptr<MRRadioButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MRRADIOBUTTON_H
