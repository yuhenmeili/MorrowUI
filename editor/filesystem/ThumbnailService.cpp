#include "ThumbnailService.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <set>

#include "renderer/resource/Texture.h"

namespace morrow::editor {

ThumbnailService::ThumbnailService(size_t capacity) : m_capacity(std::max<size_t>(1, capacity)) {
}

ThumbnailResult ThumbnailService::request(const std::filesystem::path& path) {
    const auto normalized = path.lexically_normal();
    if (!supports(normalized))
        return {ThumbnailState::Unsupported, {}};

    auto iterator = m_entries.find(normalized);
    if (iterator == m_entries.end()) {
        Entry entry;
        entry.state = ThumbnailState::Pending;
        entry.access = ++m_accessCounter;
        entry.probe = std::async(std::launch::async, [normalized]() {
            ProbeResult result;
            result.path = normalized;
            std::error_code error;
            result.exists = std::filesystem::is_regular_file(normalized, error);
            if (result.exists)
                result.modifiedTime = std::filesystem::last_write_time(normalized, error);
            return result;
        });
        iterator = m_entries.emplace(normalized, std::move(entry)).first;
        trim();
    } else {
        iterator->second.access = ++m_accessCounter;
    }
    return {iterator->second.state, iterator->second.texture};
}

size_t ThumbnailService::poll() {
    size_t completed = 0;
    for (auto& [path, entry] : m_entries) {
        (void)path;
        if (entry.state != ThumbnailState::Pending || !entry.probe.valid() || entry.probe.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            continue;
        }
        const ProbeResult result = entry.probe.get();
        if (!result.exists) {
            entry.state = ThumbnailState::Failed;
        } else {
            entry.texture = Texture::create();
            entry.texture->setImageUrl(result.path.string());
            entry.texture->setTextureName("editor_thumbnail_" + result.path.filename().string());
            entry.modifiedTime = result.modifiedTime;
            entry.state = ThumbnailState::Ready;
        }
        ++completed;
    }
    return completed;
}

void ThumbnailService::invalidate(const std::filesystem::path& path) {
    m_entries.erase(path.lexically_normal());
}

void ThumbnailService::clear() {
    m_entries.clear();
}

size_t ThumbnailService::cacheSize() const {
    return m_entries.size();
}

bool ThumbnailService::supports(const std::filesystem::path& path) {
    static const std::set<std::string> extensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga"};
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extensions.count(extension) != 0;
}

void ThumbnailService::trim() {
    while (m_entries.size() > m_capacity) {
        auto oldest = m_entries.end();
        for (auto iterator = m_entries.begin(); iterator != m_entries.end(); ++iterator) {
            if (iterator->second.state == ThumbnailState::Pending)
                continue;
            if (oldest == m_entries.end() || iterator->second.access < oldest->second.access) {
                oldest = iterator;
            }
        }
        if (oldest == m_entries.end())
            return;
        m_entries.erase(oldest);
    }
}

}  // namespace morrow::editor
