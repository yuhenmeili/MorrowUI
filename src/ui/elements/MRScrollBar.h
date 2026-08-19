#ifndef MORROW_GUI_MRSCROLLBAR_H
#define MORROW_GUI_MRSCROLLBAR_H

#include <functional>
#include <memory>

#include "MRColor.h"

namespace morrow {

class MRScrollBar : public UIWidget {
public:
    using ValueChangedCallback = std::function<void(float)>;

    static std::shared_ptr<MRScrollBar> create();

    void setValue(float value);
    float getValue() const {
        return m_value;
    }

    void setPageRatio(float ratio);
    float getPageRatio() const {
        return m_pageRatio;
    }

    void setTrackColor(const Vector4& color);
    void setThumbColor(const Vector4& color);
    void setOnValueChangedCallback(ValueChangedCallback callback);

private:
    MRScrollBar();

    void updateThumb();
    void updateValueFromThumbCenter(float screenY);

    MRColorSharedPtr m_track;
    MRColorSharedPtr m_thumb;
    ValueChangedCallback m_onValueChanged;
    float m_value = 0.0f;
    float m_pageRatio = 0.25f;
    bool m_dragging = false;
    float m_dragCenterOffset = 0.0f;
};

using MRScrollBarSharedPtr = std::shared_ptr<MRScrollBar>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSCROLLBAR_H
