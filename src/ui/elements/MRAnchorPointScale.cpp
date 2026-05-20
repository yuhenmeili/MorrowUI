//
// Created by lance on 2022/11/2.
//

#include "MRAnchorPointScale.h"
#include "base/Transform.h"

namespace morrow {
MRAnchorPointScaleSharedPtr MRAnchorPointScale::create() {
    return std::shared_ptr<MRAnchorPointScale>(new MRAnchorPointScale());
}

MRAnchorPointScale::MRAnchorPointScale() {
    m_widgetType = "MRAnchorPointScale";
    m_material->setShader("anchor_point_scale");
}
}
