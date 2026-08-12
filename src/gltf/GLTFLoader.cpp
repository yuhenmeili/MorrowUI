//
// Created by lance on 2026/3/31.
//

// Standard headers must come before windows.h (pulled in by tiny_gltf.h on MinGW).
#include "GLTFLoader.h"

#include <cstring>
#include <thread>

#include "DriverEnums.h"
#include "Log.h"
#include "tinygltf/tiny_gltf.h"

namespace morrow {

// ---------------------------------------------------------------------------
// Forward declarations (all helpers are static free functions in this TU)
// ---------------------------------------------------------------------------
static std::shared_ptr<GLTFScene> parse(const std::string& path, std::string& outError);
static VBODataSharedPtr packPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& prim);
static GLTFMaterial parseMaterial(const tinygltf::Model& model, const tinygltf::Material& mat);
static SamplerMinFilter toMinFilter(int filter);
static SamplerMagFilter toMagFilter(int filter);

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void GLTFLoader::loadAsync(const std::string& path, GLTFLoadCallback callback) {
    std::thread([path, callback]() {
        std::string error;
        auto scene = parse(path, error);
        callback(std::move(scene), error);
    }).detach();
}

std::shared_ptr<GLTFScene> GLTFLoader::loadSync(const std::string& path, std::string& outError) {
    return parse(path, outError);
}

// ---------------------------------------------------------------------------
// Core parse  (file-scope, only called from loadSync / loadAsync)
// ---------------------------------------------------------------------------

static std::shared_ptr<GLTFScene> parse(const std::string& path, std::string& outError) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string warn;

    bool ok = false;
    // Detect binary vs. ASCII by extension
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".glb") {
        ok = loader.LoadBinaryFromFile(&model, &outError, &warn, path);
    } else {
        ok = loader.LoadASCIIFromFile(&model, &outError, &warn, path);
    }
    if (!warn.empty()) {
        LOG_W("GLTFLoader: {}", warn);
    }
    if (!ok) {
        LOG_E("GLTFLoader: failed to load '{}': {}", path, outError);
        return nullptr;
    }

    auto scene = std::make_shared<GLTFScene>();

    scene->images.reserve(model.images.size());
    for (auto& image : model.images) {
        GLTFImage result;
        result.width = image.width;
        result.height = image.height;
        result.bytes = static_cast<int32_t>(image.image.size());
        switch (image.component) {
            case 1:
                result.format = PixelDataFormat::R;
                break;
            case 2:
                result.format = PixelDataFormat::RG;
                break;
            case 3:
                result.format = PixelDataFormat::RGB;
                break;
            default:
                result.format = PixelDataFormat::RGBA;
                break;
        }
        result.pixels = std::make_shared<std::vector<unsigned char>>(std::move(image.image));
        scene->images.push_back(std::move(result));
    }

    scene->textures.reserve(model.textures.size());
    for (const auto& texture : model.textures) {
        GLTFTexture result;
        result.imageIndex = texture.source;
        if (texture.sampler >= 0 && texture.sampler < int(model.samplers.size())) {
            result.minFilterType = toMinFilter(model.samplers[texture.sampler].minFilter);
            result.magFilterType = toMagFilter(model.samplers[texture.sampler].magFilter);
        }
        scene->textures.push_back(std::move(result));
    }

    // --- Materials ---
    scene->materials.reserve(model.materials.size());
    for (const auto& mat : model.materials) {
        scene->materials.push_back(parseMaterial(model, mat));
    }

    // --- Meshes ---
    scene->meshes.reserve(model.meshes.size());
    for (const auto& gMesh : model.meshes) {
        GLTFMesh mesh;
        mesh.name = gMesh.name;
        for (const auto& prim : gMesh.primitives) {
            GLTFPrimitive p;
            p.vboData = packPrimitive(model, prim);
            p.materialIndex = prim.material;
            mesh.primitives.push_back(std::move(p));
        }
        scene->meshes.push_back(std::move(mesh));
    }

    // --- Nodes (flat list, children resolved by index) ---
    scene->nodes.reserve(model.nodes.size());
    for (const auto& gNode : model.nodes) {
        GLTFNode node;
        node.name = gNode.name;
        node.meshIndex = gNode.mesh;
        node.children = std::vector<int>(gNode.children.begin(), gNode.children.end());

        if (!gNode.matrix.empty()) {
            // Column-major 4x4 double → float Matrix4
            for (int i = 0; i < 16; ++i)
                node.localTransform.elements[i] = float(gNode.matrix[i]);
        } else {
            // TRS
            Matrix4 T, R, S;

            if (!gNode.translation.empty()) {
                T.makeTranslation(float(gNode.translation[0]), float(gNode.translation[1]), float(gNode.translation[2]));
            }
            if (!gNode.rotation.empty()) {
                // GLTF quaternion: [x, y, z, w]
                Quaternion q(float(gNode.rotation[0]), float(gNode.rotation[1]), float(gNode.rotation[2]), float(gNode.rotation[3]));
                R.makeRotationFromQuaternion(q);
            }
            if (!gNode.scale.empty()) {
                S.makeScale(float(gNode.scale[0]), float(gNode.scale[1]), float(gNode.scale[2]));
            }
            node.localTransform = T * R * S;
        }
        scene->nodes.push_back(std::move(node));
    }

    // --- Root nodes from default scene ---
    int defaultScene = model.defaultScene >= 0 ? model.defaultScene : 0;
    if (!model.scenes.empty()) {
        const auto& gScene = model.scenes[defaultScene];
        scene->rootNodes = std::vector<int>(gScene.nodes.begin(), gScene.nodes.end());
    }

    // --- Animations ---
    scene->animations.reserve(model.animations.size());
    for (const auto& gAnim : model.animations) {
        GLTFAnimation anim;
        anim.name = gAnim.name;

        // Samplers
        for (const auto& gSamp : gAnim.samplers) {
            GLTFAnimationSampler samp;
            // Determine interpolation
            if (gSamp.interpolation == "STEP")
                samp.interpolation = GLTFInterpolation::Step;
            else if (gSamp.interpolation == "CUBICSPLINE")
                samp.interpolation = GLTFInterpolation::CubicSpline;
            else
                samp.interpolation = GLTFInterpolation::Linear;

            // Input (times)
            {
                const auto& acc = model.accessors[gSamp.input];
                const auto& bv = model.bufferViews[acc.bufferView];
                const auto& buf = model.buffers[bv.buffer];
                auto ptr = reinterpret_cast<const float*>(buf.data.data() + bv.byteOffset + acc.byteOffset);
                samp.input.assign(ptr, ptr + acc.count);
            }
            // Output (values)
            {
                const auto& acc = model.accessors[gSamp.output];
                const auto& bv = model.bufferViews[acc.bufferView];
                const auto& buf = model.buffers[bv.buffer];
                auto ptr = reinterpret_cast<const float*>(buf.data.data() + bv.byteOffset + acc.byteOffset);
                // component count per keyframe
                int components = (acc.type == TINYGLTF_TYPE_VEC3) ? 3 : (acc.type == TINYGLTF_TYPE_VEC4) ? 4 : 1;
                samp.output.assign(ptr, ptr + acc.count * components);
            }
            anim.samplers.push_back(std::move(samp));
        }

        // Channels
        for (const auto& gChan : gAnim.channels) {
            GLTFAnimationChannel chan;
            chan.samplerIndex = gChan.sampler;
            chan.nodeIndex = gChan.target_node;
            if (gChan.target_path == "translation")
                chan.path = GLTFAnimationPath::Translation;
            else if (gChan.target_path == "rotation")
                chan.path = GLTFAnimationPath::Rotation;
            else if (gChan.target_path == "scale")
                chan.path = GLTFAnimationPath::Scale;
            else
                chan.path = GLTFAnimationPath::Weights;
            anim.channels.push_back(chan);
        }
        scene->animations.push_back(std::move(anim));
    }

    LOG_I("GLTFLoader: loaded '{}' – {} nodes, {} meshes, {} materials, {} animations", path, scene->nodes.size(), scene->meshes.size(), scene->materials.size(),
          scene->animations.size());

    return scene;
}

// ---------------------------------------------------------------------------
// Primitive packing
// ---------------------------------------------------------------------------

// Helper: read typed buffer data into a float vector.
static std::vector<float> readAccessorFloat(const tinygltf::Model& model, int accIdx) {
    const auto& acc = model.accessors[accIdx];
    const auto& bv = model.bufferViews[acc.bufferView];
    const auto& buf = model.buffers[bv.buffer];

    int components = (acc.type == TINYGLTF_TYPE_VEC2) ? 2 : (acc.type == TINYGLTF_TYPE_VEC3) ? 3 : (acc.type == TINYGLTF_TYPE_VEC4) ? 4 : 1;
    std::vector<float> result(acc.count * components);

    const uint8_t* src = buf.data.data() + bv.byteOffset + acc.byteOffset;
    size_t stride = bv.byteStride ? bv.byteStride : size_t(components) * sizeof(float);

    for (size_t i = 0; i < acc.count; ++i) {
        auto fp = reinterpret_cast<const float*>(src + i * stride);
        for (int c = 0; c < components; ++c)
            result[i * components + c] = fp[c];
    }
    return result;
}

static std::vector<uint32_t> readAccessorIndices(const tinygltf::Model& model, int accIdx) {
    const auto& acc = model.accessors[accIdx];
    const auto& bv = model.bufferViews[acc.bufferView];
    const auto& buf = model.buffers[bv.buffer];
    const uint8_t* src = buf.data.data() + bv.byteOffset + acc.byteOffset;

    std::vector<uint32_t> result(acc.count);
    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        auto p = reinterpret_cast<const uint16_t*>(src);
        for (size_t i = 0; i < acc.count; ++i)
            result[i] = uint32_t(p[i]);
    } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        auto p = reinterpret_cast<const uint32_t*>(src);
        for (size_t i = 0; i < acc.count; ++i)
            result[i] = p[i];
    } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        for (size_t i = 0; i < acc.count; ++i)
            result[i] = uint32_t(src[i]);
    }
    return result;
}

static VBODataSharedPtr packPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& prim) {
    auto vboData = std::make_shared<VBOData>();

    // Positions (mandatory)
    std::vector<float> positions;
    if (prim.attributes.count("POSITION")) {
        positions = readAccessorFloat(model, prim.attributes.at("POSITION"));
        vboData->vertexCount = uint32_t(positions.size() / 3);
    }

    std::vector<float> normals, tangents, uvs;
    bool hasNormals = prim.attributes.count("NORMAL") > 0;
    bool hasTangents = prim.attributes.count("TANGENT") > 0;
    bool hasUVs = prim.attributes.count("TEXCOORD_0") > 0;

    if (hasNormals)
        normals = readAccessorFloat(model, prim.attributes.at("NORMAL"));
    if (hasTangents)
        tangents = readAccessorFloat(model, prim.attributes.at("TANGENT"));
    if (hasUVs)
        uvs = readAccessorFloat(model, prim.attributes.at("TEXCOORD_0"));

    // Indices
    if (prim.indices >= 0) {
        vboData->indices = readAccessorIndices(model, prim.indices);
        vboData->indexCount = uint32_t(vboData->indices.size());
    }

    // Draw mode
    switch (prim.mode) {
        case TINYGLTF_MODE_POINTS:
            vboData->drawMode = PrimitiveType::POINTS;
            break;
        case TINYGLTF_MODE_LINE:
            vboData->drawMode = PrimitiveType::LINES;
            break;
        case TINYGLTF_MODE_LINE_STRIP:
            vboData->drawMode = PrimitiveType::LINE_STRIP;
            break;
        case TINYGLTF_MODE_TRIANGLE_STRIP:
            vboData->drawMode = PrimitiveType::TRIANGLE_STRIP;
            break;
        default:
            vboData->drawMode = PrimitiveType::TRIANGLES;
            break;
    }

    // Build interleaved-style block buffer: pos | normal | tangent | uv
    size_t posSize = positions.size() * sizeof(float);
    size_t normalSize = normals.size() * sizeof(float);
    size_t tangentSize = tangents.size() * sizeof(float);
    size_t uvSize = uvs.size() * sizeof(float);
    size_t totalSize = posSize + normalSize + tangentSize + uvSize;

    vboData->vertexData.resize(totalSize);
    size_t offset = 0;

    vboData->attributes.push_back({VertexAttributeType::Position, offset, sizeof(Vector3)});
    std::memcpy(vboData->vertexData.data() + offset, positions.data(), posSize);
    offset += posSize;

    if (hasNormals) {
        vboData->attributes.push_back({VertexAttributeType::Normal, offset, sizeof(Vector3)});
        std::memcpy(vboData->vertexData.data() + offset, normals.data(), normalSize);
        offset += normalSize;
    }
    if (hasTangents) {
        vboData->attributes.push_back({VertexAttributeType::Tangent, offset, sizeof(Vector4)});
        std::memcpy(vboData->vertexData.data() + offset, tangents.data(), tangentSize);
        offset += tangentSize;
    }
    if (hasUVs) {
        vboData->attributes.push_back({VertexAttributeType::UV, offset, sizeof(Vector2)});
        std::memcpy(vboData->vertexData.data() + offset, uvs.data(), uvSize);
    }

    return vboData;
}

// ---------------------------------------------------------------------------
// Material parsing
// ---------------------------------------------------------------------------

static GLTFMaterial parseMaterial(const tinygltf::Model& model, const tinygltf::Material& mat) {
    GLTFMaterial result;
    result.doubleSided = mat.doubleSided;
    result.alphaBlend = (mat.alphaMode == "BLEND");
    result.alphaMask = (mat.alphaMode == "MASK");
    result.alphaCutoff = float(mat.alphaCutoff);

    const auto& pbr = mat.pbrMetallicRoughness;
    result.baseColorFactor = Vector4(float(pbr.baseColorFactor[0]), float(pbr.baseColorFactor[1]), float(pbr.baseColorFactor[2]), float(pbr.baseColorFactor[3]));
    result.metallicFactor = float(pbr.metallicFactor);
    result.roughnessFactor = float(pbr.roughnessFactor);
    result.emissiveFactor = Vector3(float(mat.emissiveFactor[0]), float(mat.emissiveFactor[1]), float(mat.emissiveFactor[2]));
    result.normalScale = float(mat.normalTexture.scale);
    result.occlusionStrength = float(mat.occlusionTexture.strength);

    if (pbr.baseColorTexture.index >= 0) {
        result.baseColorTexIndex = pbr.baseColorTexture.index;
    }

    if (pbr.metallicRoughnessTexture.index >= 0) {
        result.metallicRoughnessTexIndex = pbr.metallicRoughnessTexture.index;
    }

    if (mat.normalTexture.index >= 0) {
        result.normalTexIndex = mat.normalTexture.index;
    }

    if (mat.occlusionTexture.index >= 0) {
        result.occlusionTexIndex = mat.occlusionTexture.index;
    }

    if (mat.emissiveTexture.index >= 0) {
        result.emissiveTexIndex = mat.emissiveTexture.index;
    }

    return result;
}

static SamplerMinFilter toMinFilter(int filter) {
    switch (filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            return SamplerMinFilter::NEAREST;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return SamplerMinFilter::NEAREST_MIPMAP_NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return SamplerMinFilter::LINEAR_MIPMAP_NEAREST;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case -1:
        default:
            return SamplerMinFilter::LINEAR;
    }
}

static SamplerMagFilter toMagFilter(int filter) {
    switch (filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            return SamplerMagFilter::NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case -1:
        default:
            return SamplerMagFilter::LINEAR;
    }
}

}  // namespace morrow
