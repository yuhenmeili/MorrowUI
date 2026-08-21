//
// Created by lance on 2026/8/17.
//

#ifndef MORROW_GUI_MRSLIDER_H
#define MORROW_GUI_MRSLIDER_H

#include <memory>

#include "base/EventDispatcher.h"
#include "MRColor.h"
#include "MRProgressBar.h"

namespace morrow {

class MRSlider;
using MRSliderSharedPtr = std::shared_ptr<MRSlider>;

/// 滑杆组件：继承进度条渲染，额外提供可拖拽滑块与数值回调。
class MRSlider : public MRProgressBar {
public:
    struct Events {
        Observable<MRSlider&, float> onValueChanged;
    };

    /// 创建一个可拖拽的滑杆组件。
    static MRSliderSharedPtr create();

    /// 销毁滑杆组件。
    virtual ~MRSlider() = default;

    /// 设置滑杆值，取值会限制在 0 到 1 之间。
    void setValue(float value);

    /// 获取当前滑杆值。
    float getValue() const;

    /// 设置滑块尺寸。
    void setThumbSize(const Vector2& size);

    /// 设置滑块颜色。
    void setThumbColor(const Vector4& color);

    /// 使用 RGBA 分量设置滑块颜色。
    void setThumbColor(float r, float g, float b, float a);

    /// 设置滑块圆角大小。
    void setThumbRounding(float rounding);

    /// 设置是否响应点击和拖拽操作。
    void setInteractive(bool interactive);

    Events& events();

protected:
    MRSlider();

    void onProgressChanged(float progress) override;

private:
    void updateValueFromPosition(float screenX, float screenY);

    void repositionThumb();

    MRColorSharedPtr m_thumb;
    Events m_events;
    EventConnection m_touchConnection;
    EventConnection m_moveConnection;
    EventConnection m_releaseConnection;
    bool m_dragging = false;
    bool m_interactive = true;
    Vector2 m_thumbSize = Vector2(20.0f, 20.0f);
};

}  // namespace morrow
#endif  // MORROW_GUI_MRSLIDER_H
