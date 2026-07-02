//
// Created by lance on 2022/10/11.
//

#include "MRBounce.h"
#include "utils/Log.h"
#include "ui/helpers/Tween.h"
#include "base/Transform.h"

namespace morrow
{
MRBounceSharedPtr MRBounce::create()
{
    return std::shared_ptr<MRBounce>(new MRBounce());
}

MRBounce::MRBounce()
{
    m_widgetType = "MRBounce";
    m_material->setShader("bounce");
}
}
