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
    if (previous.batchKey.primitiveTopology != current.batchKey.primitiveTopology ||
        previous.batchKey.vertexLayoutMask != current.batchKey.vertexLayoutMask) {
        return BatchBreakReason::Geometry;
    }
    if (previous.batchKey.renderTargetId != current.batchKey.renderTargetId) {
        return BatchBreakReason::RenderTarget;
    }
    if (previous.batchKey.clipStateId != current.batchKey.clipStateId ||
        previous.batchKey.stencilStateId != current.batchKey.stencilStateId) {
        return BatchBreakReason::ClipState;
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

bool BatchBuilder::areRenderItemListsEquivalent(const std::vector<RenderItem>& current,
                                                const std::vector<RenderItem>& previous) {
    if (current.size() != previous.size()) return false;
    for (size_t index = 0; index < current.size(); ++index) {
        if (!current[index].isSameRenderableAs(previous[index])) {
            return false;
        }
    }
    return true;
}

} // namespace morrow
