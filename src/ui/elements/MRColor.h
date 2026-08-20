//
// Created by lance on 2026/8/17.
//

#ifndef MORROW_GUI_MRCOLOR_H
#define MORROW_GUI_MRCOLOR_H

#include <memory>

#include "base/UIWidget.h"

namespace morrow {
class MRColor;
using MRColorSharedPtr = std::shared_ptr<MRColor>;

/// 纯颜色组件：渲染一个纯色填充矩形，支持 alpha 与圆角。
class MRColor : public UIWidget {
public:
    /// 创建一个纯颜色矩形组件。
    static MRColorSharedPtr create();

    /// 销毁纯颜色矩形组件。
    virtual ~MRColor() = default;

    /// 设置填充颜色。
    void setColor(const Vector4& color);

    /// 使用 RGBA 分量设置填充颜色。
    void setColor(float r, float g, float b, float a);

    /// 设置矩形圆角大小。
    void setRounding(float rounding);

protected:
    MRColor();

private:
    float m_rounding = 0.0f;
};
}  // namespace morrow
#endif  // MORROW_GUI_MRCOLOR_H
