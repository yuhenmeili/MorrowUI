//
// Created by lance on 2026/8/17.
//

#include "MRProgressBar.h"

#include <algorithm>

#include "base/Transform.h"
#include "renderer/resource/ssbo/layouts/ProgressBarSSBOLayout.h"

namespace morrow {

MRProgressBarSharedPtr MRProgressBar::create() {
    return std::shared_ptr<MRProgressBar>(new MRProgressBar());
}

MRProgressBar::MRProgressBar() {
    setWidgetType("MRProgressBar");
    m_material->setShader("progress_bar");
    m_material->setSSBOLayout(std::make_shared<ProgressBarSSBOLayout>());
    m_material->setFloat("rounding", 0.0f);
    m_material->setFloat("progress", 0.0f);
    m_material->setFloat("direction", static_cast<float>(m_direction));
    m_material->setFloat("useTrackTexture", 0.0f);
    m_material->setFloat("useFillTexture", 0.0f);
    m_material->setVector("trackColor", Vector4(0.85f, 0.85f, 0.85f, 1.0f));
    m_material->setVector("fillColor", Vector4(0.2f, 0.5f, 0.9f, 1.0f));

    auto transform = getComponent<Transform>();
    transform->addSizeChangeListener([this]() {
        auto transform = getComponent<Transform>();
        m_material->setVector("displaySize", transform->getSize());
        requestRender("transformSizeChanged");
    });
}

void MRProgressBar::setProgress(float progress) {
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    if (m_progress != clamped) {
        m_progress = clamped;
        m_material->setFloat("progress", clamped);
        onProgressChanged(clamped);
        requestRender("setProgress");
    }
}

float MRProgressBar::getProgress() const {
    return m_progress;
}

void MRProgressBar::setDirection(ProgressDirection direction) {
    if (m_direction != direction) {
        m_direction = direction;
        m_material->setFloat("direction", static_cast<float>(direction));
        requestRender("setDirection");
    }
}

ProgressDirection MRProgressBar::getDirection() const {
    return m_direction;
}

void MRProgressBar::setTrackColor(const Vector4& color) {
    m_material->setVector("trackColor", color);
    requestRender("setTrackColor");
}

void MRProgressBar::setTrackColor(float r, float g, float b, float a) {
    setTrackColor(Vector4(r, g, b, a));
}

void MRProgressBar::setFillColor(const Vector4& color) {
    m_material->setVector("fillColor", color);
    requestRender("setFillColor");
}

void MRProgressBar::setFillColor(float r, float g, float b, float a) {
    setFillColor(Vector4(r, g, b, a));
}

void MRProgressBar::setRounding(float rounding) {
    m_material->setFloat("rounding", rounding);
    requestRender("setRounding");
}

void MRProgressBar::setTrackTexture(TextureSharedPtr texture) {
    if (texture) {
        m_material->setTexture("trackTexture", texture);
        m_material->setFloat("useTrackTexture", 1.0f);
    } else {
        m_material->setFloat("useTrackTexture", 0.0f);
    }
    requestRender("setTrackTexture");
}

void MRProgressBar::setFillTexture(TextureSharedPtr texture) {
    if (texture) {
        m_material->setTexture("fillTexture", texture);
        m_material->setFloat("useFillTexture", 1.0f);
    } else {
        m_material->setFloat("useFillTexture", 0.0f);
    }
    requestRender("setFillTexture");
}

void MRProgressBar::onProgressChanged(float /*progress*/) {
}

}  // namespace morrow
