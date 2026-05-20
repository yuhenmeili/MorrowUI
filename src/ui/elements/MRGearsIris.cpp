//
// Created by lance on 2022/10/12.
//

#include "MRGearsIris.h"

#include "../helpers/Tween.h"
#include "base/Transform.h"

namespace morrow
{
MRGearsIrisSharedPtr MRGearsIris::create()
{
    return std::shared_ptr<MRGearsIris>(new MRGearsIris());
}

void MRGearsIris::initialize() {
    auto transform = getComponent<Transform>();
    auto size = transform->getSize();

    m_material->setVector("imageSize", Vector2(size.x, size.y));
    m_material->setFloat("duration", 1.0f);
    m_material->setFloat("circleRadius", m_options->circleRadius);
    m_material->setFloat("circleDelay", m_options->circleDelay);
    m_material->setVector("circleColor", m_options->circleColor);
    m_material->setFloat("gridGap", m_options->gridGap);
    m_material->setFloat("gridResetTime", m_options->gridResetTime);
    m_material->setVector("gridHalfWidthParameter", m_options->gridHalfWidthParameter);
    m_material->setVector("gridColor", m_options->gridColor);

    auto timeTween = Tween::create(0.0f, 1.0f, 1.0f);
    timeTween->setEase(EaseType::Linear)
                .onUpdate([this](float value) {
                    m_material->setFloat("timeDelta", value);
                })
                .onComplete([timeTween]() {
                    timeTween->restart();
                });

    timeTween->play();
    TweenManager::getInstance().addTween(timeTween);

    UIWidget::initialize();
}

MRGearsIris::MRGearsIris()
{
    m_widgetType = "MRGearsIris";
    m_material->setShader("gears_iris");
    m_options = std::make_shared<MRGearsIrisOptions>();
}
}
