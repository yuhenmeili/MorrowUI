//
// Created by lance on 2022/11/2.
//

#ifndef MORROW_ANCHORPOINT_SCALE_H
#define MORROW_ANCHORPOINT_SCALE_H

#include <memory>
#include "../helpers/Tween.h"
#include "base/UIWidget.h"

namespace morrow {
class MRAnchorPointScale;

using MRAnchorPointScaleSharedPtr = std::shared_ptr<MRAnchorPointScale>;

class MRAnchorPointScale : public UIWidget {
public:
    static MRAnchorPointScaleSharedPtr create();

    ~MRAnchorPointScale() override = default;

private:
    MRAnchorPointScale();
};
}

#endif //MORROW_ANCHORPOINT_SCALE_H
