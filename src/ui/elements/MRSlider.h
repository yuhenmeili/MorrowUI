//
// Created by lance on 2026/8/17.
//

#ifndef MORROW_GUI_MRSLIDER_H
#define MORROW_GUI_MRSLIDER_H

#include <functional>
#include <memory>

#include "MRColor.h"
#include "MRProgressBar.h"

namespace morrow {

class MRSlider;
using MRSliderSharedPtr = std::shared_ptr<MRSlider>;

/// 滑杆组件：继承进度条渲染，额外提供可拖拽滑块与数值回调。
class MRSlider : public MRProgressBar {
public:
    static MRSliderSharedPtr create();

    virtual ~MRSlider() = default;

    void setValue(float value);

    float getValue() const;

    void setThumbSize(const Vector2& size);

    void setThumbColor(const Vector4& color);

    void setThumbColor(float r, float g, float b, float a);

    void setThumbRounding(float rounding);

    void setInteractive(bool interactive);

    void setOnValueChangedCallback(std::function<void(float)> callback);

protected:
    MRSlider();

    void onProgressChanged(float progress) override;

private:
    void updateValueFromPosition(float screenX, float screenY);

    void repositionThumb();

    MRColorSharedPtr m_thumb;
    std::function<void(float)> m_onValueChanged;
    bool m_dragging = false;
    bool m_interactive = true;
    Vector2 m_thumbSize = Vector2(20.0f, 20.0f);
};

}  // namespace morrow
#endif  // MORROW_GUI_MRSLIDER_H
