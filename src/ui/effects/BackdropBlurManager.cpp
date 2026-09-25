//
// BackdropBlurManager — 共享 Kawase 背景模糊链的帧编排器（实现）。
// 设计说明见 BackdropBlurManager.h 与 docs/road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md。
//

#include "BackdropBlurManager.h"

#include <algorithm>
#include <cmath>

#include "FrameState.h"
#include "GlobalObject.h"
#include "GpuTypes.h"
#include "OrthographicCamera.h"
#include "base/MeshFilter.h"
#include "base/Transform.h"

namespace morrow {

namespace {
// 与 Material.cpp 同款的哈希混合（本地实现，避免拉入工具头）
void combineHash(uint64_t& seed, uint64_t value) {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}
}  // namespace

void BackdropBlurManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    if (enabled) {
        // 手动开启视为覆盖 Off 档；重开后 RT 内容可能已过期，缓存签名失效
        m_disabledByQuality = false;
        m_backdropSignature = 0;
    }
    // 关闭：RT 链短暂保留以便快速重开（§5.7），内容待重开时强制重渲
    REQUESTRENDER;
}

bool BackdropBlurManager::isEnabled() const {
    return m_enabled;
}

void BackdropBlurManager::setQuality(BackdropBlurQuality quality) {
    if (m_quality == quality) {
        return;
    }
    const bool switchingFromOff = (m_quality == BackdropBlurQuality::Off);
    m_quality = quality;

    if (quality == BackdropBlurQuality::Off) {
        if (m_enabled) {
            m_disabledByQuality = true; // 记录关闭由档位引入，切回时自动恢复
            setEnabled(false);
        }
        return;
    }
    // Standard ↔ LowCost：基准分辨率体系变化，释放 RT 链待懒重建；
    // 释放命令经渲染命令流按序执行（天然晚于在飞帧的采样），等价安全回收
    releaseChainResources();
    if (switchingFromOff && m_disabledByQuality) {
        setEnabled(true); // releaseChainResources 已清签名，重开即全量重渲
    } else {
        REQUESTRENDER;
    }
}

BackdropBlurQuality BackdropBlurManager::getQuality() const {
    return m_quality;
}

void BackdropBlurManager::submitQuad(const Matrix4& worldMatrix, const Vector2& size, float rounding, const Vector4& tint, float blurRadius, int32_t displayLayer) {
    BlurQuad quad;
    quad.worldMatrix = worldMatrix;
    quad.size = size;
    quad.rounding = rounding;
    quad.tint = tint;
    quad.level = quantizeRadius(blurRadius);
    quad.displayLayer = displayLayer;
    m_maxNeededLevel = std::max(m_maxNeededLevel, quad.level);
    m_boundaryLayer = std::min(m_boundaryLayer, displayLayer);
    m_quads.push_back(quad);
}

bool BackdropBlurManager::isFrameActive() const {
    return m_enabled && !m_quads.empty();
}

int32_t BackdropBlurManager::getBoundaryLayer() const {
    return m_boundaryLayer;
}

uint8_t BackdropBlurManager::quantizeRadius(float blurRadius) {
    if (blurRadius <= kLevel0MaxRadius) {
        return 0;
    }
    if (blurRadius <= kLevel1MaxRadius) {
        return 1;
    }
    return 2;
}

// ---------------------------------------------------------------------------
// S2 脏标记（§5.5 方案 B）
// ---------------------------------------------------------------------------
bool BackdropBlurManager::computeBackdropDirty(const std::vector<RenderBatch>& batches, const FrameStateSharedPtr& frameState) {
    if (!frameState) {
        m_backdropDirty = true;
        return true;
    }

    // 签名覆盖"backdrop 段会画出不同像素"的全部来源：
    //   帧参数（分辨率 / 清屏色 / 分段边界 / 档位）
    //   批次结构（层数 / underlay / 裁剪矩形 / 项数——Widget 增删与重排）
    //   逐项内容（Transform 世界矩阵版本 → 位移动画；Material uniform 修订 →
    //   颜色/alpha 动画；Mesh 几何修订 → 文本内容变化）
    uint64_t signature = 0xcbf29ce484222325ULL;
    combineHash(signature, static_cast<uint64_t>(frameState->framebufferWidth));
    combineHash(signature, static_cast<uint64_t>(frameState->framebufferHeight));
    combineHash(signature, static_cast<uint64_t>(m_boundaryLayer));
    combineHash(signature, static_cast<uint64_t>(m_quality));
    combineHash(signature, static_cast<uint64_t>(std::lround(frameState->clearColor.x * 255.0f)));
    combineHash(signature, static_cast<uint64_t>(std::lround(frameState->clearColor.y * 255.0f)));
    combineHash(signature, static_cast<uint64_t>(std::lround(frameState->clearColor.z * 255.0f)));
    combineHash(signature, static_cast<uint64_t>(std::lround(frameState->clearColor.w * 255.0f)));

    for (const auto& batch : batches) {
        if (!(batch.isUnderlay || batch.displayLayer < m_boundaryLayer)) {
            continue;
        }
        combineHash(signature, static_cast<uint64_t>(batch.displayLayer));
        combineHash(signature, batch.isUnderlay ? 1U : 0U);
        combineHash(signature, batch.clipRect.enabled ? 1U : 0U);
        if (batch.clipRect.enabled) {
            combineHash(signature, static_cast<uint64_t>(static_cast<int64_t>(std::llround(batch.clipRect.left))));
            combineHash(signature, static_cast<uint64_t>(static_cast<int64_t>(std::llround(batch.clipRect.top))));
            combineHash(signature, static_cast<uint64_t>(static_cast<int64_t>(std::llround(batch.clipRect.right))));
            combineHash(signature, static_cast<uint64_t>(static_cast<int64_t>(std::llround(batch.clipRect.bottom))));
        }
        combineHash(signature, static_cast<uint64_t>(batch.materials.size()));
        for (size_t i = 0; i < batch.materials.size(); ++i) {
            if (batch.transforms[i]) {
                combineHash(signature, batch.transforms[i]->getWorldVersion());
            }
            if (batch.materials[i]) {
                combineHash(signature, batch.materials[i]->getUniformRevision());
                combineHash(signature, batch.materials[i]->getBatchCompatibilityRevision());
            }
            if (batch.meshFilters[i]) {
                combineHash(signature, batch.meshFilters[i]->getGeometryRevision());
            }
        }
    }

    m_backdropDirty = (signature != m_backdropSignature);
    if (m_backdropDirty) {
        // 本帧将渲染的内容与该签名对应；clean 帧沿用
        m_backdropSignature = signature;
    }
    return m_backdropDirty;
}

bool BackdropBlurManager::beginBackdropPass(const FrameStateSharedPtr& frameState) {
    if (!isFrameActive() || !frameState) {
        return false;
    }
    ensureResources(frameState->framebufferWidth, frameState->framebufferHeight);
    if (!m_backdropRT || !m_backdropRT->getRenderTarget().isValid() || m_levels.size() < kLevelCount) {
        return false;
    }
    if (!m_backdropDirty) {
        // 缓存命中：不绑定、不清屏，backdrop RT 内容沿用上一帧（跳过 3a/3b）
        return true;
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
    if (!frameState || !m_backdropRT || m_levels.size() < kLevelCount) {
        return;
    }

    if (m_backdropDirty) {
        RENDERINGTHREAD->setScissorRect(false, 0, 0, 0, 0);
        RENDERINGTHREAD->unbindRenderTarget(); // 结束 backdrop 段

        // 阶段 3b：逐级 downsample + kawase 至本帧最大所需层级（无大半径
        // 面片时低层级整段跳过，§5.2 分级的成本随实际用量走）
        HwTexture2D source = m_backdropRT->getColorTexture();
        int32_t sourceWidth = m_backdropRT->getWidth();
        int32_t sourceHeight = m_backdropRT->getHeight();
        for (uint8_t level = 0; level <= m_maxNeededLevel && level < kLevelCount; ++level) {
            const auto& chain = m_levels[level];
            const int32_t width = chain.downsampledRT->getWidth();
            const int32_t height = chain.downsampledRT->getHeight();
            drawChainPass(frameState, *chain.downsampledRT, source, *m_downsampleMaterial, 0.0f, sourceWidth, sourceHeight);
            drawChainPass(frameState, *chain.blurredRT, chain.downsampledRT->getColorTexture(), *m_kawaseMaterial,
                          static_cast<float>(level) + 0.5f, width, height);
            source = chain.blurredRT->getColorTexture();
            sourceWidth = width;
            sourceHeight = height;
        }

        // 结束链回到默认帧缓冲。注意设备侧的 viewport 恢复是"单槽保存"而非栈：
        // 链上多次 bind 后 unbind 恢复的是 RT 尺寸 viewport，必须显式恢复全屏，
        // 否则合成 / 面片 / 前景批次会画进左下四分之一屏幕。
        RENDERINGTHREAD->unbindRenderTarget();
        RENDERINGTHREAD->setViewPort(0, 0, frameState->framebufferWidth, frameState->framebufferHeight);
    }

    // 阶段 3c：回屏背景恢复拷贝 + 各层级模糊面片（每渲染帧执行——屏幕每帧
    // 被 beginRenderPass 清屏后需要恢复背景；面片 UV/tint 亦可能逐帧变化）
    renderComposite(frameState);
    renderBlurQuads(frameState);
}

void BackdropBlurManager::endFrame() {
    m_quads.clear();
    m_boundaryLayer = kNoBoundary;
    m_maxNeededLevel = 0;
}

void BackdropBlurManager::destroy() {
    releaseChainResources();

    if (m_passQuadVBO.isValid()) {
        RENDERINGTHREAD->deleteVBO(m_passQuadVBO);
        m_passQuadVBO = HwVBO{0};
        m_passQuadUploaded = false;
    }
    m_downsampleMaterial.reset();
    m_kawaseMaterial.reset();
    m_compositeMaterial.reset();
    m_blurQuadMaterial.reset();
    m_passQuadData.reset();
    m_quads.clear();
}

// ---------------------------------------------------------------------------
// 内部实现
// ---------------------------------------------------------------------------

void BackdropBlurManager::ensureResources(int32_t framebufferWidth, int32_t framebufferHeight) {
    // 基准分辨率：Standard = 1/2，LowCost = 1/4（§5.7 档位）。链上层级逐级
    // 减半（Standard → 1/2、1/4、1/8；LowCost → 1/4、1/8、1/16）。
    const int32_t baseShift = (m_quality == BackdropBlurQuality::LowCost) ? 2 : 1;
    const int32_t baseWidth = std::max(1, framebufferWidth >> baseShift);
    const int32_t baseHeight = std::max(1, framebufferHeight >> baseShift);

    if (!m_backdropRT || m_levels.size() < kLevelCount) {
        m_backdropRT = std::make_unique<OffscreenRenderTarget>();
        m_backdropRT->create(baseWidth, baseHeight);
        m_levels.clear();
        m_levels.reserve(kLevelCount);
        for (uint8_t level = 0; level < kLevelCount; ++level) {
            const int32_t width = std::max(1, framebufferWidth >> (baseShift + level));
            const int32_t height = std::max(1, framebufferHeight >> (baseShift + level));
            ChainLevel& chain = m_levels.emplace_back();
            chain.downsampledRT = std::make_unique<OffscreenRenderTarget>();
            chain.blurredRT = std::make_unique<OffscreenRenderTarget>();
            chain.downsampledRT->create(width, height);
            chain.blurredRT->create(width, height);
        }

        m_downsampleMaterial = Material::create("backdrop_downsample");
        m_downsampleMaterial->setBlendEnabled(false);
        m_kawaseMaterial = Material::create("backdrop_kawase");
        m_kawaseMaterial->setBlendEnabled(false);
        m_compositeMaterial = Material::create("backdrop_composite");
        m_compositeMaterial->setBlendEnabled(false);
        // 模糊面片保持默认管线状态（标准 alpha 混合）
        m_blurQuadMaterial = Material::create("backdrop");

        buildPassQuad();
    } else if (baseWidth != m_backdropRT->getWidth() || baseHeight != m_backdropRT->getHeight()) {
        // 窗口 resize：RT 链重建（尺寸变化经签名比对自动触发全量重渲）
        m_backdropRT->resize(baseWidth, baseHeight);
        for (uint8_t level = 0; level < kLevelCount; ++level) {
            const int32_t width = std::max(1, framebufferWidth >> (baseShift + level));
            const int32_t height = std::max(1, framebufferHeight >> (baseShift + level));
            m_levels[level].downsampledRT->resize(width, height);
            m_levels[level].blurredRT->resize(width, height);
        }
    }
    m_framebufferWidth = framebufferWidth;
    m_framebufferHeight = framebufferHeight;
}

void BackdropBlurManager::releaseChainResources() {
    if (m_backdropRT) {
        m_backdropRT->destroy();
    }
    for (auto& level : m_levels) {
        if (level.downsampledRT) {
            level.downsampledRT->destroy();
        }
        if (level.blurredRT) {
            level.blurredRT->destroy();
        }
    }
    m_backdropRT.reset();
    m_levels.clear();

    for (auto& vbo : m_blurQuadVBOs) {
        if (vbo.isValid()) {
            RENDERINGTHREAD->deleteVBO(vbo);
            vbo = HwVBO{0};
        }
    }
    m_backdropSignature = 0; // 强制下一帧全量重渲
    m_framebufferWidth = m_framebufferHeight = 0;
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

void BackdropBlurManager::drawChainPass(const FrameStateSharedPtr& frameState, OffscreenRenderTarget& dst, HwTexture2D src, Material& material,
                                        float kawaseOffset, int32_t texelWidth, int32_t texelHeight) {
    RENDERINGTHREAD->bindRenderTarget(dst.getRenderTarget());
    RENDERINGTHREAD->useTexture2D(src, 0);

    material.setInt("texture", 0);
    // u_texel 为采样源分辨率的倒数（downsample 用源 RT 尺寸，kawase 用层级尺寸）
    material.setVector("texel", Vector2(1.0f / static_cast<float>(std::max(1, texelWidth)), 1.0f / static_cast<float>(std::max(1, texelHeight))));
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

    // 按采样层级分组（§5.3：每层级一个批次，共 ≤ 3 批；同层级任意数量面片
    // 合并为 1 个 draw call）
    std::array<std::vector<const BlurQuad*>, kLevelCount> quadsByLevel;
    for (const auto& quad : m_quads) {
        const uint8_t level = (quad.level < kLevelCount) ? quad.level : kLevelCount - 1;
        quadsByLevel[level].push_back(&quad);
    }

    for (uint8_t level = 0; level < kLevelCount; ++level) {
        if (quadsByLevel[level].empty()) {
            continue;
        }
        auto vboData = buildQuadVBOData(quadsByLevel[level], frameState);
        if (!m_blurQuadVBOs[level].isValid()) {
            m_blurQuadVBOs[level] = RENDERINGTHREAD->createVBO();
        }
        RENDERINGTHREAD->useTexture2D(m_levels[level].blurredRT->getColorTexture(), 0);
        m_blurQuadMaterial->setInt("texture", 0);
        m_blurQuadMaterial->setMatrix4("projectionView", frameState->camera->getProjectionView());
        m_blurQuadMaterial->apply();
        RENDERINGTHREAD->updateVBO(m_blurQuadMaterial->getShader(), m_blurQuadVBOs[level], vboData);
        RENDERINGTHREAD->drawVBO(m_blurQuadVBOs[level], 1);
        frameState->drawCallCount++;
        statistics.backdropDrawCallCount++;
    }
}

VBODataSharedPtr BackdropBlurManager::buildQuadVBOData(const std::vector<const BlurQuad*>& quads, const FrameStateSharedPtr& frameState) {
    // 世界坐标 → backdrop 采样 UV（提案 §5.1）：UI 世界空间以屏幕中心为原点、
    // Y 向上，与 GL 纹理 V 轴同向，故 v = (worldY + H/2) / H。屏幕外越界的
    // UV 由 RT 纹理的 CLAMP_TO_EDGE 兜底（S2 边缘 clamp 采样）。
    const float halfScreenW = frameState->framebufferWidth * 0.5f;
    const float halfScreenH = frameState->framebufferHeight * 0.5f;
    const float invScreenW = 1.0f / static_cast<float>(frameState->framebufferWidth);
    const float invScreenH = 1.0f / static_cast<float>(frameState->framebufferHeight);

    // 每面片 4 顶点，属性平面布局（与设备端 glVertexAttribPointer stride=0 约定一致）：
    //   Position = 世界空间角点；UV = backdrop 采样 UV；Normal = (objX, objY, rounding)；
    //   Tangent = (displaySize.x, displaySize.y, 0, 0)；Color = tint。
    const uint32_t vertexCount = static_cast<uint32_t>(quads.size()) * 4;
    auto vboData = std::make_shared<VBOData>();
    vboData->vertexCount = vertexCount;
    vboData->indexCount = static_cast<uint32_t>(quads.size()) * 6;
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
    vboData->indices.reserve(quads.size() * 6);

    for (size_t q = 0; q < quads.size(); ++q) {
        const BlurQuad& quad = *quads[q];
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
    return vboData;
}
} // namespace morrow
