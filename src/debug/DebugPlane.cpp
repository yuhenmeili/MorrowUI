//
// Created by 0060328 on 25-10-27.
//

#include "DebugPlane.h"

#include "Window.h"
#include "base/Transform.h"
#include "elements/MRLabel.h"

namespace morrow {
void DebugPlane::initialize(std::shared_ptr<Window> window) {
    // m_frameLabel = std::make_shared<MRLabel>();
    // auto fpsTextTransform = m_frameLabel->getComponent<Transform>();
    // fpsTextTransform->setPosition(Vector3(0.0f, 0.0f, 0.0f));
    // fpsTextTransform->setSize(Vector3(100.0f, 50.0f, 0.0f));
    //
    // m_frameLabel->setText( L"0");
    // m_frameLabel->setFontColor(1.0f, 0.0f, 0.0f, 1.0f);
    //
    // window->addChild(m_frameLabel);
    //
    //
    // m_batchLabel = std::make_shared<MRLabel>();
    // auto batchTextTransform = m_batchLabel->getComponent<Transform>();
    // batchTextTransform->setPosition(Vector3(200.0f, 0.0f, 0.0f));
    // batchTextTransform->setSize(Vector3(900.0f, 50.0f, 0.0f));
    //
    // m_batchLabel->setText(L"Batch statistics unavailable");
    // m_batchLabel->setFontColor(0.0f, 0.0f, 1.0f, 1.0f);
    //
    // window->addChild(m_batchLabel);
}

void DebugPlane::update(std::shared_ptr<FrameState> frame_state) {
    // m_frameLabel->setText(L"FPS: " + std::to_wstring(frame_state->fps));
    // const auto& stats = frame_state->batchStatistics;
    // const auto breakCount = [&stats](BatchBreakReason reason) {
    //     return stats.breakReasonCounts[static_cast<size_t>(reason)];
    // };
    // m_batchLabel->setText(
    //     L"Items:" + std::to_wstring(stats.renderItemCount) +
    //     L" Batches:" + std::to_wstring(stats.batchCount) +
    //     L" Draws:" + std::to_wstring(frame_state->drawCallCount) +
    //     L" BatchDraws:" + std::to_wstring(stats.batchDrawCallCount) +
    //     L" SSBO:" + std::to_wstring(stats.ssboBatchCount) +
    //     L" Std:" + std::to_wstring(stats.standardBatchCount) +
    //     L" Cache:" + std::to_wstring(stats.cacheHitCount) +
    //     L"/" + std::to_wstring(stats.cacheMissCount) +
    //     L" Fallback:" + std::to_wstring(stats.ssboFallbackBatchCount) +
    //     L" Break[L/M/G/O]:" +
    //     std::to_wstring(breakCount(BatchBreakReason::DisplayLayer)) + L"/" +
    //     std::to_wstring(breakCount(BatchBreakReason::MaterialState)) + L"/" +
    //     std::to_wstring(breakCount(BatchBreakReason::Geometry)) + L"/" +
    //     std::to_wstring(breakCount(BatchBreakReason::OrderBarrier)));
}
} // morrow
