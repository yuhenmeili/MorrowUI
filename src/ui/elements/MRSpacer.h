#ifndef MORROW_GUI_MRSPACER_H
#define MORROW_GUI_MRSPACER_H

#include <memory>

#include "base/UIWidget.h"

namespace morrow {

class MRSpacer : public UIWidget {
public:
    /// 创建一个按 flex 权重占用布局剩余空间的占位组件。
    static std::shared_ptr<MRSpacer> create(float flex = 1.0f);

    /// 设置分配布局剩余空间时使用的权重。
    void setFlex(float flex);

    /// 获取当前 flex 权重。
    float getFlex() const {
        return m_flex;
    }

    /// 设置占位组件的最小尺寸。
    void setMinimumSize(const Vector2& size);

    /// 获取占位组件的最小尺寸。
    Vector2 getMinimumSize() const {
        return m_minimumSize;
    }

private:
    explicit MRSpacer(float flex);

    float m_flex = 1.0f;
    Vector2 m_minimumSize = Vector2(0.0f, 0.0f);
};

using MRSpacerSharedPtr = std::shared_ptr<MRSpacer>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSPACER_H
