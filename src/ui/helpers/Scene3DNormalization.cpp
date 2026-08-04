#include "Scene3DNormalization.h"

#include <algorithm>
#include <cmath>

namespace morrow {
namespace {

bool isFinite(const Math::Vector3& value) {
    return std::isfinite(value.x) &&
           std::isfinite(value.y) &&
           std::isfinite(value.z);
}

} // namespace

Scene3DNormalizationResult calculateScene3DNormalization(
    const Math::Vector3& boundsMin,
    const Math::Vector3& boundsMax,
    const Scene3DNormalizationOptions& options) {
    Scene3DNormalizationResult result;
    result.boundsMin = boundsMin;
    result.boundsMax = boundsMax;

    if (!options.enabled ||
        !isFinite(boundsMin) ||
        !isFinite(boundsMax) ||
        !std::isfinite(options.targetRadius) ||
        options.targetRadius <= 0.0f) {
        return result;
    }

    const Math::Vector3 extent = boundsMax - boundsMin;
    if (extent.x < 0.0f || extent.y < 0.0f || extent.z < 0.0f) {
        return result;
    }

    result.sourceRadius = extent.length() * 0.5f;
    const float minSourceRadius = std::max(options.minSourceRadius, 0.000001f);
    if (!std::isfinite(result.sourceRadius) ||
        result.sourceRadius < minSourceRadius) {
        return result;
    }

    const float minScale = std::max(options.minScale, 0.000001f);
    const float maxScale = std::max(options.maxScale, minScale);
    result.uniformScale = std::clamp(
        options.targetRadius / result.sourceRadius,
        minScale,
        maxScale);
    result.normalizedRadius = result.sourceRadius * result.uniformScale;
    result.boundsMin = boundsMin * result.uniformScale;
    result.boundsMax = boundsMax * result.uniformScale;
    result.applied = true;
    return result;
}

} // namespace morrow
