#ifndef MORROW_GUI_MRSELECTABLEBUTTON_H
#define MORROW_GUI_MRSELECTABLEBUTTON_H

#include <functional>
#include <memory>

#include "MRButton.h"

namespace morrow {

class MRSelectableButton : public MRButton {
public:
    using CheckedCallback = std::function<void(bool)>;

    void setChecked(bool checked);

    bool isChecked() const {
        return m_checked;
    }

    void setOnCheckedChangedCallback(CheckedCallback callback);

    void setCheckedColor(const Vector4& color);

    void setCheckedColor(float r, float g, float b, float a);

    void setCheckedHoverColor(const Vector4& color);

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
    CheckedCallback m_onCheckedChanged;
};

using MRSelectableButtonSharedPtr = std::shared_ptr<MRSelectableButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSELECTABLEBUTTON_H
