#ifndef MORROW_GUI_SCENE3D_PASS_CONTEXT_H
#define MORROW_GUI_SCENE3D_PASS_CONTEXT_H

#include <cstdint>
#include <memory>
#include <vector>

#include "Vector3.h"
#include "GpuTypes.h"

namespace morrow {
class Texture;
class PerspectiveCamera;

struct Scene3DLightingState {
    Math::Vector3 sunDirection = {-0.35f, -1.0f, -0.25f};
    Math::Vector3 sunColor = {1.0f, 0.98f, 0.95f};
    float sunIntensity = 4.5f;

    Math::Vector3 ambientColor = {0.45f, 0.48f, 0.56f};
    float ambientIntensity = 0.28f;
};

struct Scene3DIBLState {
    std::shared_ptr<Texture> irradianceTexture;
    std::shared_ptr<Texture> specularTexture;
    std::shared_ptr<Texture> brdfLUTTexture;

    float rgbmRange = 64.0f;
    float intensity = 1.0f;

    std::vector<int32_t> specularMipWidths;
    std::vector<int32_t> specularMipHeights;
    std::vector<int32_t> specularMipOffsetsY;

    [[nodiscard]] bool isValid() const {
        return irradianceTexture && specularTexture && brdfLUTTexture
            && !specularMipWidths.empty()
            && specularMipWidths.size() == specularMipHeights.size()
            && specularMipWidths.size() == specularMipOffsetsY.size();
    }
};

struct Scene3DPassContext {
    std::shared_ptr<PerspectiveCamera> camera;
    HwUBO frameUBO{0};
    Scene3DLightingState lighting;
    Scene3DIBLState ibl;
    int32_t passWidth = 0;
    int32_t passHeight = 0;
};

using Scene3DPassContextSharedPtr = std::shared_ptr<Scene3DPassContext>;
} // namespace morrow

#endif // MORROW_GUI_SCENE3D_PASS_CONTEXT_H

