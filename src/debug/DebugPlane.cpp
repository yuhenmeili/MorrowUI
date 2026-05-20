//
// Created by 0060328 on 25-10-27.
//

#include "DebugPlane.h"

#include "Window.h"
#include "base/Transform.h"
#include "elements/MRLabel.h"

namespace morrow {
void DebugPlane::initialize(std::shared_ptr<Window> window) {
    m_frameLabel = std::make_shared<MRLabel>();
    auto fpsTextTransform = m_frameLabel->getComponent<Transform>();
    fpsTextTransform->setPosition(Vector3(0.0f, 0.0f, 0.0f));
    fpsTextTransform->setSize(Vector3(100.0f, 50.0f, 0.0f));

    m_frameLabel->setText( L"0");
    m_frameLabel->setFontColor(1.0f, 0.0f, 0.0f, 1.0f);

    window->addChild(m_frameLabel);


    m_drawCallLabel = std::make_shared<MRLabel>();
    auto drawCallTextTransform = m_drawCallLabel->getComponent<Transform>();
    drawCallTextTransform->setPosition(Vector3(200.0f, 0.0f, 0.0f));
    drawCallTextTransform->setSize(Vector3(100.0f, 50.0f, 0.0f));

    m_drawCallLabel->setText( L"0");
    m_drawCallLabel->setFontColor(0.0f, 0.0f, 1.0f, 1.0f);

    window->addChild(m_drawCallLabel);
}

void DebugPlane::update(std::shared_ptr<FrameState> frame_state) {
    // LOG_I("FPS: {}, drawCallCount: {}", frame_state->fps, frame_state->drawCallCount);
    m_frameLabel->setText(L"FPS: " + std::to_wstring(frame_state->fps));
    m_drawCallLabel->setText(L"DrawCall: " + std::to_wstring(frame_state->drawCallCount));
}
} // morrow