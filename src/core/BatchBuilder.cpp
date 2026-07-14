#include "BatchBuilder.h"

namespace morrow {

bool BatchBuilder::canBatch(const RenderItem& previous, const RenderItem& current) {
    if (previous.displayLayer != current.displayLayer) return false;
    return previous.batchKey == current.batchKey;
}

BatchBreakReason BatchBuilder::getBreakReason(const RenderItem& previous,
                                              const RenderItem& current) {
    if (previous.displayLayer != current.displayLayer) {
        return BatchBreakReason::DisplayLayer;
    }
    if (previous.batchKey.shaderName != current.batchKey.shaderName) {
        return BatchBreakReason::Shader;
    }
    if (previous.batchKey.primaryTexture != current.batchKey.primaryTexture) {
        return BatchBreakReason::Texture;
    }
    if (!(previous.batchKey == current.batchKey)) {
        return BatchBreakReason::MaterialState;
    }
    return BatchBreakReason::OrderBarrier;
}

BatchBuildResult BatchBuilder::build(const std::vector<RenderItem>& items,
                                     const BatchBuildOptions& options) const {
    BatchBuildResult result;
    if (items.empty()) return result;

    // P0.2 仅提供安全模式。保留 options 是为了后续显式增加可证明安全的 reorder 策略。
    (void)options;

    result.groups.reserve(items.size());
    result.breakReasons.reserve(items.size() - 1);

    BatchGroup currentGroup;
    currentGroup.itemIndices.push_back(0);

    for (uint32_t index = 1; index < static_cast<uint32_t>(items.size()); ++index) {
        const auto& previous = items[index - 1];
        const auto& current = items[index];
        if (canBatch(previous, current)) {
            currentGroup.itemIndices.push_back(index);
            continue;
        }

        result.groups.emplace_back(std::move(currentGroup));
        result.breakReasons.push_back(getBreakReason(previous, current));
        currentGroup = {};
        currentGroup.itemIndices.push_back(index);
    }

    result.groups.emplace_back(std::move(currentGroup));
    return result;
}

} // namespace morrow
