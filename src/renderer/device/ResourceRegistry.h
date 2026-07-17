// ResourceRegistry.h
//
// GPU 资源注册表 — 后端（GLRenderDevice）持有，负责：
//   1. 前端预分配 Handle ID（原子操作，非阻塞）
//   2. 渲染线程提交 GPU 对象到 Handle
//   3. 渲染线程销毁 GPU 对象并回收 Handle
//   4. 渲染线程按 Handle 查询后端对象

#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include "ResourceHandle.h"
#include "GlResourceObjects.h"

namespace morrow {
class ResourceRegistry {
public:
    ResourceRegistry() = default;

    ~ResourceRegistry();

    // ═══════════════════════════════════════════════════════════════
    // 前端调用（线程安全，非阻塞，立即返回 Handle）
    //   从原子计数器预分配 ID。真正的 GPU 对象由渲染线程稍后创建。
    // ═══════════════════════════════════════════════════════════════

    HwTexture2D allocateTexture2D();

    HwVBO allocateVBO();

    HwUBO allocateUBO();

    HwSSBO allocateSSBO();

    HwGPUProgram allocateGPUProgram();

    HwRenderTarget allocateRenderTarget();

    // ═══════════════════════════════════════════════════════════════
    // 渲染线程调用 — 提交 GPU 对象（在 execute 阶段）
    // ═══════════════════════════════════════════════════════════════

    void commitTexture2D(HwTexture2D handle, GlTexture2D* obj);

    void commitVBO(HwVBO handle, GlVBO* obj);

    void commitUBO(HwUBO handle, GlUBO* obj);

    void commitSSBO(HwSSBO handle, GlSSBO* obj);

    void commitGPUProgram(HwGPUProgram handle, GlProgram* obj);

    void commitRenderTarget(HwRenderTarget handle, GlRenderTarget* obj);

    // ═══════════════════════════════════════════════════════════════
    // 渲染线程调用 — 销毁 GPU 对象并回收 ID
    // ═══════════════════════════════════════════════════════════════

    void destroyTexture2D(HwTexture2D handle);

    void destroyVBO(HwVBO handle);

    void destroyUBO(HwUBO handle);

    void destroySSBO(HwSSBO handle);

    void destroyGPUProgram(HwGPUProgram handle);

    void destroyRenderTarget(HwRenderTarget handle);

    // ═══════════════════════════════════════════════════════════════
    // 渲染线程调用 — 查询后端对象（仅供内部/渲染线程使用）
    // ═══════════════════════════════════════════════════════════════

    GlTexture2D* getTexture2D(HwTexture2D handle) const;

    GlVBO* getVBO(HwVBO handle) const;

    GlUBO* getUBO(HwUBO handle) const;

    GlSSBO* getSSBO(HwSSBO handle) const;

    GlProgram* getGPUProgram(HwGPUProgram handle) const;

    GlRenderTarget* getRenderTarget(HwRenderTarget handle) const;

private:
    template <typename T>
    static void ensureCapacity(std::vector<T*>& vec, uint32_t id) {
        if (id >= vec.size()) {
            vec.resize(static_cast<size_t>(id) + 1, nullptr);
        }
    }

    // ── 原子 ID 分配器（前端线程安全访问）──
    std::atomic<uint32_t> m_nextTextureId{1};
    std::atomic<uint32_t> m_nextVboId{1};
    std::atomic<uint32_t> m_nextUboId{1};
    std::atomic<uint32_t> m_nextSsboId{1};
    std::atomic<uint32_t> m_nextProgramId{1};
    std::atomic<uint32_t> m_nextRenderTargetId{1};

    // ── 稠密数组，用 handle.id 索引 ──
    std::vector<GlTexture2D*> m_textures;
    std::vector<GlVBO*> m_vbos;
    std::vector<GlUBO*> m_ubos;
    std::vector<GlSSBO*> m_ssbos;
    std::vector<GlProgram*> m_programs;
    std::vector<GlRenderTarget*> m_renderTargets;
};
} // namespace morrow