// ResourceRegistry.cpp
//
// GPU 资源注册表实现

#include "ResourceRegistry.h"

namespace morrow {

ResourceRegistry::~ResourceRegistry() {
    // 清理所有未销毁的资源（防御性，正常流程应在 shutdown 前全部销毁）
    for (auto* obj : m_textures)       { delete obj; }
    for (auto* obj : m_vbos)           { delete obj; }
    for (auto* obj : m_ubos)           { delete obj; }
    for (auto* obj : m_ssbos)          { delete obj; }
    for (auto* obj : m_programs)       { delete obj; }
    for (auto* obj : m_renderTargets)  { delete obj; }
}

// ─────────────────────────────────────────────────────────────────
// 前端分配（线程安全，非阻塞）
// ─────────────────────────────────────────────────────────────────

HwTexture2D ResourceRegistry::allocateTexture2D() {
    return HwTexture2D{ m_nextTextureId.fetch_add(1, std::memory_order_relaxed) };
}

HwVBO ResourceRegistry::allocateVBO() {
    return HwVBO{ m_nextVboId.fetch_add(1, std::memory_order_relaxed) };
}

HwUBO ResourceRegistry::allocateUBO() {
    return HwUBO{ m_nextUboId.fetch_add(1, std::memory_order_relaxed) };
}

HwSSBO ResourceRegistry::allocateSSBO() {
    return HwSSBO{ m_nextSsboId.fetch_add(1, std::memory_order_relaxed) };
}

HwGPUProgram ResourceRegistry::allocateGPUProgram() {
    return HwGPUProgram{ m_nextProgramId.fetch_add(1, std::memory_order_relaxed) };
}

HwRenderTarget ResourceRegistry::allocateRenderTarget() {
    return HwRenderTarget{ m_nextRenderTargetId.fetch_add(1, std::memory_order_relaxed) };
}

// ─────────────────────────────────────────────────────────────────
// 渲染线程提交
// ─────────────────────────────────────────────────────────────────

void ResourceRegistry::commitTexture2D(HwTexture2D handle, GlTexture2D* obj) {
    ensureCapacity(m_textures, handle.id);
    m_textures[handle.id] = obj;
}

void ResourceRegistry::commitVBO(HwVBO handle, GlVBO* obj) {
    ensureCapacity(m_vbos, handle.id);
    m_vbos[handle.id] = obj;
}

void ResourceRegistry::commitUBO(HwUBO handle, GlUBO* obj) {
    ensureCapacity(m_ubos, handle.id);
    m_ubos[handle.id] = obj;
}

void ResourceRegistry::commitSSBO(HwSSBO handle, GlSSBO* obj) {
    ensureCapacity(m_ssbos, handle.id);
    m_ssbos[handle.id] = obj;
}

void ResourceRegistry::commitGPUProgram(HwGPUProgram handle, GlProgram* obj) {
    ensureCapacity(m_programs, handle.id);
    m_programs[handle.id] = obj;
}

void ResourceRegistry::commitRenderTarget(HwRenderTarget handle, GlRenderTarget* obj) {
    ensureCapacity(m_renderTargets, handle.id);
    m_renderTargets[handle.id] = obj;
}

// ─────────────────────────────────────────────────────────────────
// 渲染线程销毁
// ─────────────────────────────────────────────────────────────────

void ResourceRegistry::destroyTexture2D(HwTexture2D handle) {
    if (handle.id == 0 || handle.id >= m_textures.size()) return;
    delete m_textures[handle.id];
    m_textures[handle.id] = nullptr;
}

void ResourceRegistry::destroyVBO(HwVBO handle) {
    if (handle.id == 0 || handle.id >= m_vbos.size()) return;
    delete m_vbos[handle.id];
    m_vbos[handle.id] = nullptr;
}

void ResourceRegistry::destroyUBO(HwUBO handle) {
    if (handle.id == 0 || handle.id >= m_ubos.size()) return;
    delete m_ubos[handle.id];
    m_ubos[handle.id] = nullptr;
}

void ResourceRegistry::destroySSBO(HwSSBO handle) {
    if (handle.id == 0 || handle.id >= m_ssbos.size()) return;
    delete m_ssbos[handle.id];
    m_ssbos[handle.id] = nullptr;
}

void ResourceRegistry::destroyGPUProgram(HwGPUProgram handle) {
    if (handle.id == 0 || handle.id >= m_programs.size()) return;
    delete m_programs[handle.id];
    m_programs[handle.id] = nullptr;
}

void ResourceRegistry::destroyRenderTarget(HwRenderTarget handle) {
    if (handle.id == 0 || handle.id >= m_renderTargets.size()) return;
    delete m_renderTargets[handle.id];
    m_renderTargets[handle.id] = nullptr;
}

// ─────────────────────────────────────────────────────────────────
// 渲染线程查询
// ─────────────────────────────────────────────────────────────────

GlTexture2D* ResourceRegistry::getTexture2D(HwTexture2D handle) const {
    if (handle.id == 0 || handle.id >= m_textures.size()) return nullptr;
    return m_textures[handle.id];
}

GlVBO* ResourceRegistry::getVBO(HwVBO handle) const {
    if (handle.id == 0 || handle.id >= m_vbos.size()) return nullptr;
    return m_vbos[handle.id];
}

GlUBO* ResourceRegistry::getUBO(HwUBO handle) const {
    if (handle.id == 0 || handle.id >= m_ubos.size()) return nullptr;
    return m_ubos[handle.id];
}

GlSSBO* ResourceRegistry::getSSBO(HwSSBO handle) const {
    if (handle.id == 0 || handle.id >= m_ssbos.size()) return nullptr;
    return m_ssbos[handle.id];
}

GlProgram* ResourceRegistry::getGPUProgram(HwGPUProgram handle) const {
    if (handle.id == 0 || handle.id >= m_programs.size()) return nullptr;
    return m_programs[handle.id];
}

GlRenderTarget* ResourceRegistry::getRenderTarget(HwRenderTarget handle) const {
    if (handle.id == 0 || handle.id >= m_renderTargets.size()) return nullptr;
    return m_renderTargets[handle.id];
}

} // namespace morrow
