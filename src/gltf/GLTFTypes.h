//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_GLTFTYPES_H
#define MORROW_GUI_GLTFTYPES_H

#include <string>
#include <vector>
#include <functional>

#include "GpuTypes.h"
#include "Matrix4.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"

namespace morrow {
using namespace Math;

// ---------------------------------------------------------------------------
// CPU-side intermediate data (no GPU handles, thread-safe to pass around)
// ---------------------------------------------------------------------------

struct GLTFPrimitive {
    VBODataSharedPtr vboData;   // 顶点/法线/UV/索引，复用现有 VBOData
    int materialIndex = -1;
};

struct GLTFMaterial {
    Vector4 baseColorFactor  = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector3 emissiveFactor   = {0.0f, 0.0f, 0.0f};
    float   metallicFactor   = 1.0f;
    float   roughnessFactor  = 1.0f;
    float   normalScale      = 1.0f;
    float   occlusionStrength = 1.0f;
    float   alphaCutoff      = 0.5f;
    TextureData baseColorTexture;
    TextureData metallicRoughnessTexture;
    TextureData normalTexture;
    TextureData occlusionTexture;
    TextureData emissiveTexture;
    int     baseColorTexIndex = -1; // source image index in GLTFScene::images
    int     metallicRoughnessTexIndex = -1;
    int     normalTexIndex = -1;
    int     occlusionTexIndex = -1;
    int     emissiveTexIndex = -1;
    bool    doubleSided = false;
    bool    alphaMask   = false;
    bool    alphaBlend  = false;
};

struct GLTFMesh {
    std::string name;
    std::vector<GLTFPrimitive> primitives;
};

struct GLTFNode {
    std::string name;
    Matrix4 localTransform;         // T×R×S 已合并
    int meshIndex = -1;
    std::vector<int> children;
};

// ---------------------------------------------------------------------------
// Phase 3 – animation
// ---------------------------------------------------------------------------

enum class GLTFAnimationPath { Translation, Rotation, Scale, Weights };
enum class GLTFInterpolation  { Linear, Step, CubicSpline };

struct GLTFAnimationSampler {
    std::vector<float>    input;    // keyframe times (seconds)
    std::vector<float>    output;   // packed output values
    GLTFInterpolation interpolation = GLTFInterpolation::Linear;
};

struct GLTFAnimationChannel {
    int samplerIndex = -1;
    int nodeIndex    = -1;
    GLTFAnimationPath path = GLTFAnimationPath::Translation;
};

struct GLTFAnimation {
    std::string name;
    std::vector<GLTFAnimationSampler> samplers;
    std::vector<GLTFAnimationChannel> channels;
};

// ---------------------------------------------------------------------------
// Top-level scene
// ---------------------------------------------------------------------------

struct GLTFScene {
    std::vector<GLTFNode>      nodes;
    std::vector<GLTFMesh>      meshes;
    std::vector<GLTFMaterial>  materials;
    std::vector<GLTFAnimation> animations;
    std::vector<int>           rootNodes;  // indices into nodes[]
};

// Async load callback: invoked on the worker thread after background parse.
using GLTFLoadCallback = std::function<void(std::shared_ptr<GLTFScene>, const std::string& error)>;

} // namespace morrow

#endif //MORROW_GUI_GLTFTYPES_H

