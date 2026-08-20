#ifndef MORROW_GUI_MRCHECKBUTTON_H
#define MORROW_GUI_MRCHECKBUTTON_H

#include "MRSelectableButton.h"

namespace morrow {

class MRCheckButton : public MRSelectableButton {
public:
    /// 创建一个可切换选中状态的按钮。
    static std::shared_ptr<MRCheckButton> create();

protected:
    MRCheckButton();
};

using MRCheckButtonSharedPtr = std::shared_ptr<MRCheckButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MRCHECKBUTTON_H
