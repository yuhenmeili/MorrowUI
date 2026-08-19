#ifndef MORROW_GUI_MRPARALLAXBACKGROUND_H
#define MORROW_GUI_MRPARALLAXBACKGROUND_H

#include <memory>
#include <vector>

#include "MRParallax2D.h"

namespace morrow {

/// 视差背景容器：统一驱动多个 MRParallax2D 层。
class MRParallaxBackground : public UIWidget {
public:
    static std::shared_ptr<MRParallaxBackground> create();

    /// 添加一个视差层，并设置该层的移动倍率。
    void addLayer(const MRParallax2DSharedPtr& layer, float scaleX, float scaleY);
    /// 移除指定视差层。
    bool removeLayer(const MRParallax2DSharedPtr& layer);
    /// 设置所有视差层共用的背景滚动偏移。
    void setScrollOffset(float x, float y);
    /// 获取背景滚动偏移。
    Vector2 getScrollOffset() const;
    /// 返回当前管理的视差层。
    const std::vector<MRParallax2DSharedPtr>& getLayers() const;

private:
    MRParallaxBackground();

    Vector2 m_scrollOffset{0.0f, 0.0f};
    std::vector<MRParallax2DSharedPtr> m_layers;
};

using MRParallaxBackgroundSharedPtr = std::shared_ptr<MRParallaxBackground>;
using ParallaxBackground = MRParallaxBackground;

}  // namespace morrow

#endif  // MORROW_GUI_MRPARALLAXBACKGROUND_H
