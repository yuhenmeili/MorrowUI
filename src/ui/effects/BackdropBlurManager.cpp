//
// BackdropBlurManager — 共享 Kawase 背景模糊链的帧编排器（实现）。
// 设计说明见 BackdropBlurManager.h 与 docs/road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md。
//

#include "BackdropBlurManager.h"

#include <algorithm>

#include "FrameState.h"
#include "GlobalObject.h"
#include "GpuTypes.h"
#include "OrthographicCamera.h"

namespace morrow {

void BackdropBlurManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    // 开关只翻转标志位：关闭后组件走 tint 降级路径、本管理器不再分段帧结构；
    // RT 链短暂保留以便快速重开（正式生命周期管理见提案 S2 的 fence 释放）。
    REQUESTRENDER;
}

bool BackdropBlurManager::isEnabled() const {
    return m_enabled;
}

void BackdropBlurManager::submitQuad(const Matrix4& worldMatrix, const Vector2& size, float rounding, const Vector4& tint, int32_t displayLayer) {
    BlurQuad quad;
    quad.worldMatrix = worldMatrix;
    quad.size = size;
    quad.rounding = rounding;
    quad.tint = tint;
    quad.displayLayer = displayLayer;
    m_boundaryLayer = std::min(m_boundaryLayer, displayLayer);
    m_quads.push_back(quad);
}

bool BackdropBlurManager::isFrameActive() const {
    return m_enabled && !m_quads.empty();
}

int32_t BackdropBlurManager::getBoundaryLayer() const {
    return m_boundaryLayer;
}

bool BackdropBlurManager::beginBackdropPass(const FrameStateSharedPtr& frameState) {
    if (!isFrameActive() || !frameState) {
        return false;
    }
    ensureResources(frameState->framebufferWidth, frameState->framebufferHeight);
    if (!m_backdropRT || !m_backdropRT->getRenderTarget().isValid()) {
        return false;
    }

    RENDERINGTHREAD->setScissorRect(false, 0, 0, 0, 0);
    RENDERINGTHREAD->bindRenderTarget(m_backdropRT->getRenderTarget());
    // 与窗口同色清屏：回屏合成后背景与无模糊路径逐像素一致（提案 §5.1）
    const Vector4& clear = frameState->clearColor;
    RENDERINGTHREAD->setClearColor(clear.x, clear.y, clear.z, clear.w);
    RENDERINGTHREAD->clear();
    return true;
}

void BackdropBlurManager::renderBackdropChain(const FrameStateSharedPtr& frameState) {
    if (!frameState || !m_backdropRT || !m_chainPingRT || !m_chainPongRT) {
        return;
    }

    RENDERINGTHREAD->setScissorRect(false, 0, 0, 0, 0);
    RENDERINGTHREAD->unbindRenderTarget(); // 结束 backdrop 段，回到默认帧缓冲

    // 阶段 3b：模糊链（S1 固定半径档 = 1/2 分辨率 downsample + 2 次 kawase）
    drawChainPass(frameState, *m_chainPingRT, m_backdropRT->getColorTexture(), *m_downsampleMaterial, 0.0f);
    drawChainPass(frameState, *m_chainPongRT, m_chainPingRT->getColorTexture(), *m_kawaseMaterial, 0.5f);
    drawChainPass(frameState, *m_chainPingRT, m_chainPongRT->getColorTexture(), *m_kawaseMaterial, 1.5f);

    // 结束链回到默认帧缓冲。注意设备侧的 viewport 恢复是"单槽保存"而非栈：
    // 链上多次 bind 后 unbind 恢复的是 RT 尺寸 viewport，必须显式恢复全屏，
    // 否则合成 / 面片 / 前景批次会画进左下四分之一屏幕。
    RENDERINGTHREAD->unbindRenderTarget();
    RENDERINGTHREAD->setViewPort(0, 0, frameState->framebufferWidth, frameState->framebufferHeight);

    // 阶段 3c：回屏背景恢复拷贝（backdrop RT → 默认帧缓冲）
    renderComposite(frameState);

    // 阶段 3c：模糊面片合并绘制（采样 L0' = ping RT）
    renderBlurQuads(frameState);
}

void BackdropBlurManager::endFrame() {
    m_quads.clear();
    m_boundaryLayer = kNoBoundary;
}

void BackdropBlurManager::destroy() {
    if (m_backdropRT) {
        m_backdropRT->destroy();
    }
    if (m_chainPingRT) {
        m_chainPingRT->destroy();
    }
    if (m_chainPongRT) {
        m_chainPongRT->destroy();
    }
    m_backdropRT.reset();
    m_chainPingRT.reset();
    m_chainPongRT.reset();

    if (m_passQuadVBO.isValid()) {
        RENDERINGTHREAD->deleteVBO(m_passQuadVBO);
        m_passQuadVBO = HwVBO{0};
        m_passQuadUploaded = false;
    }
    if (m_blurQuadVBO.isValid()) {
        RENDERINGTHREAD->deleteVBO(m_blurQuadVBO);
        m_blurQuadVBO = HwVBO{0};
    }

    m_downsampleMaterial.reset();
    m_kawaseMaterial.reset();
    m_compositeMaterial.reset();
    m_blurQuadMaterial.reset();
    m_passQuadData.reset();
    m_quads.clear();
    m_rtWidth = m_rtHeight = 0;
}

// ---------------------------------------------------------------------------
// 内部实现
// ---------------------------------------------------------------------------

void BackdropBlurManager::ensureResources(int32_t framebufferWidth, int32_t framebufferHeight) {
    // backdrop 段与链均在 1/2 分辨率上工作（提案 §7 的关键成本杠杆：
    // 输出本来就是模糊的，半分辨率的质量损失被模糊本身掩盖）
    const int32_t rtWidth = std::max(1, framebufferWidth / 2);
    const int32_t rtHeight = std::max(1, framebufferHeight / 2);
    if (!m_backdropRT) {
        m_backdropRT = std::make_unique<OffscreenRenderTarget>();
        m_chainPingRT = std::make_unique<OffscreenRenderTarget>();
        m_chainPongRT = std::make_unique<OffscreenRenderTarget>();
        m_backdropRT->create(rtWidth, rtHeight);
        m_chainPingRT->create(rtWidth, rtHeight);
        m_chainPongRT->create(rtWidth, rtHeight);

        m_downsampleMaterial = Material::create("backdrop_downsample");
        m_downsampleMaterial->setBlendEnabled(false);
        m_kawaseMaterial = Material::create("backdrop_kawase");
        m_kawaseMaterial->setBlendEnabled(false);
        m_compositeMaterial = Material::create("backdrop_composite");
        m_compositeMaterial->setBlendEnabled(false);
        // 模糊面片保持默认管线状态（标准 alpha 混合）
        m_blurQuadMaterial = Material::create("backdrop");

        buildPassQuad();
    } else if (rtWidth != m_rtWidth || rtHeight != m_rtHeight) {
        m_backdropRT->resize(rtWidth, rtHeight);
        m_chainPingRT->resize(rtWidth, rtHeight);
        m_chainPongRT->resize(rtWidth, rtHeight);
    }
    m_rtWidth = rtWidth;
    m_rtHeight = rtHeight;
}

void BackdropBlurManager::buildPassQuad() {
    // 裁剪空间全屏四边形，顶点顺序与 UV 朝向沿用 MR3DSceneView 显示面片约定：
    // NDC (-1,+1) 为屏幕左上 ↔ 纹理 V=1（GL 纹理原点在左下）
    auto vboData = std::make_shared<VBOData>();
    vboData->vertexCount = 4;
    vboData->indexCount = 6;
    vboData->drawMode = PrimitiveType::TRIANGLES;

    const Vector3 positions[4] = {
        Vector3(-1.0f, 1.0f, 0.0f),
        Vector3(1.0f, 1.0f, 0.0f),
        Vector3(1.0f, -1.0f, 0.0f),
        Vector3(-1.0f, -1.0f, 0.0f),
    };
    const Vector2 uvs[4] = {
        Vector2(0.0f, 1.0f),
        Vector2(1.0f, 1.0f),
        Vector2(1.0f, 0.0f),
        Vector2(0.0f, 0.0f),
    };
    const uint32_t indices[6] = {0, 1, 2, 0, 2, 3};

    const size_t posSize = 4 * sizeof(Vector3);
    const size_t uvSize = 4 * sizeof(Vector2);
    vboData->vertexData.resize(posSize + uvSize);
    vboData->attributes.push_back({VertexAttributeType::Position, 0, sizeof(Vector3)});
    vboData->attributes.push_back({VertexAttributeType::UV, posSize, sizeof(Vector2)});
    for (int i = 0; i < 4; ++i) {
        auto* p = reinterpret_cast<Vector3*>(vboData->vertexData.data() + i * sizeof(Vector3));
        *p = positions[i];
        auto* uv = reinterpret_cast<Vector2*>(vboData->vertexData.data() + posSize + i * sizeof(Vector2));
        *uv = uvs[i];
    }
    vboData->indices.assign(indices, indices + 6);
    m_passQuadData = std::move(vboData);
}

void BackdropBlurManager::drawChainPass(const FrameStateSharedPtr& frameState, OffscreenRenderTarget& dst, HwTexture2D src,
                                        Material& material, float kawaseOffset) {
    RENDERINGTHREAD->bindRenderTarget(dst.getRenderTarget());
    RENDERINGTHREAD->useTexture2D(src, 0);

    material.setInt("texture", 0);
    material.setVector("texel", Vector2(1.0f / static_cast<float>(m_rtWidth), 1.0f / static_cast<float>(m_rtHeight)));
    material.setFloat("offset", kawaseOffset); // downsample 无 u_offset 声明，驱动侧忽略
    material.apply(); // 先 apply 确保 shader 已构建（updateVBO 依赖有效 program 定位属性）

    if (!m_passQuadVBO.isValid()) {
        m_passQuadVBO = RENDERINGTHREAD->createVBO();
    }
    if (!m_passQuadUploaded) {
        RENDERINGTHREAD->updateVBO(material.getShader(), m_passQuadVBO, m_passQuadData);
        m_passQuadUploaded = true;
    }
    RENDERINGTHREAD->drawVBO(m_passQuadVBO, 1);
    frameState->drawCallCount++;
    frameState->batchStatistics.backdropDrawCallCount++;
}

void BackdropBlurManager::renderComposite(const FrameStateSharedPtr& frameState) {
    RENDERINGTHREAD->useTexture2D(m_backdropRT->getColorTexture(), 0);
    m_compositeMaterial->setInt("texture", 0);
    m_compositeMaterial->apply();
    if (!m_passQuadVBO.isValid() || !m_passQuadUploaded) {
        return;
    }
    RENDERINGTHREAD->drawVBO(m_passQuadVBO, 1);
    frameState->drawCallCount++;
    frameState->batchStatistics.backdropDrawCallCount++;
}

void BackdropBlurManager::renderBlurQuads(const FrameStateSharedPtr& frameState) {
    if (m_quads.empty()) {
        return;
    }
    auto& statistics = frameState->batchStatistics;
    statistics.backdropQuadCount = static_cast<uint32_t>(m_quads.size());

    // 世界坐标 → backdrop 采样 UV（提案 §5.1）：UI 世界空间以屏幕中心为原点、
    // Y 向上，与 GL 纹理 V 轴同向，故 v = (worldY + H/2) / H。
    const float halfScreenW = frameState->framebufferWidth * 0.5f;
    const float halfScreenH = frameState->framebufferHeight * 0.5f;
    const float invScreenW = 1.0f / static_cast<float>(frameState->framebufferWidth);
    const float invScreenH = 1.0f / static_cast<float>(frameState->framebufferHeight);

    // 每面片 4 顶点，属性平面布局（与设备端 glVertexAttribPointer stride=0 约定一致）：
    //   Position = 世界空间角点；UV = backdrop 采样 UV；Normal = (objX, objY, rounding)；
    //   Tangent = (displaySize.x, displaySize.y, 0, 0)；Color = tint。
    const uint32_t vertexCount = static_cast<uint32_t>(m_quads.size()) * 4;
    auto vboData = std::make_shared<VBOData>();
    vboData->vertexCount = vertexCount;
    vboData->indexCount = static_cast<uint32_t>(m_quads.size()) * 6;
    vboData->drawMode = PrimitiveType::TRIANGLES;

    const size_t posBytes = vertexCount * sizeof(Vector3);
    const size_t colorBytes = vertexCount * sizeof(Vector4);
    const size_t uvBytes = vertexCount * sizeof(Vector2);
    const size_t normalBytes = vertexCount * sizeof(Vector3);
    const size_t tangentBytes = vertexCount * sizeof(Vector4);
    size_t offset = 0;
    vboData->attributes.push_back({VertexAttributeType::Position, offset, sizeof(Vector3)});
    offset += posBytes;
    vboData->attributes.push_back({VertexAttributeType::Color, offset, sizeof(Vector4)});
    offset += colorBytes;
    vboData->attributes.push_back({VertexAttributeType::UV, offset, sizeof(Vector2)});
    offset += uvBytes;
    vboData->attributes.push_back({VertexAttributeType::Normal, offset, sizeof(Vector3)});
    offset += normalBytes;
    vboData->attributes.push_back({VertexAttributeType::Tangent, offset, sizeof(Vector4)});
    vboData->vertexData.resize(offset + tangentBytes);

    auto* positions = reinterpret_cast<Vector3*>(vboData->vertexData.data());
    auto* colors = reinterpret_cast<Vector4*>(vboData->vertexData.data() + posBytes);
    auto* uvs = reinterpret_cast<Vector2*>(vboData->vertexData.data() + posBytes + colorBytes);
    auto* normals = reinterpret_cast<Vector3*>(vboData->vertexData.data() + posBytes + colorBytes + uvBytes);
    auto* tangents = reinterpret_cast<Vector4*>(vboData->vertexData.data() + posBytes + colorBytes + uvBytes + normalBytes);
    vboData->indices.reserve(m_quads.size() * 6);

    for (size_t q = 0; q < m_quads.size(); ++q) {
        const BlurQuad& quad = m_quads[q];
        const float halfW = quad.size.x * 0.5f;
        const float halfH = quad.size.y * 0.5f;
        // 顶点顺序与 MeshFilter 四边形一致：左上、右上、右下、左下（对象空间 Y 向上）
        const Vector2 corners[4] = {
            Vector2(-halfW, halfH),
            Vector2(halfW, halfH),
            Vector2(halfW, -halfH),
            Vector2(-halfW, -halfH),
        };
        const uint32_t base = static_cast<uint32_t>(q) * 4;
        for (int c = 0; c < 4; ++c) {
            Vector3 world(corners[c].x, corners[c].y, 0.0f);
            world.apply(quad.worldMatrix);
            positions[base + c] = world;
            uvs[base + c] = Vector2((world.x + halfScreenW) * invScreenW, (world.y + halfScreenH) * invScreenH);
            colors[base + c] = quad.tint;
            normals[base + c] = Vector3(corners[c].x, corners[c].y, quad.rounding);
            tangents[base + c] = Vector4(quad.size.x, quad.size.y, 0.0f, 0.0f);
        }
        vboData->indices.push_back(base);
        vboData->indices.push_back(base + 1);
        vboData->indices.push_back(base + 2);
        vboData->indices.push_back(base);
        vboData->indices.push_back(base + 2);
        vboData->indices.push_back(base + 3);
    }

    if (!m_blurQuadVBO.isValid()) {
        m_blurQuadVBO = RENDERINGTHREAD->createVBO();
    }
    RENDERINGTHREAD->useTexture2D(m_chainPingRT->getColorTexture(), 0);
    m_blurQuadMaterial->setInt("texture", 0);
    m_blurQuadMaterial->setMatrix4("projectionView", frameState->camera->getProjectionView());
    m_blurQuadMaterial->apply();
    RENDERINGTHREAD->updateVBO(m_blurQuadMaterial->getShader(), m_blurQuadVBO, vboData);
    RENDERINGTHREAD->drawVBO(m_blurQuadVBO, 1);
    frameState->drawCallCount++;
    statistics.backdropDrawCallCount++;
}
} // namespace morrow
