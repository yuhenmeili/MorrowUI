//
// Created by lance on 2022/10/12.
//

#include "MRGearsOpening.h"
#include "base/Transform.h"
#include "../helpers/Tween.h"

namespace morrow
{
MRGearsOpeningSharedPtr MRGearsOpening::create()
{
    return std::shared_ptr<MRGearsOpening>(new MRGearsOpening());
}
void MRGearsOpening::initialize() {
    auto transform = getComponent<Transform>();
    auto size = transform->getSize();

    m_material->setVector("imageSize", Vector2(size.x, size.y));
    m_material->setFloat("duration", 1.0f);
    m_material->setBool("forward", m_options->forward);
    m_material->setFloat("blurRadius", m_options->blurRadius);

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

MRGearsOpening::MRGearsOpening()
{
    m_widgetType = "MRGearsOpening";
    m_material->setShader("gears_opening");
    m_options = std::make_shared<MRGearsOpeningOptions>();
}
}
