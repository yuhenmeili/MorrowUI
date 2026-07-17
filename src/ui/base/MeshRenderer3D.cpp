//
// Created by lance on 2026/3/31.
//

#include "MeshRenderer3D.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "GlobalObject.h"
#include "Transform3D.h"
#include "Texture.h"
#include "RenderDeviceProxy.h"
#include "PerspectiveCamera.h"
#include "Scene3DUBO.h"

namespace morrow {
namespace {
template <typename T>
std::shared_ptr<UBOData> makeUBOData(const T& payload) {
    auto* pool = RENDERINGTHREAD->getUBODataRecyclePool();
    auto uboData = pool ? pool->acquire() : std::make_shared<UBOData>();
    uboData->size = static_cast<uint32_t>(sizeof(T));
    uboData->data.resize(sizeof(T));
    std::memcpy(uboData->data.data(), &payload, sizeof(T));
    return uboData;
}

TextureSharedPtr createTextureFromGLTFData(const TextureData& textureData) {
    if (!textureData.pixels || textureData.width <= 0 || textureData.height <= 0) {
        return nullptr;
    }

    auto texture = Texture::create(textureData.imageType);
    texture->setTextureData(
        std::shared_ptr<unsigned char>(
            static_cast<unsigned char*>(textureData.pixels),
            [owner = textureData.pixelOwner](unsigned char*) mutable {
                owner.reset();
            }),
        textureData.width,
        textureData.height,
        textureData.format,
        textureData.bytes,
        textureData.compressedTexture);
    texture->setMinFilterType(textureData.minFilterType);
    texture->setMagFilterType(textureData.magFilterType);
    return texture;
}
} // namespace

// ---------------------------------------------------------------------------
// Internal primitive state (one per GLTFPrimitive)
// ---------------------------------------------------------------------------
struct Prim3D {
    MaterialSharedPtr material;
    VBODataSharedPtr vboData;
    HwVBO vbo{0}; // created lazily
    bool uploaded = false; // true after first updateVBO
};

// We store the primitives list behind a shared_ptr so the class stays
// move-constructible after the opaque internal struct.
struct MeshRenderer3D::Impl {
    std::vector<Prim3D> prims;
    HwUBO drawUbo{0};
};

// ---------------------------------------------------------------------------

void MeshRenderer3D::setFromGLTFMesh(const GLTFMesh& mesh, const std::vector<GLTFMaterial>& materials, const std::string& shaderName) {
    m_impl = std::make_unique<Impl>();

    for (const auto& prim : mesh.primitives) {
        Prim3D pd;
        pd.vboData = prim.vboData;

        // Create material with the requested shader
        pd.material = Material::create(shaderName);
        pd.material->setBlendEnabled(false);
        pd.material->setDoubleSided(false);
        pd.material->setScene3DMaterialUBO(buildScene3DMaterialUBO(nullptr));

        // Configure material from GLTFMaterial
        if (prim.materialIndex >= 0 && prim.materialIndex < int(materials.size())) {
            const GLTFMaterial& gm = materials[prim.materialIndex];

            pd.material->setBlendEnabled(gm.alphaBlend);
            pd.material->setDoubleSided(gm.doubleSided);
            pd.material->setScene3DMaterialUBO(buildScene3DMaterialUBO(&gm));

            if (auto tex = createTextureFromGLTFData(gm.baseColorTexture)) {
                pd.material->setTexture("baseColorTexture", tex);
            }
            if (auto tex = createTextureFromGLTFData(gm.metallicRoughnessTexture)) {
                pd.material->setTexture("metallicRoughnessTexture", tex);
            }
            if (auto tex = createTextureFromGLTFData(gm.normalTexture)) {
                pd.material->setTexture("normalTexture", tex);
            }
            if (auto tex = createTextureFromGLTFData(gm.occlusionTexture)) {
                pd.material->setTexture("occlusionTexture", tex);
            }
            if (auto tex = createTextureFromGLTFData(gm.emissiveTexture)) {
                pd.material->setTexture("emissiveTexture", tex);
            }
        }

        m_impl->prims.push_back(std::move(pd));
    }
}

void MeshRenderer3D::update(FrameStateSharedPtr frameState) {
    if (!m_impl || m_impl->prims.empty()) return;

    auto transform3D = getComponent<Transform3D>();
    if (!transform3D) return;

    Matrix4 modelMatrix = transform3D->getWorldTransformMatrix();

    Scene3DPassContextSharedPtr passContext;
    Scene3DIBLState ibl;
    HwUBO scene3DFrameUbo{0};
    if (frameState) {
        passContext = frameState->scene3DPassContext;
        if (passContext) {
            ibl = passContext->ibl;
            scene3DFrameUbo = passContext->frameUBO;
        } else {
            ibl = frameState->scene3DIBL;
            scene3DFrameUbo = frameState->scene3DFrameUBO;
        }
    }
    if (!scene3DFrameUbo.isValid()) return;

    if (!m_impl->drawUbo.isValid()) {
        m_impl->drawUbo = RENDERINGTHREAD->createUBO();
    }

    const Scene3DDrawUBO drawUboPayload{modelMatrix};
    RENDERINGTHREAD->updateUBO(m_impl->drawUbo, makeUBOData(drawUboPayload));


    for (auto& pd : m_impl->prims) {
        if (!pd.vboData) continue;

        if (ibl.isValid()) {
            pd.material->setTexture("irradianceTexture", ibl.irradianceTexture);
            pd.material->setTexture("specularTexture", ibl.specularTexture);
            pd.material->setTexture("brdfLUTTexture", ibl.brdfLUTTexture);
        }

        RENDERINGTHREAD->setCullFace(pd.material->isDoubleSided() ? CullFaceMode::NONE : CullFaceMode::BACK);
        RENDERINGTHREAD->setDepthWrite(!pd.material->isBlendEnabled());

        // Apply material – this builds & binds the shader, uploads textures,
        // and sets all stored uniforms (including the matrices above).
        pd.material->apply();

        auto shader = pd.material->getShader();
        if (!shader.isValid()) continue;

        RENDERINGTHREAD->bindUBO(shader, scene3DFrameUbo, "Scene3DFrame", kScene3DFrameBindingPoint);
        RENDERINGTHREAD->bindUBO(shader, m_impl->drawUbo, "Scene3DDraw", kScene3DDrawBindingPoint);
        pd.material->bindScene3DMaterialUBO(shader);

        // Create VBO lazily (first update)
        if (!pd.vbo.isValid()) {
            pd.vbo = RENDERINGTHREAD->createVBO();
        }

        // Upload vertex data exactly once (static mesh)
        if (!pd.uploaded) {
            RENDERINGTHREAD->updateVBO(shader, pd.vbo, pd.vboData);
            pd.uploaded = true;
        }

        // Draw
        if (frameState) {
            frameState->drawCallCount++;
        }
        RENDERINGTHREAD->drawVBO(pd.vbo, 1);
    }

    RENDERINGTHREAD->setCullFace(CullFaceMode::BACK);
    RENDERINGTHREAD->setDepthWrite(true);
}

MeshRenderer3D::MeshRenderer3D() = default;

MeshRenderer3D::~MeshRenderer3D() {
    if (!m_impl) return;

    for (auto& prim : m_impl->prims) {
        if (prim.vbo.isValid()) {
            RENDERINGTHREAD->deleteVBO(prim.vbo);
            prim.vbo = HwVBO{0};
        }
    }
}
} // namespace morrow
