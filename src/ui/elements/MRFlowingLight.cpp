//
// Created by lance on 2023/2/22.
//

#include "MRFlowingLight.h"
#include "base/Transform.h"
#include "ui/helpers/Tween.h"

namespace morrow
{

MRFlowingLightSharedPtr MRFlowingLight::create()
{
    return std::shared_ptr<MRFlowingLight>(new MRFlowingLight());
}
void MRFlowingLight::initialize() {
    auto transform = getComponent<Transform>();
    auto size = transform->getSize();
    m_material->setFloat("duration", 2.0f);
    m_material->setVector("displaySize", Vector2(size.x, size.y));
    m_material->setFloat("flowingLightLength", m_options->flowingLightLength);
    m_material->setVector("flowingLightColor", m_options->flowingLightColor);
    m_material->setFloat("flowingLightThickness", m_options->flowingLightThickness);


    auto timeTween = Tween::create(0.0f, 2.0f, 2.0f);
    timeTween->setLoop(-1);
    timeTween->setEase(EaseType::Linear)
             .onUpdate([this](float value) {
                 m_material->setFloat("timeDelta", value);
             });

    timeTween->play();
    TweenManager::getInstance().addTween(timeTween);

    UIWidget::initialize();
}

MRFlowingLight::MRFlowingLight()
{
    setWidgetType("MRFlowingLight");
    m_material->setShader("flowing_light");
    m_options = std::make_shared<MRFlowingLightOptions>();
}
}
