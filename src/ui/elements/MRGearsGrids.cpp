//
// Created by lance on 2023/4/21.
//

#include "MRGearsGrids.h"

namespace morrow
{
MRGearsGridsSharedPtr MRGearsGrids::create()
{
    return std::shared_ptr<MRGearsGrids>(new MRGearsGrids());
}

void MRGearsGrids::initialize()
{
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

    int index = 0;
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

    m_material->setFloat("pointSize", 5.0f);
}

MRGearsGrids::MRGearsGrids()
{
    setWidgetType("MRGearsGrids");
    m_material->setShader("gears_grids");
    m_options = std::make_shared<MRGearsGridsOptions>();
}
}
