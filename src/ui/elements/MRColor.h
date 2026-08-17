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
    static MRColorSharedPtr create();

    virtual ~MRColor() = default;

    void setColor(const Vector4& color);

    void setColor(float r, float g, float b, float a);

    void setRounding(float rounding);

private:
    MRColor();

    float m_rounding = 0.0f;
};
}  // namespace morrow
#endif  // MORROW_GUI_MRCOLOR_H
