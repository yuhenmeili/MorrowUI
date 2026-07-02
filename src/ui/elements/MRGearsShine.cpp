//
// Created by lance on 2023/3/23.
//

#include "MRGearsShine.h"
#include "utils/Log.h"
#include "base/Transform.h"
#include "ui/helpers/Tween.h"

namespace morrow
{
MRGearsShineSharedPtr MRGearsShine::create()
{
    return std::shared_ptr<MRGearsShine>(new MRGearsShine());
}

MRGearsShine::MRGearsShine()
{
    m_widgetType = "MRGearsShine";
    m_options = std::make_shared<MRGearsShineOptions>();
    m_material->setShader("gears_shine");
}

void MRGearsShine::initialize()
{
    //init mesh
    int32_t vertexCount = m_options->pointRow * m_options->pointColumn;
    vertexCount -= m_options->hollowRow.size() * m_options->hollowColumn.size();

    auto mesh = getComponent<MeshFilter>()->getMesh();
    mesh->clear();
    mesh->setDrawMode(PrimitiveType::POINTS);
    std::vector<Vector3> gearsVertices(vertexCount);
    std::vector<Vector4> gearsColors(vertexCount);
    std::vector<Vector2> gearsUVs(vertexCount);

    float startX = -(float(m_options->pointColumn - 1) / 2.0f) * m_options->pointColumnGap;
    float startY = -(float(m_options->pointRow - 1) / 2.0f) * m_options->pointRowGap;
    auto zValue = float(m_displayLayer);

    int32_t index = 0;
    for (int32_t i = 0; i < m_options->pointRow; ++i) {
        for (int32_t j = 0; j < m_options->pointColumn; ++j) {
            if (m_options->isCurrentRowAndColumnHollow(i, j)) {
                //do nothing
            } else {
                float currentX = startX + j * m_options->pointColumnGap;
                float currentY = startY + i * m_options->pointRowGap;
                float alpha = 1.0f - std::abs(currentY / startY) * (1.0f - m_options->pointColorAlphaOffset);

                gearsVertices[index].set(currentX, currentY, zValue);
                gearsColors[index].set(1.0f, 1.0f, 1.0f, alpha);
                gearsUVs[index].set(0.0f, 0.0f);
                ++index;
            }
        }
    }
    mesh->setVertices(gearsVertices);
    mesh->setColors(gearsColors);
    mesh->setUVs(gearsUVs);

    //init material
    auto transform = getComponent<Transform>();
    auto size = transform->getSize();
    m_material->setVector("imageSize", Vector2(size.x, size.y));
    m_material->setFloat("duration", 1.0f);
    m_material->setInt("direct", m_options->direct);
    m_material->setFloat("radiusHighlight", m_options->radiusHighlight);
    m_material->setFloat("pointSize", m_options->pointSize);
    m_material->setFloat("pointSizeHighlight", m_options->pointSizeHighlight);
    m_material->setVector("pointColorHighlight", m_options->pointColorHighlight);

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
}
}
