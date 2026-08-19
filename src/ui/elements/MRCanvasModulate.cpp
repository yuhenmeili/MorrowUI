#include "MRCanvasModulate.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRCanvasModulate> MRCanvasModulate::create() {
    return std::shared_ptr<MRCanvasModulate>(new MRCanvasModulate());
}

MRCanvasModulate::MRCanvasModulate() {
    setWidgetType("MRCanvasModulate");
    setDisplayLayer(10);
    m_material->setShader("canvas_modulate");
    m_material->setBlendEnabled(true);
    m_material->setBlendFunc(BlendFactor::ZERO, BlendFactor::SRC_COLOR, BlendFactor::ZERO, BlendFactor::ONE);
    applyModulation();
}

void MRCanvasModulate::setModulateColor(const Vector4& color) {
    m_modulateColor = color;
    applyModulation();
}

void MRCanvasModulate::setStrength(float strength) {
    m_strength = std::clamp(strength, 0.0f, 1.0f);
    applyModulation();
}

void MRCanvasModulate::setAutoFitParent(bool enabled) {
    m_autoFitParent = enabled;
    requestRender("canvas modulate auto fit");
}

void MRCanvasModulate::setNightMode(bool enabled) {
    m_nightMode = enabled;
    setStrength(enabled ? 1.0f : 0.0f);
}

void MRCanvasModulate::update(FrameStateSharedPtr frameState) {
    if (m_autoFitParent && m_parent) {
        if (auto parentTransform = m_parent->getComponent<Transform>()) {
            const Vector3 parentSize = parentTransform->getSize();
            auto transform = getComponent<Transform>();
            const Vector3 size = transform->getSize();
            if (size.x != parentSize.x || size.y != parentSize.y) {
                transform->setPosition(0.0f, 0.0f, 0.0f);
                transform->setSize(parentSize.x, parentSize.y);
            }
        }
    }
    UIWidget::update(frameState);
}

void MRCanvasModulate::applyModulation() {
    const Vector4 effective(1.0f + (m_modulateColor.x - 1.0f) * m_strength, 1.0f + (m_modulateColor.y - 1.0f) * m_strength, 1.0f + (m_modulateColor.z - 1.0f) * m_strength, 1.0f);
    m_material->setVector("color", effective);
    requestRender("canvas modulation");
}

}  // namespace morrow
