//
// Created by 0060328 on 26-3-27.
//

#include "PresentStage.h"

#include <algorithm>

#include "GlobalObject.h"
#include "PlatformWindow.h"
#include "morrow/utils/Log.h"

namespace morrow {
namespace {
/// 等比缩放 + 居中：返回内容区在窗口像素空间（原点左下、Y 向上，与窗口
/// 相机约定一致）中的矩形 {offX, offY, scaledW, scaledH}。
/// 由于内容区总是居中的，Y 方向约定不影响结果。
Vector4 letterboxRect(int32_t winWidth, int32_t winHeight, int32_t contentWidth, int32_t contentHeight) {
    if (winWidth <= 0 || winHeight <= 0 || contentWidth <= 0 || contentHeight <= 0) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    const float scale = std::min(static_cast<float>(winWidth) / contentWidth,
                                 static_cast<float>(winHeight) / contentHeight);
    const float scaledW = contentWidth * scale;
    const float scaledH = contentHeight * scale;
    const float offX = (winWidth - scaledW) * 0.5f;
    const float offY = (winHeight - scaledH) * 0.5f;
    return {offX, offY, scaledW, scaledH};
}
} // namespace

PresentStage::~PresentStage() {
    releaseResources();
}

void PresentStage::setContentSize(int32_t width, int32_t height) {
    m_contentWidth = width;
    m_contentHeight = height;
}

void PresentStage::beginContentPass() {
    if (m_contentWidth <= 0 || m_contentHeight <= 0) {
        return;
    }
    if (!m_contentRTCreated) {
        m_contentRT.create(m_contentWidth, m_contentHeight);
        m_contentRTCreated = true;
    } else {
        m_contentRT.resize(m_contentWidth, m_contentHeight);
    }
    RENDERINGTHREAD->bindRenderTarget(m_contentRT.getRenderTarget());
}

void PresentStage::endContentPass() {
    if (!m_contentRTCreated) {
        return;
    }
    // unbind 内含 MSAA resolve（MSAA 时 RT 内容 blit 到颜色纹理），
    // 但 viewport 只按单槽恢复，回窗口前必须显式重设（见 presentAll）
    RENDERINGTHREAD->unbindRenderTarget();
}

void PresentStage::presentAll(const std::vector<PlatformWindowSharedPtr>& windows) {
    if (!m_contentRTCreated || !m_contentRT.getRenderTarget().isValid()) {
        return;
    }
    ensurePresentMaterial();
    const int32_t contentWidth = m_contentRT.getWidth();
    const int32_t contentHeight = m_contentRT.getHeight();

    for (const auto& window : windows) {
        if (!window || !window->isWindowInited() || !window->isScreenVisible()) {
            continue;
        }
        void* surface = window->nativeSurface();
        if (!surface) {
            continue;
        }
        const Vector2 winSize = window->framebufferSize();
        if (winSize.x <= 0 || winSize.y <= 0) {
            continue;
        }

        RENDERINGTHREAD->makeCurrent(surface);
        // 帧起点关 scissor：黑边清屏不受残留裁剪影响（同 EGLWindow::beginRenderPass）
        RENDERINGTHREAD->setScissorRect(false, 0, 0, 0, 0);
        RENDERINGTHREAD->setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        RENDERINGTHREAD->setViewPort(0, 0, int32_t(winSize.x), int32_t(winSize.y));
        RENDERINGTHREAD->clear();

        const Vector4 rect = letterboxRect(int32_t(winSize.x), int32_t(winSize.y), contentWidth, contentHeight);
        if (rect.z > 0 && rect.w > 0) {
            RENDERINGTHREAD->useTexture2D(m_contentRT.getColorTexture(), 0);
            m_presentMaterial->setInt("texture", 0);
            // a_position(-1..1) * u_scale + u_offset → 内容区 NDC 矩形
            m_presentMaterial->setVector("scale",
                Vector2(rect.z / winSize.x, rect.w / winSize.y));
            m_presentMaterial->setVector("offset",
                Vector2((rect.x + rect.z * 0.5f) / winSize.x * 2.0f - 1.0f,
                        (rect.y + rect.w * 0.5f) / winSize.y * 2.0f - 1.0f));
            m_presentMaterial->apply(); // 构建 shader + 绑定管线 + 提交 uniform
            ensureQuadUploaded();       // updateVBO 依赖已构建的 shader 定位属性
            RENDERINGTHREAD->drawVBO(m_quadVBO, 1);
        }
        RENDERINGTHREAD->present(surface);
    }
}

void PresentStage::releaseResources() {
    if (m_contentRTCreated) {
        m_contentRT.destroy();
        m_contentRTCreated = false;
    }
    if (m_quadVBO.isValid()) {
        RENDERINGTHREAD->deleteVBO(m_quadVBO);
        m_quadVBO = HwVBO{0};
    }
    m_quadUploaded = false;
    m_quadData.reset();
    m_presentMaterial.reset();
}

void PresentStage::ensurePresentMaterial() {
    if (!m_presentMaterial) {
        m_presentMaterial = Material::create("present");
        m_presentMaterial->setBlendEnabled(false);
    }
}

void PresentStage::ensureQuadUploaded() {
    if (!m_quadData) {
        // 裁剪空间全屏四边形，顶点顺序与 UV 朝向沿用
        // BackdropBlurManager::buildPassQuad 的约定：
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
        m_quadData = std::move(vboData);
    }
    if (!m_quadVBO.isValid()) {
        m_quadVBO = RENDERINGTHREAD->createVBO();
    }
    if (!m_quadUploaded) {
        RENDERINGTHREAD->updateVBO(m_presentMaterial->getShader(), m_quadVBO, m_quadData);
        m_quadUploaded = true;
    }
}
} // morrow
