#ifndef MORROW_EDITOR_THUMBNAIL_SERVICE_H
#define MORROW_EDITOR_THUMBNAIL_SERVICE_H

#include <filesystem>
#include <future>
#include <map>
#include <memory>

namespace morrow {
class Texture;
using TextureSharedPtr = std::shared_ptr<Texture>;
} // namespace morrow

namespace morrow::editor {
enum class ThumbnailState {
    Missing,
    Pending,
    Ready,
    Unsupported,
    Failed,
};

struct ThumbnailResult {
    ThumbnailState state = ThumbnailState::Missing;
    morrow::TextureSharedPtr texture;
};

class ThumbnailService {
public:
    explicit ThumbnailService(size_t capacity = 128);

    ThumbnailResult request(const std::filesystem::path& path);

    size_t poll();

    void invalidate(const std::filesystem::path& path);

    void clear();

    size_t cacheSize() const;

    static bool supports(const std::filesystem::path& path);

private:
    struct ProbeResult {
        std::filesystem::path path;
        bool exists = false;
        std::filesystem::file_time_type modifiedTime{};
    };

    struct Entry {
        ThumbnailState state = ThumbnailState::Missing;
        morrow::TextureSharedPtr texture;
        std::filesystem::file_time_type modifiedTime{};
        std::future<ProbeResult> probe;
        uint64_t access = 0;
    };

    void trim();

    size_t m_capacity = 128;
    uint64_t m_accessCounter = 0;
    std::map<std::filesystem::path, Entry> m_entries;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_THUMBNAIL_SERVICE_H