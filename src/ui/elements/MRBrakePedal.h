//
// Created by lance on 2022/10/12.
//

#ifndef MORROW_BRAKE_PEDAL_H
#define MORROW_BRAKE_PEDAL_H

#include <memory>
#include "base/UIWidget.h"

namespace morrow {
/// 使用制动踏板着色器绘制的 UI 特效组件。
class MRBrakePedal : public UIWidget {
public:
    /// 创建一个制动踏板特效组件。
    static std::shared_ptr<MRBrakePedal> create();

    /// 销毁制动踏板特效组件。
    virtual ~MRBrakePedal() = default;

private:
    MRBrakePedal();
};

using MRBrakePedalSharedPtr = std::shared_ptr<MRBrakePedal>;
}


#endif //MORROW_BRAKE_PEDAL_H
