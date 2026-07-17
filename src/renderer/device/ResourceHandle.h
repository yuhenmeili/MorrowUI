// ResourceHandle.h
//
// 通用整数 GPU 资源句柄，类似 filament::Handle<HwTexture>。
// 前端对象只持有 Handle（整数 ID），后端 ResourceRegistry 维护实际 GPU 资源。
//
// 0 表示无效 / null handle。
//
// 预留 generation 字段 — 初期固定为 1，后续可启用 ABA 防护：
//   打开 MORROW_HANDLE_GENERATION_CHECK 宏后，ResourceRegistry 在
//   每次 ID 复用时递增 generation，Handle 访问时校验 generation 匹配。

#pragma once

#include <cstdint>
#include <functional>

namespace morrow {
#ifndef MORROW_HANDLE_GENERATION_CHECK
#define MORROW_HANDLE_GENERATION_CHECK 0
#endif

template <typename Tag>
struct ResourceHandle {
    uint32_t id = 0;

#if MORROW_HANDLE_GENERATION_CHECK
    uint16_t generation = 1;
#endif

    bool isValid() const { return id != 0; }

    explicit operator bool() const { return id != 0; }

    bool operator==(ResourceHandle other) const { return id == other.id; }

    bool operator!=(ResourceHandle other) const { return id != other.id; }

    struct Hash {
        size_t operator()(ResourceHandle h) const { return std::hash<uint32_t>{}(h.id); }
    };
};

// ── 各资源类型的 Handle 别名 ──

using HwTexture2D = ResourceHandle<struct Texture2DTag>;
using HwVBO = ResourceHandle<struct VBOTag>;
using HwUBO = ResourceHandle<struct UBOTag>;
using HwSSBO = ResourceHandle<struct SSBOTag>;
using HwGPUProgram = ResourceHandle<struct GPUProgramTag>;
using HwRenderTarget = ResourceHandle<struct RenderTargetTag>;
} // namespace morrow