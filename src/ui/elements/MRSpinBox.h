#ifndef MORROW_GUI_MRSPINBOX_H
#define MORROW_GUI_MRSPINBOX_H

#include <memory>
#include <string>

#include "MRButton.h"
#include "MRColor.h"
#include "MRLabel.h"

namespace morrow {

class MRSpinBox : public UIWidget {
public:
    struct Events {
        Observable<MRSpinBox&, double> onValueChanged;
    };

    /// 创建一个数值步进输入控件。
    static std::shared_ptr<MRSpinBox> create();

    /// 设置允许输入的最小值和最大值。
    void setRange(double minimum, double maximum);

    /// 获取允许的最小值。
    double getMinimum() const {
        return m_minimum;
    }
    /// 获取允许的最大值。
    double getMaximum() const {
        return m_maximum;
    }

    /// 设置每次增减的步长。
    void setStep(double step);

    /// 获取当前步长。
    double getStep() const {
        return m_step;
    }

    /// 设置当前数值，取值会限制在设定范围内。
    void setValue(double value);

    /// 获取当前数值。
    double getValue() const {
        return m_value;
    }

    /// 设置显示的小数位数。
    void setDecimals(int decimals);

    /// 设置数值文字前缀。
    void setPrefix(const std::wstring& prefix);

    /// 设置数值文字后缀。
    void setSuffix(const std::wstring& suffix);

    /// 设置增减按钮是否可用。
    void setEnabled(bool enabled);

    Events& events();

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
    Events m_events;
    Observable<BaseButton&>::Connection m_decreaseConnection;
    Observable<BaseButton&>::Connection m_increaseConnection;
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
