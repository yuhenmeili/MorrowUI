#ifndef MORROW_GUI_MRPARALLAX2D_H
#define MORROW_GUI_MRPARALLAX2D_H

#include <memory>

#include "base/UIWidget.h"

namespace morrow {

/// 视差层：根据背景滚动偏移，以指定倍率移动自身及其子节点。
class MRParallax2D : public UIWidget {
public:
    /// 创建一个视差层。
    static std::shared_ptr<MRParallax2D> create();

    /// 设置该层相对于滚动偏移的移动倍率。
    void setScrollScale(float x, float y);
    /// 获取该层的移动倍率。
    Vector2 getScrollScale() const;

    /// 设置该层的初始位置。视差更新会以此位置为基准。
    void setBasePosition(float x, float y, float z = 0.0f);
    /// 设置背景滚动偏移。
    void setScrollOffset(const Vector2& offset);
    /// 获取背景滚动偏移。
    Vector2 getScrollOffset() const;

private:
    MRParallax2D();

    void applyPosition();

    Vector2 m_scrollScale{1.0f, 1.0f};
    Vector2 m_scrollOffset{0.0f, 0.0f};
    Vector3 m_basePosition{0.0f, 0.0f, 0.0f};
};

using MRParallax2DSharedPtr = std::shared_ptr<MRParallax2D>;
using Parallax2D = MRParallax2D;

}  // namespace morrow

#endif  // MORROW_GUI_MRPARALLAX2D_H
