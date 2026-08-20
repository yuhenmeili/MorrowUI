//
// Created by lance on 2022/10/11.
//

#ifndef MORROW_BOUNCE_H
#define MORROW_BOUNCE_H

#include <memory>
#include "base/Widget.h"
#include "Texture.h"
#include "ui/helpers/Tween.h"
#include "base/UIWidget.h"

namespace morrow {
/// 使用弹跳着色器绘制的 UI 特效组件。
class MRBounce : public UIWidget {
public:
    /// 创建一个弹跳特效组件。
    static std::shared_ptr<MRBounce> create();

    /// 销毁弹跳特效组件。
    ~MRBounce() override = default;

private:
    MRBounce();
};

using MRBounceSharedPtr = std::shared_ptr<MRBounce>;
}


#endif //MORROW_BOUNCE_H
