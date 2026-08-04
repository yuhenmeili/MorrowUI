#ifndef MORROW_DEBUG_OBJECTREGISTRY_H
#define MORROW_DEBUG_OBJECTREGISTRY_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

#ifndef MORROW_ENABLE_OBJECT_DIAGNOSTICS
#define MORROW_ENABLE_OBJECT_DIAGNOSTICS 0
#endif

namespace morrow {
using DebugObjectId = uint64_t;

enum class DebugObjectCategory : uint8_t {
    Widget,
    Component,
    Texture,
    Font,
    Other
};

struct DebugObjectRecord {
    DebugObjectId id = 0;
    DebugObjectCategory category = DebugObjectCategory::Other;
    std::string typeName;
    std::string name;
    DebugObjectId parentId = 0;
    DebugObjectId ownerId = 0;
    uint64_t createdAtFrame = 0;
    double createdAtSeconds = 0.0;
};

struct ObjectSnapshotSummary {
    size_t widgets = 0;
    size_t components = 0;
    size_t textures = 0;
    size_t fonts = 0;
};

struct ObjectSnapshot {
    uint64_t frame = 0;
    std::string timestamp;
    ObjectSnapshotSummary summary;
    std::vector<DebugObjectRecord> objects;
    std::vector<DebugObjectId> orphanIds;
    DebugObjectId widgetRootId = 0;
};

class DebugObjectHandle;
struct DebugObjectState;

class ObjectRegistry {
public:
    static ObjectRegistry& getInstance();

    void setCurrentFrame(uint64_t frame);

    ObjectSnapshot snapshot(uint64_t frame, DebugObjectId widgetRootId = 0) const;

    bool writeSnapshot(const std::string& path, uint64_t frame, DebugObjectId widgetRootId = 0) const;

private:
    friend class DebugObjectHandle;

    ObjectRegistry();

    ~ObjectRegistry();

    std::shared_ptr<DebugObjectState> registerObject(DebugObjectCategory category, const std::string& typeName);

    void unregisterObject(DebugObjectId id);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/// Non-owning diagnostic registration token. The registry stores only a
/// weak reference to its state, so diagnostics never extend object lifetime.
class DebugObjectHandle {
public:
    DebugObjectHandle() = default;

    DebugObjectHandle(DebugObjectCategory category, const std::string& typeName);

    ~DebugObjectHandle();

    DebugObjectHandle(const DebugObjectHandle&) = delete;

    DebugObjectHandle& operator=(const DebugObjectHandle&) = delete;

    DebugObjectHandle(DebugObjectHandle&& other) noexcept;

    DebugObjectHandle& operator=(DebugObjectHandle&& other) noexcept;

    DebugObjectId id() const;

    void setTypeName(const std::string& typeName);

    void setName(const std::string& name);

    void setParentId(DebugObjectId parentId);

    void setOwnerId(DebugObjectId ownerId);

private:
    void reset();

#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    std::shared_ptr<DebugObjectState> m_state;
#endif
};

const char* debugObjectCategoryName(DebugObjectCategory category);

template <typename T>
std::string debugTypeName() {
#if defined(_MSC_VER)
    std::string_view signature = __FUNCSIG__;
    const std::string_view prefix = "debugTypeName<";
    const size_t begin = signature.find(prefix);
    if (begin == std::string_view::npos) return typeid(T).name();
    const size_t typeBegin = begin + prefix.size();
    const size_t end = signature.find(">(void)", typeBegin);
    if (end == std::string_view::npos) return typeid(T).name();
    return std::string(signature.substr(typeBegin, end - typeBegin));
#elif defined(__clang__) || defined(__GNUC__)
    std::string_view signature = __PRETTY_FUNCTION__;
    const std::string_view prefix = "T = ";
    const size_t begin = signature.find(prefix);
    if (begin == std::string_view::npos) return typeid(T).name();
    const size_t typeBegin = begin + prefix.size();
    size_t end = signature.find(';', typeBegin);
    if (end == std::string_view::npos) end = signature.find(']', typeBegin);
    if (end == std::string_view::npos) return typeid(T).name();
    std::string name(signature.substr(typeBegin, end - typeBegin));
    const std::string namespacePrefix = "morrow::";
    if (name.compare(0, namespacePrefix.size(), namespacePrefix) == 0) {
        name.erase(0, namespacePrefix.size());
    }
    return name;
#else
    return typeid(T).name();
#endif
}
} // namespace morrow

#endif // MORROW_DEBUG_OBJECTREGISTRY_H