///
/// Zero-allocation flat command buffer for the render command stream.
///
/// Encoding layout in m_buffer:
///   [CmdHeader(8B) | pad(8B) | payload @ +16（16 字节对齐）]
///
/// Trivial payloads    – push<T>(type)  : inline embed, no destructor needed.
/// No-payload commands – push(type)     : only a header written.
/// Non-trivial payloads– pushNT<T>(type): placement-new inline, destructor
///                                        registered in cleanup chain.
///
/// Payload 一律 16 字节对齐：Release 下编译器会把 payload 的构造 / 拷贝
/// 向量化为 movaps 等 16 字节对齐 SSE 指令，仅 8 字节对齐时（如
/// align8 累积步长落在 8 mod 16）会确定性段错误——ImageDemo 启动期的
/// 字形上传序列曾触发（桌面 GL 多线程模式）。
///
/// IMPORTANT: The backing buffer is fixed-size.  It never reallocates so raw
/// pointers stored in the cleanup chain remain valid.  Overflow behaviour:
///   - Debug builds assert immediately (increase capacity via
///     EngineOptions::deviceOptions.commandBufferCapacity).
///   - Release builds degrade safely: the command is dropped (its payload is
///     constructed/destructed in a dedicated overflow arena, never written
///     past the buffer) and endFrame() reports the drop count.
///

#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <deque>
#include <vector>
#include <type_traits>
#include <new>

namespace morrow {

class CommandBuffer {
public:
    // QNX startup frames can batch a large number of resource creation and
    // texture upload commands before the render thread drains the ring slot.
    // 16 MB keeps the buffer fixed-size while leaving enough headroom.
    // Production builds can shrink this via RenderDeviceOptions to reduce
    // startup allocation and resident memory.
    static constexpr size_t kDefaultCapacity = 16u * 1024u * 1024u;

    // ---------------------------------------------------------------
    // Command header – 8 bytes, naturally aligned.
    // ---------------------------------------------------------------
    struct alignas(8) CmdHeader {
        uint32_t type;
        uint32_t payloadSize;   // raw payload bytes (pre-alignment)
    };
    static_assert(sizeof(CmdHeader) == 8, "CmdHeader must be exactly 8 bytes");

    static constexpr size_t align8(size_t v) noexcept { return (v + 7u) & ~7u; }
    static constexpr size_t align16(size_t v) noexcept { return (v + 15u) & ~15u; }

    /// 记录内 payload 的字节偏移（紧跟 header，上取整到 16 对齐）。
    /// 编码（alloc）与解码（executeFrame）必须使用同一常量。
    static constexpr size_t payloadOffset() noexcept { return 16u; }

    /// 单条命令记录的步长：payloadOffset + align16(payloadSize)。
    static constexpr size_t strideOf(size_t payloadSize) noexcept { return payloadOffset() + align16(payloadSize); }

    // ---------------------------------------------------------------
    // Construction / assignment
    // ---------------------------------------------------------------
    explicit CommandBuffer(size_t capacity = kDefaultCapacity)
        : m_buffer(capacity), m_size(0) {
        m_cleanups.reserve(64);
    }
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) = default;
    CommandBuffer& operator=(CommandBuffer&&) = default;

    // ---------------------------------------------------------------
    // Encode a trivially-destructible payload inline.
    // Returns a pointer to the zero-initialised payload area.
    // ---------------------------------------------------------------
    template<typename T>
    T* push(uint32_t type) {
        static_assert(std::is_trivially_destructible<T>::value,
            "Use pushNT for non-trivially-destructible types (e.g. those with "
            "std::string / std::vector / std::shared_ptr members).");
        static_assert(sizeof(T) <= kOverflowSlotSize,
            "Payload too large for the overflow slot; raise kOverflowSlotSize");
        uint8_t* p = alloc(type, sizeof(T));
        std::memset(p, 0, sizeof(T));
        return reinterpret_cast<T*>(p);
    }

    // Encode a command with no payload.
    void push(uint32_t type) {
        alloc(type, 0);
    }

    // ---------------------------------------------------------------
    // Encode a non-trivially-destructible payload via placement new.
    // The destructor is registered and called on clear().
    // ---------------------------------------------------------------
    template<typename T>
    T* pushNT(uint32_t type) {
        static_assert(sizeof(T) <= kOverflowSlotSize,
            "Payload too large for the overflow slot; raise kOverflowSlotSize");
        uint8_t* p = alloc(type, sizeof(T));
        T* obj = new (p) T();                               // default-construct
        m_cleanups.push_back({ obj, [](void* ptr) {
            static_cast<T*>(ptr)->~T();
        }});
        return obj;
    }

    // ---------------------------------------------------------------
    // Reset for reuse – calls destructors in reverse order then zeroes size.
    // ---------------------------------------------------------------
    void clear() {
        for (auto it = m_cleanups.rbegin(); it != m_cleanups.rend(); ++it)
            it->dtor(it->ptr);
        m_cleanups.clear();
        m_overflowSlots.clear();
        m_size = 0;
        m_overflowed = false;
        m_droppedCommands = 0;
    }

    // ---------------------------------------------------------------
    // Iteration helpers
    // ---------------------------------------------------------------
    uint8_t*       mutable_data() noexcept { return m_buffer.data(); }
    const uint8_t* data()   const noexcept { return m_buffer.data(); }
    size_t         size()   const noexcept { return m_size; }
    bool           empty()  const noexcept { return m_size == 0; }
    size_t         capacity() const noexcept { return m_buffer.size(); }

    /// 本帧是否有命令因容量不足被丢弃（Release 安全降级路径）。
    bool   overflowed() const noexcept { return m_overflowed; }
    size_t droppedCommands() const noexcept { return m_droppedCommands; }

private:
    // Capacity for one dropped command's payload in the overflow path.
    // Every encoded payload type is far below this; enforced by static_assert.
    static constexpr size_t kOverflowSlotSize = 512;
    // Overflowed trivial payloads land here (memset only, no destructor).
    // 与主缓冲一致按 16 对齐：溢出路径同样会 placement-new 构造 payload。
    alignas(16) uint8_t m_scratch[kOverflowSlotSize] = {};

    // ---------------------------------------------------------------
    // Allocate space and write the header.  Returns pointer to payload area
    // (16 字节对齐，见文件头注释).  NEVER reallocates – capacity must be
    // sufficient.  On overflow the command is dropped: trivial payloads use
    // m_scratch, non-trivial payloads get a stable slot in m_overflowSlots
    // so their registered destructors stay valid until clear().
    // ---------------------------------------------------------------
    uint8_t* alloc(uint32_t type, size_t payloadSize) {
        const size_t stride = strideOf(payloadSize);
        if (m_size + stride > m_buffer.size()) {
            assert(false &&
                   "CommandBuffer overflow: increase capacity via "
                   "EngineOptions::deviceOptions.commandBufferCapacity");
            ++m_droppedCommands;
            m_overflowed = true;
            if (payloadSize > kOverflowSlotSize) {
                // static_assert 上限内的 payload 不会走到这里；防御性兜底。
                return m_scratch;
            }
            return payloadSize ? overflowSlot() : m_scratch;
        }
        uint8_t* base = m_buffer.data() + m_size;
        auto* h       = reinterpret_cast<CmdHeader*>(base);
        h->type        = type;
        h->payloadSize = static_cast<uint32_t>(payloadSize);
        m_size        += stride;
        return base + payloadOffset();    // pointer to payload area
    }

    // Stable storage for one dropped non-trivial payload.  deque 元素地址
    // 在后续插入时保持稳定，cleanup 链中的指针直到 clear() 前都有效；
    // 16 对齐同 m_scratch。
    struct alignas(16) OverflowSlot { std::array<uint8_t, kOverflowSlotSize> bytes; };
    uint8_t* overflowSlot() {
        m_overflowSlots.emplace_back();
        return m_overflowSlots.back().bytes.data();
    }

    struct Cleanup { void* ptr; void (*dtor)(void*); };

    std::vector<uint8_t>  m_buffer;
    size_t                m_size;
    std::vector<Cleanup>  m_cleanups;
    std::deque<OverflowSlot> m_overflowSlots;
    bool                  m_overflowed = false;
    size_t                m_droppedCommands = 0;
};

} // namespace morrow
