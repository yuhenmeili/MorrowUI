//
// Created by 0060328 on 26-3-27.
//
// PresentStage — 多屏镜像上屏阶段（ScreenRenderMode::Mirror）。
//

#ifndef MORROW_RENDERER_PRESENTSTAGE_H_
#define MORROW_RENDERER_PRESENTSTAGE_H_

#include <memory>
#include <vector>

#include "resource/OffscreenRenderTarget.h"
#include "morrow/Material.h"
#include "GpuTypes.h"

namespace morrow {

class PlatformWindow;
using PlatformWindowSharedPtr = std::shared_ptr<PlatformWindow>;

/**
 * 多屏镜像上屏阶段（ScreenRenderMode::Mirror）：
 *
 *   UI 管线只渲染一次到离屏内容 RT（beginContentPass / endContentPass 包裹
 *   主窗口的渲染管线），随后 presentAll 对每个窗口画一个全屏贴图 quad
 *   （等比缩放 + 黑边 letterbox，适配各屏分辨率差异），每屏仅 1 个 draw call。
 *
 * 复用 BackdropBlur 的裁剪空间 pass-quad 模式（buildPassQuad / renderComposite），
 * 不经过 BatchManager，全部经 RENDERINGTHREAD（RenderDeviceProxy）下命令。
 */
class PresentStage {
public:
    PresentStage() = default;

    ~PresentStage();

    PresentStage(const PresentStage&) = delete;
    PresentStage& operator=(const PresentStage&) = delete;

    /// 内容分辨率（主窗口尺寸），beginContentPass 前设置；变化时自动重建 RT
    void setContentSize(int32_t width, int32_t height);

    /// 绑定内容 RT：之后的 clear / 合批渲染全部进入 RT（MSAA 在 unbind 时自动 resolve）
    void beginContentPass();

    /// 解绑内容 RT 并恢复 viewport
    void endContentPass();

    /// 对每个已初始化窗口：makeCurrent → 清屏（黑边）→ 贴图 quad → present
    void presentAll(const std::vector<PlatformWindowSharedPtr>& windows);

    /// 释放 GPU 资源（必须在 GlobalObject::destroy 之前、渲染设备仍存活时调用）
    void releaseResources();

private:
    /// 懒创建 present 材质（Material::create("present") + 关混合）
    void ensurePresentMaterial();

    /// 懒构建裁剪空间 quad（照抄 BackdropBlurManager::buildPassQuad）并上传 VBO
    void ensureQuadUploaded();

    OffscreenRenderTarget m_contentRT;
    bool m_contentRTCreated = false;

    std::shared_ptr<Material> m_presentMaterial;
    HwVBO m_quadVBO{0};
    std::shared_ptr<VBOData> m_quadData;
    bool m_quadUploaded = false;

    int32_t m_contentWidth = 0;
    int32_t m_contentHeight = 0;
};

} // morrow

#endif //MORROW_RENDERER_PRESENTSTAGE_H_
