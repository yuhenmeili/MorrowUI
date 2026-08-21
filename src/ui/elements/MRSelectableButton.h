#ifndef MORROW_GUI_MRSELECTABLEBUTTON_H
#define MORROW_GUI_MRSELECTABLEBUTTON_H

#include <memory>

#include "MRButton.h"

namespace morrow {

class MRSelectableButton : public MRButton {
public:
    struct SelectionEvents {
        Observable<MRSelectableButton&, bool> onCheckedChanged;
    };

    /// 设置按钮是否处于选中状态。
    void setChecked(bool checked);

    /// 获取按钮当前是否处于选中状态。
    bool isChecked() const {
        return m_checked;
    }

    SelectionEvents& selectionEvents();

    /// 设置选中状态下的默认颜色。
    void setCheckedColor(const Vector4& color);

    /// 使用 RGBA 分量设置选中状态下的默认颜色。
    void setCheckedColor(float r, float g, float b, float a);

    /// 设置选中并悬停状态下的颜色。
    void setCheckedHoverColor(const Vector4& color);

    /// 设置选中并按下状态下的颜色。
    void setCheckedPressedColor(const Vector4& color);

protected:
    MRSelectableButton();

    void onActivated() override;

    void updateVisualState() override;

    virtual bool canUncheck() const {
        return true;
    }
    virtual void onCheckedChanged(bool checked);

    Vector4 m_checkedColor = Vector4(0.12f, 0.55f, 0.42f, 1.0f);
    Vector4 m_checkedHoverColor = Vector4(0.16f, 0.65f, 0.5f, 1.0f);
    Vector4 m_checkedPressedColor = Vector4(0.08f, 0.42f, 0.32f, 1.0f);
    bool m_checked = false;
    SelectionEvents m_selectionEvents;
};

using MRSelectableButtonSharedPtr = std::shared_ptr<MRSelectableButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSELECTABLEBUTTON_H
