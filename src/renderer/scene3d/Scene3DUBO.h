#ifndef MORROW_GUI_SCENE3D_UBO_H
#define MORROW_GUI_SCENE3D_UBO_H

#include <algorithm>
#include <cstdint>

#include "GLTFTypes.h"
#include "Matrix4.h"
#include "PerspectiveCamera.h"
#include "Scene3DPassContext.h"

namespace morrow {
constexpr int kScene3DMaxIBLMips = 8;
constexpr uint32_t kScene3DFrameBindingPoint = 1;
constexpr uint32_t kScene3DDrawBindingPoint = 2;
constexpr uint32_t kScene3DMaterialBindingPoint = 3;

struct alignas(16) Scene3DFrameUBO {
    Matrix4 projectionView;
    float cameraWorldPos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float sunDirection[4] = {0.0f, -1.0f, 0.0f, 0.0f};
    float sunColorIntensity[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float ambientColorIntensity[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float iblParams[4] = {64.0f, 1.0f, 0.0f, 0.0f};
    int32_t frameFlags[4] = {0, 0, 0, 0};
    int32_t specularMipInfo[kScene3DMaxIBLMips][4] = {};
};

struct alignas(16) Scene3DDrawUBO {
    Matrix4 modelMatrix;
};

struct alignas(16) Scene3DMaterialUBO {
    float baseColorFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float emissiveFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float materialParams[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // metallic, roughness, normalScale, occlusionStrength
    float alphaParams[4] = {0.5f, 0.0f, 0.0f, 0.0f};
    int32_t materialFlags0[4] = {0, 0, 0, 0};  // alphaMask, doubleSided, hasBaseColor, hasMR
    int32_t materialFlags1[4] = {0, 0, 0, 0};  // hasNormal, hasOcclusion, hasEmissive, reserved
};

inline Scene3DFrameUBO buildScene3DFrameUBO(const Scene3DPassContext& passContext) {
    Scene3DFrameUBO ubo;
    const Matrix4 projectionView = passContext.camera ? passContext.camera->getProjectionView() : Matrix4();
    const Vector3 cameraWorldPos = passContext.camera ? passContext.camera->getPosition() : Vector3();
    const auto& lighting = passContext.lighting;
    const auto& ibl = passContext.ibl;

    ubo.projectionView = projectionView;
    ubo.cameraWorldPos[0] = cameraWorldPos.x;
    ubo.cameraWorldPos[1] = cameraWorldPos.y;
    ubo.cameraWorldPos[2] = cameraWorldPos.z;
    ubo.sunDirection[0] = lighting.sunDirection.x;
    ubo.sunDirection[1] = lighting.sunDirection.y;
    ubo.sunDirection[2] = lighting.sunDirection.z;
    ubo.sunColorIntensity[0] = lighting.sunColor.x;
    ubo.sunColorIntensity[1] = lighting.sunColor.y;
    ubo.sunColorIntensity[2] = lighting.sunColor.z;
    ubo.sunColorIntensity[3] = lighting.sunIntensity;
    ubo.ambientColorIntensity[0] = lighting.ambientColor.x;
    ubo.ambientColorIntensity[1] = lighting.ambientColor.y;
    ubo.ambientColorIntensity[2] = lighting.ambientColor.z;
    ubo.ambientColorIntensity[3] = lighting.ambientIntensity;
    ubo.iblParams[0] = ibl.rgbmRange;
    ubo.iblParams[1] = ibl.intensity;
    ubo.frameFlags[0] = ibl.isValid() ? 1 : 0;
    ubo.frameFlags[1] = ibl.isValid() ? int32_t(std::min<size_t>(ibl.specularMipWidths.size(), kScene3DMaxIBLMips)) : 0;
    for (int32_t i = 0; i < ubo.frameFlags[1]; ++i) {
        ubo.specularMipInfo[i][0] = ibl.specularMipWidths[static_cast<size_t>(i)];
        ubo.specularMipInfo[i][1] = ibl.specularMipHeights[static_cast<size_t>(i)];
        ubo.specularMipInfo[i][2] = ibl.specularMipOffsetsY[static_cast<size_t>(i)];
    }
    return ubo;
}

inline Scene3DMaterialUBO buildScene3DMaterialUBO(const GLTFMaterial* material) {
    Scene3DMaterialUBO ubo;
    if (!material) {
        return ubo;
    }

    ubo.baseColorFactor[0] = material->baseColorFactor.x;
    ubo.baseColorFactor[1] = material->baseColorFactor.y;
    ubo.baseColorFactor[2] = material->baseColorFactor.z;
    ubo.baseColorFactor[3] = material->baseColorFactor.w;
    ubo.emissiveFactor[0] = material->emissiveFactor.x;
    ubo.emissiveFactor[1] = material->emissiveFactor.y;
    ubo.emissiveFactor[2] = material->emissiveFactor.z;
    ubo.materialParams[0] = material->metallicFactor;
    ubo.materialParams[1] = material->roughnessFactor;
    ubo.materialParams[2] = material->normalScale;
    ubo.materialParams[3] = material->occlusionStrength;
    ubo.alphaParams[0] = material->alphaCutoff;
    ubo.materialFlags0[0] = material->alphaMask ? 1 : 0;
    ubo.materialFlags0[1] = material->doubleSided ? 1 : 0;
    ubo.materialFlags0[2] = material->baseColorTexIndex >= 0 ? 1 : 0;
    ubo.materialFlags0[3] = material->metallicRoughnessTexIndex >= 0 ? 1 : 0;
    ubo.materialFlags1[0] = material->normalTexIndex >= 0 ? 1 : 0;
    ubo.materialFlags1[1] = material->occlusionTexIndex >= 0 ? 1 : 0;
    ubo.materialFlags1[2] = material->emissiveTexIndex >= 0 ? 1 : 0;
    return ubo;
}
}  // namespace morrow

#endif  // MORROW_GUI_SCENE3D_UBO_H
