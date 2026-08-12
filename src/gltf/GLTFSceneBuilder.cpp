//
// Created by lance on 2026/3/31.
//

#include "GLTFSceneBuilder.h"

#include <functional>

#include "Texture.h"
#include "base/MeshRenderer3D.h"
#include "base/SceneNode.h"

namespace morrow {
namespace {
void expandBounds(Vector3& outMin, Vector3& outMax, const Vector3& point, bool& hasBounds) {
    if (!hasBounds) {
        outMin = point;
        outMax = point;
        hasBounds = true;
        return;
    }

    outMin.min(point);
    outMax.max(point);
}

void accumulatePrimitiveBounds(const GLTFPrimitive& primitive, const Matrix4& worldMatrix, Vector3& outMin, Vector3& outMax, bool& hasBounds) {
    if (!primitive.vboData || primitive.vboData->vertexCount == 0)
        return;

    const void* positionData = primitive.vboData->getAttributeData(VertexAttributeType::Position);
    if (!positionData)
        return;

    const auto* positions = static_cast<const float*>(positionData);
    for (uint32_t i = 0; i < primitive.vboData->vertexCount; ++i) {
        Vector3 point(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
        point.apply(worldMatrix);
        expandBounds(outMin, outMax, point, hasBounds);
    }
}
}  // namespace

std::shared_ptr<SceneNode> GLTFSceneBuilder::build(const std::shared_ptr<GLTFScene>& scene, const std::string& shaderName) {
    if (!scene)
        return nullptr;

    std::vector<TextureSharedPtr> textures(scene->textures.size());
    for (size_t i = 0; i < scene->textures.size(); ++i) {
        const auto& textureInfo = scene->textures[i];
        if (textureInfo.imageIndex < 0 || textureInfo.imageIndex >= static_cast<int>(scene->images.size())) {
            continue;
        }
        const auto& image = scene->images[textureInfo.imageIndex];
        if (!image.pixels || image.width <= 0 || image.height <= 0) {
            continue;
        }

        auto texture = Texture::create(ImageType::IMAGE);
        texture->setTextureData(image.pixels, image.width, image.height, image.format, false);
        texture->setMinFilterType(textureInfo.minFilterType);
        texture->setMagFilterType(textureInfo.magFilterType);
        textures[i] = std::move(texture);
    }

    // Create an invisible root SceneNode that groups all top-level nodes.
    auto root = std::make_shared<SceneNode>();
    root->setWidgetName("GLTFSceneRoot");

    for (int rootIdx : scene->rootNodes) {
        auto child = buildNode(*scene, textures, rootIdx, shaderName);
        if (child)
            root->addSceneChild(child);
    }

    // If no rootNodes list, treat all nodes as roots (degenerate case)
    if (scene->rootNodes.empty()) {
        for (int i = 0; i < int(scene->nodes.size()); ++i) {
            auto child = buildNode(*scene, textures, i, shaderName);
            if (child)
                root->addSceneChild(child);
        }
    }

    return root;
}

bool GLTFSceneBuilder::computeBounds(const std::shared_ptr<GLTFScene>& scene, Vector3& outMin, Vector3& outMax) {
    if (!scene)
        return false;

    bool hasBounds = false;
    std::function<void(int, const Matrix4&)> visitNode;
    visitNode = [&](int nodeIndex, const Matrix4& parentWorld) {
        if (nodeIndex < 0 || nodeIndex >= int(scene->nodes.size()))
            return;

        const GLTFNode& node = scene->nodes[nodeIndex];
        Matrix4 worldMatrix = parentWorld * node.localTransform;

        if (node.meshIndex >= 0 && node.meshIndex < int(scene->meshes.size())) {
            const GLTFMesh& mesh = scene->meshes[node.meshIndex];
            for (const auto& primitive : mesh.primitives) {
                accumulatePrimitiveBounds(primitive, worldMatrix, outMin, outMax, hasBounds);
            }
        }

        for (int childIdx : node.children) {
            visitNode(childIdx, worldMatrix);
        }
    };

    if (!scene->rootNodes.empty()) {
        Matrix4 identity;
        for (int rootIdx : scene->rootNodes) {
            visitNode(rootIdx, identity);
        }
    } else {
        Matrix4 identity;
        for (int i = 0; i < int(scene->nodes.size()); ++i) {
            visitNode(i, identity);
        }
    }

    return hasBounds;
}

std::shared_ptr<SceneNode> GLTFSceneBuilder::buildNode(const GLTFScene& scene, const std::vector<TextureSharedPtr>& textures, int nodeIndex, const std::string& shaderName) {
    if (nodeIndex < 0 || nodeIndex >= int(scene.nodes.size()))
        return nullptr;

    const GLTFNode& node = scene.nodes[nodeIndex];

    auto sceneNode = std::make_shared<SceneNode>();
    sceneNode->setWidgetName(node.name.empty() ? ("GLTFNode_" + std::to_string(nodeIndex)) : node.name);

    auto transform = sceneNode->getTransform();
    transform->setLocalTransformMatrix(node.localTransform);

    // MeshRenderer3D component – only if this node owns a mesh
    if (node.meshIndex >= 0 && node.meshIndex < int(scene.meshes.size())) {
        auto renderer = sceneNode->addComponent<MeshRenderer3D>();
        renderer->setFromGLTFMesh(scene.meshes[node.meshIndex], scene.materials, textures, shaderName);
    }

    // Recurse into children
    for (int childIdx : node.children) {
        auto childWidget = buildNode(scene, textures, childIdx, shaderName);
        if (childWidget)
            sceneNode->addSceneChild(childWidget);
    }

    return sceneNode;
}
}  // namespace morrow
