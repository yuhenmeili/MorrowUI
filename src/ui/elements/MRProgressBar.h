//
// Created by lance on 2026/8/17.
//

#ifndef MORROW_GUI_MRPROGRESSBAR_H
#define MORROW_GUI_MRPROGRESSBAR_H

#include <memory>

#include "Texture.h"
#include "base/UIWidget.h"

namespace morrow {

enum class ProgressDirection : int32_t {
    /// 从左向右增加填充区域。
    LeftToRight = 0,
    /// 从右向左增加填充区域。
    RightToLeft = 1,
    /// 从下向上增加填充区域。
    BottomToTop = 2,
    /// 从上向下增加填充区域。
    TopToBottom = 3,
};

class MRProgressBar;
using MRProgressBarSharedPtr = std::shared_ptr<MRProgressBar>;

/// 进度条组件：单个 quad 同时渲染轨道与填充，支持方向与可替换颜色/纹理。
class MRProgressBar : public UIWidget {
public:
    /// 创建一个线性进度条组件。
    static MRProgressBarSharedPtr create();

    /// 销毁进度条组件。
    virtual ~MRProgressBar() = default;

    /// 设置进度值，取值会限制在 0 到 1 之间。
    void setProgress(float progress);

    /// 获取当前进度值。
    float getProgress() const;

    /// 设置进度条的填充方向。
    void setDirection(ProgressDirection direction);

    /// 获取当前填充方向。
    ProgressDirection getDirection() const;

    /// 设置轨道颜色。
    void setTrackColor(const Vector4& color);

    /// 使用 RGBA 分量设置轨道颜色。
    void setTrackColor(float r, float g, float b, float a);

    /// 设置填充区域颜色。
    void setFillColor(const Vector4& color);

    /// 使用 RGBA 分量设置填充区域颜色。
    void setFillColor(float r, float g, float b, float a);

    /// 设置进度条圆角大小。
    void setRounding(float rounding);

    /// 设置轨道纹理。
    void setTrackTexture(TextureSharedPtr texture);

    /// 设置填充区域纹理。
    void setFillTexture(TextureSharedPtr texture);

protected:
    MRProgressBar();

    /// 进度变化钩子（供 MRSlider 重写以联动滑块位置与数值回调）
    virtual void onProgressChanged(float progress);

    float m_progress = 0.0f;
    ProgressDirection m_direction = ProgressDirection::LeftToRight;
};

}  // namespace morrow
#endif  // MORROW_GUI_MRPROGRESSBAR_H
