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
    LeftToRight = 0,
    RightToLeft = 1,
    BottomToTop = 2,
    TopToBottom = 3,
};

class MRProgressBar;
using MRProgressBarSharedPtr = std::shared_ptr<MRProgressBar>;

/// 进度条组件：单个 quad 同时渲染轨道与填充，支持方向与可替换颜色/纹理。
class MRProgressBar : public UIWidget {
public:
    static MRProgressBarSharedPtr create();

    virtual ~MRProgressBar() = default;

    void setProgress(float progress);

    float getProgress() const;

    void setDirection(ProgressDirection direction);

    ProgressDirection getDirection() const;

    void setTrackColor(const Vector4& color);

    void setTrackColor(float r, float g, float b, float a);

    void setFillColor(const Vector4& color);

    void setFillColor(float r, float g, float b, float a);

    void setRounding(float rounding);

    void setTrackTexture(TextureSharedPtr texture);

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
