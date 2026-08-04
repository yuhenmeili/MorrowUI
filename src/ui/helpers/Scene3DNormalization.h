#ifndef MORROW_SCENE3D_NORMALIZATION_H
#define MORROW_SCENE3D_NORMALIZATION_H

#include "Vector3.h"

namespace morrow {
struct Scene3DNormalizationOptions {
    /// Normalize loaded GLTF scenes by default; callers may opt out.
    bool enabled = true;
    /// Radius of the world-space AABB sphere after normalization.
    float targetRadius = 2.5f;
    /// Reject degenerate bounds instead of producing an extreme scale.
    float minSourceRadius = 0.0001f;
    float minScale = 0.0001f;
    float maxScale = 10000.0f;
};

struct Scene3DNormalizationResult {
    bool applied = false;
    float uniformScale = 1.0f;
    float sourceRadius = 0.0f;
    float normalizedRadius = 0.0f;
    Math::Vector3 boundsMin;
    Math::Vector3 boundsMax;
};

Scene3DNormalizationResult calculateScene3DNormalization(
    const Math::Vector3& boundsMin,
    const Math::Vector3& boundsMax,
    const Scene3DNormalizationOptions& options);
} // namespace morrow

#endif // MORROW_SCENE3D_NORMALIZATION_H
