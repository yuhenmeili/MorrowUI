//
// BackdropBlurManager — 共享 Kawase 背景模糊链的帧编排器
//（KAWASE_BACKDROP_BLUR_PROPOSAL.md §5.1 / §6 S1）。
//
// S1 范围：单层级（1/2 分辨率 L0'，固定半径档 = downsample + 2 次 kawase）、
// 全屏链、无内容缓存。帧结构由 BatchManager::renderBatches 驱动：
//
//   阶段 3a（beginBackdropPass 返回 true 时）
//     bindRenderTarget(RT_backdrop) + clear
//     BatchManager 绘制 underlay 批次 + displayLayer < boundary 的批次
//   阶段 3b + 3c（renderBackdropChain）
//     unbind → downsample → kawase ×2（ping-pong）→ 回屏合成拷贝
//     → 全部模糊面片合并为 1 个 draw（采样 L0'）
//   BatchManager 继续绘制 displayLayer ≥ boundary 的批次（面板内容等）
//
// 模糊面片不走 BatchManager 的通用收集通道（相邻合批无法把分散在各面板
// 之前的面片合成一批），而是由 BackdropBlur 组件每帧 submitQuad 提交、
// 这里在 CPU 侧合并成一个自管 VBO 一次绘制 —— 全部模糊面片恒为 1 个
// draw call，且不依赖 SSBO 能力（无非 SSBO 回退分叉）。
//
// 全局开关 setEnabled(false)：模糊面片降级为 tint 纯色面板（由组件走普通
// 通道注册），本管理器不再分段帧结构，模糊子系统零成本（§5.7）。
//

#ifndef MORROW_UI_EFFECTS_BACKDROPBLURMANAGER_H
#define MORROW_UI_EFFECTS_BACKDROPBLURMANAGER_H

#include <memory>
#include <vector>

#include "Material.h"
#include "Matrix4.h"
#include "OffscreenRenderTarget.h"
#include "Vector2.h"
#include "Vector4.h"
#include "utils/Singleton.h"

namespace morrow {
struct FrameState;
using FrameStateSharedPtr = std::shared_ptr<FrameState>;

class BackdropBlurManager : public Singleton<BackdropBlurManager> {
    friend class Singleton<BackdropBlurManager>;

public:
    /// 全局运行时开关（§5.7）：关闭后模糊面片降级为 tint 面板、帧结构退化为
    /// 现有单段路径；组件不销毁，重开即恢复。翻转只改标志位，下一帧生效。
    void setEnabled(bool enabled);

    bool isEnabled() const;

    /// BackdropBlur 组件在 update 阶段提交一个模糊面片（世界矩阵与尺寸取自
    /// 属主 Transform；displayLayer 取自属主 Widget，用于推导分段边界）。
    void submitQuad(const Matrix4& worldMatrix, const Vector2& size, float rounding, const Vector4& tint, int32_t displayLayer);

    /// 本帧是否存在活跃模糊（enabled 且已提交面片）。
    bool isFrameActive() const;

    /// 分段边界：本帧全部模糊面片属主的最小 displayLayer。严格小于边界的
    /// 批次画进 backdrop 段，其余画在回屏合成之后。
    [[nodiscard]] int32_t getBoundaryLayer() const;

    // ---- 以下三个方法由 BatchManager::renderBatches 按序调用 ----

    /// 阶段 3a 开始：本帧有活跃模糊时绑定 backdrop RT 并按窗口清屏色清屏。
    /// 返回 false 表示本帧不需要分段（调用方走现有单段路径，不触碰任何 GPU 状态）。
    bool beginBackdropPass(const FrameStateSharedPtr& frameState);

    /// 阶段 3b + 3c：模糊链 ping-pong、回屏背景恢复拷贝、模糊面片合并绘制。
    void renderBackdropChain(const FrameStateSharedPtr& frameState);

    /// 帧收尾：清空本场面片提交，重置分段边界。
    void endFrame();

    /// 释放全部 GPU 资源（GlobalObject::destroy 时调用，渲染线程仍存活）。
    /// 静态单例析构时渲染设备已销毁，故析构函数不再触碰 GPU 资源。
    void destroy();

private:
    BackdropBlurManager() = default;

    ~BackdropBlurManager() = default;

    struct BlurQuad {
        Matrix4 worldMatrix;
        Vector2 size;
        float rounding = 0.0f;
        Vector4 tint;
        int32_t displayLayer = 0;
    };

    /// 懒创建 / 随窗口尺寸重建 RT 链与 pass 材质（约 3×2MB @1080p 1/2 分辨率）。
    void ensureResources(int32_t framebufferWidth, int32_t framebufferHeight);

    /// 构建链 pass 共用的全屏裁剪空间面片（-1..1，Position + UV 平面布局）。
    void buildPassQuad();

    /// 执行一次全屏 pass：绑定 dst RT，采样 src 纹理绘制。
    void drawChainPass(const FrameStateSharedPtr& frameState, OffscreenRenderTarget& dst, HwTexture2D src,
                       Material& material, float kawaseOffset);

    /// 回屏背景恢复：backdrop RT → 默认帧缓冲的全屏拷贝（blend 关闭）。
    void renderComposite(const FrameStateSharedPtr& frameState);

    /// 全部模糊面片在 CPU 侧烘焙进一个 VBO，一次 draw 采样 L0'。
    void renderBlurQuads(const FrameStateSharedPtr& frameState);

    static constexpr int32_t kNoBoundary = 11; // displayLayer 域为 [-10,10]，11 = 本帧尚无提交

    bool m_enabled = true;
    std::vector<BlurQuad> m_quads;
    int32_t m_boundaryLayer = kNoBoundary;

    int32_t m_rtWidth = 0;
    int32_t m_rtHeight = 0;
    std::unique_ptr<OffscreenRenderTarget> m_backdropRT;  // backdrop 段渲染目标（1/2 分辨率）
    std::unique_ptr<OffscreenRenderTarget> m_chainPingRT; // 链 ping-pong A；链结束后即 L0' 采样层
    std::unique_ptr<OffscreenRenderTarget> m_chainPongRT; // 链 ping-pong B

    MaterialSharedPtr m_downsampleMaterial;
    MaterialSharedPtr m_kawaseMaterial;
    MaterialSharedPtr m_compositeMaterial;
    MaterialSharedPtr m_blurQuadMaterial;

    HwVBO m_passQuadVBO{0};
    VBODataSharedPtr m_passQuadData;
    bool m_passQuadUploaded = false;
    HwVBO m_blurQuadVBO{0};
};
} // namespace morrow

#endif // MORROW_UI_EFFECTS_BACKDROPBLURMANAGER_H
