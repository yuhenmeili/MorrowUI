//
// Created by lance on 2022/11/2.
//

#include "MRAnchorPointScale.h"
#include "base/Transform.h"
#include "renderer/resource/ssbo/layouts/DefaultImageSSBOLayout.h"

namespace morrow {
MRAnchorPointScaleSharedPtr MRAnchorPointScale::create() {
    return std::shared_ptr<MRAnchorPointScale>(new MRAnchorPointScale());
}

MRAnchorPointScale::MRAnchorPointScale() {
    setWidgetType("MRAnchorPointScale");
    m_material->setShader("anchor_point_scale");
    m_material->setSSBOLayout(std::make_shared<DefaultImageSSBOLayout>());
}
}
