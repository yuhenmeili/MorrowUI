#ifndef MORROW_BATCHBUILDER_H
#define MORROW_BATCHBUILDER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "renderer/BatchStatistics.h"

namespace morrow {
class Material;
class MeshFilter;
class Transform;

struct BatchCompatibilityKey {
    uint64_t materialStateHash = 0;
    uint64_t meshStateHash = 0;

    bool operator==(const BatchCompatibilityKey& other) const {
        return materialStateHash == other.materialStateHash &&
               meshStateHash == other.meshStateHash;
    }
};

struct RenderItem {
    std::shared_ptr<Material> material;
    std::shared_ptr<MeshFilter> meshFilter;
    std::shared_ptr<Transform> transform;
    BatchCompatibilityKey batchKey;
    int32_t displayLayer = 0;
    uint32_t insertionIndex = 0;

    // Compares only state that can change batch grouping. Mesh contents and
    // per-object material parameters are handled by their upload/apply paths.
    bool isSameRenderableAs(const RenderItem& other) const {
        return material.get() == other.material.get() &&
               meshFilter.get() == other.meshFilter.get() &&
               transform.get() == other.transform.get() &&
               displayLayer == other.displayLayer &&
               batchKey == other.batchKey;
    }
};

struct BatchGroup {
    std::vector<uint32_t> itemIndices;
};

struct BatchBuildResult {
    std::vector<BatchGroup> groups;
    std::vector<BatchBreakReason> breakReasons;
};

/// 纯 CPU 批次构建器：不创建 GPU 资源、不依赖 Window/FrameState/RenderingThread。
class BatchBuilder {
public:
    BatchBuildResult build(const std::vector<RenderItem>& items) const;

    static bool areRenderItemListsEquivalent(const std::vector<RenderItem>& current, const std::vector<RenderItem>& previous);

private:
    static bool canBatch(const RenderItem& previous, const RenderItem& current);

    static BatchBreakReason getBreakReason(const RenderItem& previous, const RenderItem& current);
};
} // namespace morrow

#endif // MORROW_BATCHBUILDER_H