#ifndef MORROW_GUI_MRRADIOBUTTON_H
#define MORROW_GUI_MRRADIOBUTTON_H

#include <memory>
#include <vector>

#include "MRColor.h"
#include "MRSelectableButton.h"
#include "base/UIWidget.h"

namespace morrow {

class MRRadioButton;

class MRRadioGroup : public UIWidget {
public:
    static std::shared_ptr<MRRadioGroup> create();

    MRRadioGroup();

    /// 将单选按钮加入当前互斥组。
    void add(const std::shared_ptr<MRRadioButton>& button);

    /// 将单选按钮移出当前互斥组。
    void remove(const std::shared_ptr<MRRadioButton>& button);

    /// 选中指定按钮，并取消同组其他按钮的选中状态。
    void select(const std::shared_ptr<MRRadioButton>& button);

    /// 获取当前选中的单选按钮；没有选中项时返回空指针。
    std::shared_ptr<MRRadioButton> getSelected() const;

    void addChild(std::shared_ptr<Widget> widget) override;

    bool removeChild(std::shared_ptr<Widget> widget) override;

private:
    std::vector<std::weak_ptr<MRRadioButton>> m_buttons;
};

using MRRadioGroupSharedPtr = std::shared_ptr<MRRadioGroup>;

class MRRadioButton : public MRSelectableButton {
public:
    /// 创建一个单选按钮。
    static std::shared_ptr<MRRadioButton> create();

    /// 设置单选按钮文字，并在文字变化后重新布局指示器。
    void setText(const std::wstring& text, const std::string& fontName) {
        MRButton::setText(text, fontName);
        layoutIndicator();
    }

    /// 设置按钮所属的互斥单选组。
    void setGroup(const MRRadioGroupSharedPtr& group);

    /// 获取按钮当前所属的互斥单选组。
    MRRadioGroupSharedPtr getGroup() const {
        return m_group.lock();
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

    std::weak_ptr<MRRadioGroup> m_group;
    MRColorSharedPtr m_indicator;
    MRColorSharedPtr m_indicatorFill;
};

using MRRadioButtonSharedPtr = std::shared_ptr<MRRadioButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MRRADIOBUTTON_H
