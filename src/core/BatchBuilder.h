#ifndef MORROW_BATCHBUILDER_H
#define MORROW_BATCHBUILDER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "renderer/BatchStatistics.h"

namespace morrow {

class Material;
class MeshFilter;
class Transform;

struct RenderItem {
    std::shared_ptr<Material> material;
    std::shared_ptr<MeshFilter> meshFilter;
    std::shared_ptr<Transform> transform;
    int32_t displayLayer = 0;
    uint32_t insertionIndex = 0;

    bool isSameRenderableAs(const RenderItem& other) const {
        return material.get() == other.material.get() &&
               meshFilter.get() == other.meshFilter.get() &&
               transform.get() == other.transform.get() &&
               displayLayer == other.displayLayer;
    }
};

struct BatchBuildOptions {
    // P0.2 默认严格保持 painter's order，只合并连续兼容项。
    bool preservePainterOrder = true;
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
    BatchBuildResult build(const std::vector<RenderItem>& items,
                           const BatchBuildOptions& options = {}) const;

private:
    static bool canBatch(const RenderItem& previous, const RenderItem& current);
    static BatchBreakReason getBreakReason(const RenderItem& previous,
                                           const RenderItem& current);
    static const void* getPrimaryTextureIdentity(const std::shared_ptr<Material>& material);
};

} // namespace morrow

#endif // MORROW_BATCHBUILDER_H
