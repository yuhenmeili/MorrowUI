#ifndef MORROW_GUI_MRSCROLLBAR_H
#define MORROW_GUI_MRSCROLLBAR_H

#include <memory>

#include "base/EventDispatcher.h"
#include "MRColor.h"

namespace morrow {

class MRScrollBar : public UIWidget {
public:
    struct Events {
        Observable<MRScrollBar&, float> onValueChanged;
    };

    /// 创建一个垂直滚动条。
    static std::shared_ptr<MRScrollBar> create();

    /// 设置归一化滚动值，取值会限制在 0 到 1 之间。
    void setValue(float value);
    /// 获取当前归一化滚动值。
    float getValue() const {
        return m_value;
    }

    /// 设置可见页面占全部内容的比例。
    void setPageRatio(float ratio);
    /// 获取可见页面占全部内容的比例。
    float getPageRatio() const {
        return m_pageRatio;
    }

    /// 设置滚动条轨道颜色。
    void setTrackColor(const Vector4& color);
    /// 设置滚动条滑块颜色。
    void setThumbColor(const Vector4& color);

    Events& events();

private:
    MRScrollBar();

    void updateThumb();
    void updateValueFromThumbCenter(float screenY);

    MRColorSharedPtr m_track;
    MRColorSharedPtr m_thumb;
    Events m_events;
    EventConnection m_touchConnection;
    EventConnection m_moveConnection;
    EventConnection m_releaseConnection;
    float m_value = 0.0f;
    float m_pageRatio = 0.25f;
    bool m_dragging = false;
    float m_dragCenterOffset = 0.0f;
};

using MRScrollBarSharedPtr = std::shared_ptr<MRScrollBar>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSCROLLBAR_H
