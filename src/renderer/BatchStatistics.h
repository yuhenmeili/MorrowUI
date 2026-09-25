#ifndef MORROW_BATCHSTATISTICS_H
#define MORROW_BATCHSTATISTICS_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace morrow {

enum class BatchBreakReason : uint8_t {
    None = 0,
    DisplayLayer,
    MaterialState,
    Geometry,
    OrderBarrier,
    Count
};

struct BatchStatistics {
    uint32_t renderItemCount = 0;
    uint32_t batchCount = 0;
    uint32_t ssboBatchCount = 0;
    uint32_t standardBatchCount = 0;
    uint32_t batchDrawCallCount = 0;
    uint32_t cacheHitCount = 0;
    uint32_t cacheMissCount = 0;
    uint32_t ssboFallbackBatchCount = 0;
    // 背景模糊（KAWASE_BACKDROP_BLUR_PROPOSAL.md S1）：本帧提交的模糊面片数
    // 与模糊子系统（链 pass + 回屏合成 + 面片合并绘制）的 draw call 数；
    // 无模糊场景恒为 0。
    uint32_t backdropQuadCount = 0;
    uint32_t backdropDrawCallCount = 0;
    std::array<uint32_t, static_cast<size_t>(BatchBreakReason::Count)> breakReasonCounts{};

    void reset() {
        *this = {};
    }

    void recordBreak(BatchBreakReason reason) {
        if (reason != BatchBreakReason::None) {
            ++breakReasonCounts[static_cast<size_t>(reason)];
        }
    }
};

} // namespace morrow

#endif // MORROW_BATCHSTATISTICS_H
