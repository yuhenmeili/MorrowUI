///
/// Zero-allocation flat command buffer for the render command stream.
///
/// Encoding layout in m_buffer:
///   [CmdHeader(8B) | payload (padded to 8-byte alignment)]  ...
///
/// Trivial payloads    – push<T>(type)  : inline embed, no destructor needed.
/// No-payload commands – push(type)     : only a header written.
/// Non-trivial payloads– pushNT<T>(type): placement-new inline, destructor
///                                        registered in cleanup chain.
///
/// IMPORTANT: The backing buffer is fixed-size (kDefaultCapacity).  It never
/// reallocates so raw pointers stored in the cleanup chain remain valid.
/// Increase kDefaultCapacity if an assert fires in debug builds.
///

#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <vector>
#include <type_traits>
#include <new>

namespace morrow {

class CommandBuffer {
public:
    // QNX startup frames can batch a large number of resource creation and
    // texture upload commands before the render thread drains the ring slot.
    // 16 MB keeps the buffer fixed-size while leaving enough headroom.
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
        m_size = 0;
    }

    // ---------------------------------------------------------------
    // Iteration helpers
    // ---------------------------------------------------------------
    uint8_t*       mutable_data() noexcept { return m_buffer.data(); }
    const uint8_t* data()   const noexcept { return m_buffer.data(); }
    size_t         size()   const noexcept { return m_size; }
    bool           empty()  const noexcept { return m_size == 0; }

private:
    // ---------------------------------------------------------------
    // Allocate space and write the header.  Returns pointer to payload area.
    // NEVER reallocates – capacity must be sufficient.
    // ---------------------------------------------------------------
    uint8_t* alloc(uint32_t type, size_t payloadSize) {
        const size_t stride = sizeof(CmdHeader) + align8(payloadSize);
        assert(m_size + stride <= m_buffer.size() &&
               "CommandBuffer overflow: increase kDefaultCapacity");
        uint8_t* base = m_buffer.data() + m_size;
        auto* h       = reinterpret_cast<CmdHeader*>(base);
        h->type        = type;
        h->payloadSize = static_cast<uint32_t>(payloadSize);
        m_size        += stride;
        return base + sizeof(CmdHeader);    // pointer to payload area
    }

    struct Cleanup { void* ptr; void (*dtor)(void*); };

    std::vector<uint8_t>  m_buffer;
    size_t                m_size;
    std::vector<Cleanup>  m_cleanups;
};

} // namespace morrow

