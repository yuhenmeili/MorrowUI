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
    std::string shaderName;
    uint64_t shaderVariantHash = 0;
    uint64_t textureSetHash = 0;
    const void* primaryTexture = nullptr;
    bool blendEnabled = true;
    int32_t srcBlendFactor = 0;
    int32_t dstBlendFactor = 0;
    bool doubleSided = false;
    uint64_t materialStateHash = 0;
    uint32_t primitiveTopology = 0;
    uint32_t vertexLayoutMask = 0;
    uintptr_t renderTargetId = 0;
    uint64_t clipStateId = 0;
    uint64_t stencilStateId = 0;
    uint64_t ssboLayoutHash = 0;

    bool operator==(const BatchCompatibilityKey& other) const {
        return shaderName == other.shaderName &&
               shaderVariantHash == other.shaderVariantHash &&
               textureSetHash == other.textureSetHash &&
               primaryTexture == other.primaryTexture &&
               blendEnabled == other.blendEnabled &&
               srcBlendFactor == other.srcBlendFactor &&
               dstBlendFactor == other.dstBlendFactor &&
               doubleSided == other.doubleSided &&
               materialStateHash == other.materialStateHash &&
               primitiveTopology == other.primitiveTopology &&
               vertexLayoutMask == other.vertexLayoutMask &&
               renderTargetId == other.renderTargetId &&
               clipStateId == other.clipStateId &&
               stencilStateId == other.stencilStateId &&
               ssboLayoutHash == other.ssboLayoutHash;
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

    static bool areRenderItemListsEquivalent(const std::vector<RenderItem>& current,
                                             const std::vector<RenderItem>& previous);

private:
    static bool canBatch(const RenderItem& previous, const RenderItem& current);
    static BatchBreakReason getBreakReason(const RenderItem& previous,
                                           const RenderItem& current);
};

} // namespace morrow

#endif // MORROW_BATCHBUILDER_H
