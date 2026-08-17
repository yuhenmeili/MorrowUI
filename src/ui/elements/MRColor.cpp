//
// Created by lance on 2026/8/17.
//

#include "MRColor.h"

#include "base/Transform.h"
#include "renderer/resource/ssbo/layouts/DefaultColorSSBOLayout.h"

namespace morrow {
MRColorSharedPtr MRColor::create() {
    return std::shared_ptr<MRColor>(new MRColor());
}

MRColor::MRColor() {
    setWidgetType("MRColor");
    m_material->setShader("default_color");
    m_material->setSSBOLayout(std::make_shared<DefaultColorSSBOLayout>());
    m_material->setFloat("rounding", 0.0f);
    m_material->setVector("color", Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this]() {
        auto transform = getComponent<Transform>();
        m_material->setVector("displaySize", transform->getSize());
        requestRender("transformSizeChanged");
    });
}

void MRColor::setColor(const Vector4& color) {
    m_material->setVector("color", color);
    requestRender("setColor");
}

void MRColor::setColor(float r, float g, float b, float a) {
    setColor(Vector4(r, g, b, a));
}

void MRColor::setRounding(float rounding) {
    if (m_rounding != rounding) {
        m_rounding = rounding;
        m_material->setFloat("rounding", rounding);
        requestRender("setRounding");
    }
}
}  // namespace morrow
