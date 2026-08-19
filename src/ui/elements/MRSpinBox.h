#ifndef MORROW_GUI_MRSPINBOX_H
#define MORROW_GUI_MRSPINBOX_H

#include <functional>
#include <memory>
#include <string>

#include "MRButton.h"
#include "MRColor.h"
#include "MRLabel.h"

namespace morrow {

class MRSpinBox : public UIWidget {
public:
    using ValueChangedCallback = std::function<void(double)>;

    static std::shared_ptr<MRSpinBox> create();

    void setRange(double minimum, double maximum);

    double getMinimum() const {
        return m_minimum;
    }
    double getMaximum() const {
        return m_maximum;
    }

    void setStep(double step);

    double getStep() const {
        return m_step;
    }

    void setValue(double value);

    double getValue() const {
        return m_value;
    }

    void setDecimals(int decimals);

    void setPrefix(const std::wstring& prefix);

    void setSuffix(const std::wstring& suffix);

    void setEnabled(bool enabled);

    void setOnValueChangedCallback(ValueChangedCallback callback);

private:
    MRSpinBox();

    void initializeChildren();

    void layoutChildren();

    void refreshText();

    void stepBy(double amount);

    MRColorSharedPtr m_background;
    std::shared_ptr<MRButton> m_decreaseButton;
    std::shared_ptr<MRButton> m_increaseButton;
    std::shared_ptr<MRLabel> m_valueLabel;
    ValueChangedCallback m_onValueChanged;
    double m_minimum = 0.0;
    double m_maximum = 100.0;
    double m_step = 1.0;
    double m_value = 0.0;
    int m_decimals = 0;
    std::wstring m_prefix;
    std::wstring m_suffix;
};

using MRSpinBoxSharedPtr = std::shared_ptr<MRSpinBox>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSPINBOX_H
