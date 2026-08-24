#include "FileSystemWatcher.h"

#include <chrono>

namespace morrow::editor {

FileSystemWatcher::~FileSystemWatcher() {
    stop();
}

bool FileSystemWatcher::start(const std::filesystem::path& projectRoot, std::string& error) {
    stop();
    std::error_code filesystemError;
    m_root = std::filesystem::weakly_canonical(projectRoot, filesystemError);
    if (filesystemError || !std::filesystem::is_directory(m_root, filesystemError)) {
        error = "file watcher root is not a directory: " + projectRoot.string();
        return false;
    }
    m_snapshot = captureSnapshot();
    m_pendingChanges.clear();
    m_running = true;
    m_thread = std::thread(&FileSystemWatcher::run, this);
    return true;
}

void FileSystemWatcher::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingChanges.clear();
}

std::vector<FileChange> FileSystemWatcher::poll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileChange> changes;
    changes.swap(m_pendingChanges);
    return changes;
}

bool FileSystemWatcher::isRunning() const {
    return m_running.load();
}

FileSystemWatcher::Snapshot FileSystemWatcher::captureSnapshot() const {
    Snapshot snapshot;
    std::error_code filesystemError;
    if (!std::filesystem::exists(m_root, filesystemError))
        return snapshot;

    for (std::filesystem::recursive_directory_iterator iterator(m_root, std::filesystem::directory_options::skip_permission_denied, filesystemError);
         !filesystemError && iterator != std::filesystem::recursive_directory_iterator(); iterator.increment(filesystemError)) {
        const auto& entry = *iterator;
        const auto relative = std::filesystem::relative(entry.path(), m_root, filesystemError);
        if (filesystemError)
            break;
        const auto name = entry.path().filename().string();
        if (name == ".morrow" || name == ".git" || name == ".idea" || name == ".vscode" || entry.path().extension() == ".import") {
            if (entry.is_directory(filesystemError))
                iterator.disable_recursion_pending();
            continue;
        }

        FileStamp stamp;
        stamp.directory = entry.is_directory(filesystemError);
        stamp.modifiedTime = entry.last_write_time(filesystemError);
        if (!stamp.directory)
            stamp.size = entry.file_size(filesystemError);
        snapshot[relative.generic_string()] = stamp;
    }
    return snapshot;
}

void FileSystemWatcher::run() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (!m_running)
            break;

        const Snapshot current = captureSnapshot();
        std::vector<FileChange> changes;
        for (const auto& [path, stamp] : current) {
            const auto previous = m_snapshot.find(path);
            if (previous == m_snapshot.end()) {
                changes.push_back({path, FileChangeKind::Added});
            } else if (previous->second.modifiedTime != stamp.modifiedTime || previous->second.size != stamp.size || previous->second.directory != stamp.directory) {
                changes.push_back({path, FileChangeKind::Modified});
            }
        }
        for (const auto& [path, stamp] : m_snapshot) {
            (void)stamp;
            if (current.find(path) == current.end())
                changes.push_back({path, FileChangeKind::Removed});
        }
        m_snapshot = current;
        if (!changes.empty()) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingChanges.insert(m_pendingChanges.end(), changes.begin(), changes.end());
        }
    }
}

}  // namespace morrow::editor
