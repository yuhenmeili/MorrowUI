//
// Created by lance on 2022/10/11.
//

#include "MRBounce.h"
#include "utils/Log.h"
#include "ui/helpers/Tween.h"
#include "base/Transform.h"
#include "renderer/resource/ssbo/layouts/BounceSSBOLayout.h"

namespace morrow
{
MRBounceSharedPtr MRBounce::create()
{
    return std::shared_ptr<MRBounce>(new MRBounce());
}

MRBounce::MRBounce()
{
    setWidgetType("MRBounce");
    m_material->setShader("bounce");
    m_material->setSSBOLayout(std::make_shared<BounceSSBOLayout>());
}
}
