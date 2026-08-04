#include "ObjectRegistry.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace morrow {
struct DebugObjectState {
    mutable std::mutex mutex;
    DebugObjectRecord record;
};

struct ObjectRegistry::Impl {
    mutable std::mutex mutex;
    std::unordered_map<DebugObjectId, std::weak_ptr<DebugObjectState>> objects;
    std::atomic<DebugObjectId> nextId{1};
    std::atomic<uint64_t> currentFrame{0};
};

namespace {
double monotonicSeconds() {
    using Clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t value = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &value);
#else
    localtime_r(&value, &localTime);
#endif
    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return stream.str();
}

std::string escapeJson(const std::string& value) {
    std::ostringstream stream;
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"':
                stream << "\\\"";
                break;
            case '\\':
                stream << "\\\\";
                break;
            case '\b':
                stream << "\\b";
                break;
            case '\f':
                stream << "\\f";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (ch < 0x20) {
                    stream << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(ch)
                        << std::dec << std::setw(0);
                } else {
                    stream << static_cast<char>(ch);
                }
                break;
        }
    }
    return stream.str();
}

void writeWidgetNode(
    std::ostream& stream,
    DebugObjectId id,
    const std::unordered_map<DebugObjectId, const DebugObjectRecord*>& widgets,
    const std::unordered_map<DebugObjectId, std::vector<DebugObjectId>>& children,
    std::unordered_set<DebugObjectId>& visited,
    int indent) {
    const auto recordIt = widgets.find(id);
    if (recordIt == widgets.end()) {
        stream << "null";
        return;
    }
    if (!visited.insert(id).second) {
        stream << "{\"id\":" << id << ",\"cycle\":true}";
        return;
    }

    const auto* record = recordIt->second;
    const std::string padding(static_cast<size_t>(indent), ' ');
    const std::string childPadding(static_cast<size_t>(indent + 2), ' ');
    stream << "{\n"
        << childPadding << "\"id\":" << record->id << ",\n"
        << childPadding << "\"type\":\"" << escapeJson(record->typeName) << "\",\n"
        << childPadding << "\"name\":\"" << escapeJson(record->name) << "\",\n"
        << childPadding << "\"children\":[";

    const auto childrenIt = children.find(id);
    if (childrenIt != children.end() && !childrenIt->second.empty()) {
        stream << "\n";
        for (size_t index = 0; index < childrenIt->second.size(); ++index) {
            stream << std::string(static_cast<size_t>(indent + 4), ' ');
            writeWidgetNode(stream,
                            childrenIt->second[index],
                            widgets,
                            children,
                            visited,
                            indent + 4);
            if (index + 1 < childrenIt->second.size()) stream << ",";
            stream << "\n";
        }
        stream << childPadding;
    }
    stream << "]\n" << padding << "}";
}
} // namespace

ObjectRegistry& ObjectRegistry::getInstance() {
    // Diagnostic handles can belong to function-local singletons. Keep the
    // registry alive until process exit to avoid static destruction ordering.
    static ObjectRegistry* registry = new ObjectRegistry();
    return *registry;
}

ObjectRegistry::ObjectRegistry()
    : m_impl(std::make_unique<Impl>()) {
}

ObjectRegistry::~ObjectRegistry() = default;

void ObjectRegistry::setCurrentFrame(uint64_t frame) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    getInstance().m_impl->currentFrame.store(frame, std::memory_order_relaxed);
#else
    (void)frame;
#endif
}

std::shared_ptr<DebugObjectState> ObjectRegistry::registerObject(DebugObjectCategory category, const std::string& typeName) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    auto state = std::make_shared<DebugObjectState>();
    state->record.id = m_impl->nextId.fetch_add(1, std::memory_order_relaxed);
    state->record.category = category;
    state->record.typeName = typeName;
    state->record.createdAtFrame =
        m_impl->currentFrame.load(std::memory_order_relaxed);
    state->record.createdAtSeconds = monotonicSeconds();
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->objects[state->record.id] = state;
    }
    return state;
#else
    (void)category;
    (void)typeName;
    return {};
#endif
}

void ObjectRegistry::unregisterObject(DebugObjectId id) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (id == 0 || !m_impl) return;
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->objects.erase(id);
#else
    (void)id;
#endif
}

ObjectSnapshot ObjectRegistry::snapshot(uint64_t frame,
                                        DebugObjectId widgetRootId) const {
    ObjectSnapshot result;
    result.frame = frame;
    result.timestamp = currentTimestamp();
    result.widgetRootId = widgetRootId;

#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_impl) return result;

    std::vector<std::shared_ptr<DebugObjectState>> states;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        states.reserve(m_impl->objects.size());
        for (auto it = m_impl->objects.begin(); it != m_impl->objects.end();) {
            if (auto state = it->second.lock()) {
                states.push_back(std::move(state));
                ++it;
            } else {
                it = m_impl->objects.erase(it);
            }
        }
    }

    result.objects.reserve(states.size());
    for (const auto& state : states) {
        std::lock_guard<std::mutex> lock(state->mutex);
        result.objects.push_back(state->record);
    }
    std::sort(result.objects.begin(),
              result.objects.end(),
              [](const DebugObjectRecord& lhs, const DebugObjectRecord& rhs) {
                  return lhs.id < rhs.id;
              });

    std::unordered_map<DebugObjectId, const DebugObjectRecord*> widgets;
    std::unordered_map<DebugObjectId, std::vector<DebugObjectId>> children;
    for (const auto& object : result.objects) {
        switch (object.category) {
            case DebugObjectCategory::Widget:
                ++result.summary.widgets;
                break;
            case DebugObjectCategory::Component:
                ++result.summary.components;
                break;
            case DebugObjectCategory::Texture:
                ++result.summary.textures;
                break;
            case DebugObjectCategory::Font:
                ++result.summary.fonts;
                break;
            case DebugObjectCategory::Other:
                break;
        }
        if (object.category == DebugObjectCategory::Widget) {
            widgets[object.id] = &object;
            if (object.parentId != 0) {
                children[object.parentId].push_back(object.id);
            }
        }
    }
    for (auto& entry : children) {
        std::sort(entry.second.begin(), entry.second.end());
    }

    std::unordered_set<DebugObjectId> reachable;
    if (widgetRootId != 0 && widgets.find(widgetRootId) != widgets.end()) {
        std::vector<DebugObjectId> stack{widgetRootId};
        while (!stack.empty()) {
            const auto id = stack.back();
            stack.pop_back();
            if (!reachable.insert(id).second) continue;
            const auto it = children.find(id);
            if (it != children.end()) {
                stack.insert(stack.end(), it->second.begin(), it->second.end());
            }
        }
    }
    for (const auto& widget : widgets) {
        if (widget.first != widgetRootId &&
            reachable.find(widget.first) == reachable.end()) {
            result.orphanIds.push_back(widget.first);
        }
    }
    std::sort(result.orphanIds.begin(), result.orphanIds.end());
#endif
    return result;
}

bool ObjectRegistry::writeSnapshot(const std::string& path, uint64_t frame, DebugObjectId widgetRootId) const {
    if (path.empty()) return false;
    const ObjectSnapshot data = snapshot(frame, widgetRootId);

    std::error_code error;
    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path(), error);
        if (error) return false;
    }
    std::ofstream stream(outputPath, std::ios::out | std::ios::trunc);
    if (!stream.is_open()) return false;

    std::unordered_map<DebugObjectId, const DebugObjectRecord*> widgets;
    std::unordered_map<DebugObjectId, std::vector<DebugObjectId>> children;
    for (const auto& object : data.objects) {
        if (object.category != DebugObjectCategory::Widget) continue;
        widgets[object.id] = &object;
        if (object.parentId != 0) children[object.parentId].push_back(object.id);
    }
    for (auto& entry : children) {
        std::sort(entry.second.begin(), entry.second.end());
    }

    stream << "{\n"
        << "  \"frame\":" << data.frame << ",\n"
        << "  \"timestamp\":\"" << escapeJson(data.timestamp) << "\",\n"
        << "  \"summary\":{\n"
        << "    \"widgets\":" << data.summary.widgets << ",\n"
        << "    \"components\":" << data.summary.components << ",\n"
        << "    \"textures\":" << data.summary.textures << ",\n"
        << "    \"fonts\":" << data.summary.fonts << "\n"
        << "  },\n"
        << "  \"objects\":[";

    if (!data.objects.empty()) stream << "\n";
    for (size_t index = 0; index < data.objects.size(); ++index) {
        const auto& object = data.objects[index];
        stream << "    {"
            << "\"id\":" << object.id << ","
            << "\"category\":\"" << debugObjectCategoryName(object.category) << "\","
            << "\"type\":\"" << escapeJson(object.typeName) << "\","
            << "\"name\":\"" << escapeJson(object.name) << "\","
            << "\"parentId\":" << object.parentId << ","
            << "\"ownerId\":" << object.ownerId << ","
            << "\"createdAtFrame\":" << object.createdAtFrame
            << "}";
        if (index + 1 < data.objects.size()) stream << ",";
        stream << "\n";
    }
    stream << "  ],\n"
        << "  \"widgetTree\":";
    std::unordered_set<DebugObjectId> visited;
    if (data.widgetRootId != 0 &&
        widgets.find(data.widgetRootId) != widgets.end()) {
        writeWidgetNode(stream,
                        data.widgetRootId,
                        widgets,
                        children,
                        visited,
                        2);
    } else {
        stream << "{}";
    }
    stream << ",\n"
        << "  \"orphans\":[";
    for (size_t index = 0; index < data.orphanIds.size(); ++index) {
        const auto id = data.orphanIds[index];
        const auto widgetIt = widgets.find(id);
        if (index > 0) stream << ",";
        stream << "{"
            << "\"id\":" << id << ","
            << "\"type\":\""
            << escapeJson(widgetIt != widgets.end() ? widgetIt->second->typeName : "")
            << "\","
            << "\"name\":\""
            << escapeJson(widgetIt != widgets.end() ? widgetIt->second->name : "")
            << "\"}";
    }
    stream << "]\n"
        << "}\n";
    return stream.good();
}

DebugObjectHandle::DebugObjectHandle(DebugObjectCategory category, const std::string& typeName) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    m_state = ObjectRegistry::getInstance().registerObject(category, typeName);
#else
    (void)category;
    (void)typeName;
#endif
}

DebugObjectHandle::~DebugObjectHandle() {
    reset();
}

DebugObjectHandle::DebugObjectHandle(DebugObjectHandle&& other) noexcept
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    : m_state(std::move(other.m_state)) {
#else
    {
        (void)other;
#endif
}

DebugObjectHandle& DebugObjectHandle::operator=(DebugObjectHandle&& other) noexcept {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (this != &other) {
        reset();
        m_state = std::move(other.m_state);
    }
#else
    (void)other;
#endif
    return *this;
}

DebugObjectId DebugObjectHandle::id() const {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_state) return 0;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return m_state->record.id;
#else
    return 0;
#endif
}

void DebugObjectHandle::setTypeName(const std::string& typeName) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_state) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    m_state->record.typeName = typeName;
#else
    (void)typeName;
#endif
}

void DebugObjectHandle::setName(const std::string& name) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_state) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    m_state->record.name = name;
#else
    (void)name;
#endif
}

void DebugObjectHandle::setParentId(DebugObjectId parentId) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_state) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    m_state->record.parentId = parentId;
#else
    (void)parentId;
#endif
}

void DebugObjectHandle::setOwnerId(DebugObjectId ownerId) {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_state) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    m_state->record.ownerId = ownerId;
#else
    (void)ownerId;
#endif
}

void DebugObjectHandle::reset() {
#if MORROW_ENABLE_OBJECT_DIAGNOSTICS
    if (!m_state) return;
    const DebugObjectId objectId = id();
    ObjectRegistry::getInstance().unregisterObject(objectId);
    m_state.reset();
#endif
}

const char* debugObjectCategoryName(DebugObjectCategory category) {
    switch (category) {
        case DebugObjectCategory::Widget:
            return "Widget";
        case DebugObjectCategory::Component:
            return "Component";
        case DebugObjectCategory::Texture:
            return "Texture";
        case DebugObjectCategory::Font:
            return "Font";
        case DebugObjectCategory::Other:
            return "Other";
    }
    return "Other";
}
} // namespace morrow