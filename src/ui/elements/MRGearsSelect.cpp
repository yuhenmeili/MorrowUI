//
// Created by lance on 2022/11/2.
//

#include "MRGearsSelect.h"

#include "ui/helpers/Tween.h"

namespace morrow
{
MRGearsSelectSharedPtr MRGearsSelect::create()
{
    return std::shared_ptr<MRGearsSelect>(new MRGearsSelect());
}

MRGearsSelect::MRGearsSelect()
{
    setWidgetType("MRGearsSelect");
    m_material->setShader("gears_select");
    m_options = std::make_shared<MRGearsSelectOptions>();
}

void MRGearsSelect::initialize()
{
    auto mesh = getComponent<MeshFilter>()->getMesh();
    mesh->clear();
    mesh->setDrawMode(PrimitiveType::POINTS);
    std::vector<Vector3> gearsVertices(1);
    std::vector<Vector4> gearsColors(1);

    gearsVertices[0].set(0.0f, 0.0f, float(m_displayLayer));
    gearsColors[0].copy(m_options->color);
    mesh->setVertices(gearsVertices);
    mesh->setColors(gearsColors);

    m_material->setFloat("pointSize", m_options->pointSize);

    auto timeTween = Tween::create(0.0f, 1.0f, 2.0f);
    timeTween->setEase(EaseType::Linear)
                .onUpdate([this](float value) {
                    m_material->setFloat("alpha", value);
                })
                .onComplete([timeTween]() {
                    timeTween->restart();
                });

    timeTween->play();
    TweenManager::getInstance().addTween(timeTween);
}
}
