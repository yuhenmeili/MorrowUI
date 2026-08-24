#ifndef MORROW_EDITOR_FILE_SYSTEM_WATCHER_H
#define MORROW_EDITOR_FILE_SYSTEM_WATCHER_H

#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace morrow::editor {

enum class FileChangeKind {
    Added,
    Removed,
    Modified,
};

struct FileChange {
    std::filesystem::path relativePath;
    FileChangeKind kind = FileChangeKind::Modified;
};

class FileSystemWatcher {
public:
    FileSystemWatcher() = default;
    ~FileSystemWatcher();

    FileSystemWatcher(const FileSystemWatcher&) = delete;
    FileSystemWatcher& operator=(const FileSystemWatcher&) = delete;

    bool start(const std::filesystem::path& projectRoot, std::string& error);

    void stop();

    std::vector<FileChange> poll();

    bool isRunning() const;

private:
    struct FileStamp {
        std::filesystem::file_time_type modifiedTime{};
        uintmax_t size = 0;
        bool directory = false;
    };

    using Snapshot = std::map<std::string, FileStamp>;

    Snapshot captureSnapshot() const;
    void run();

    std::filesystem::path m_root;
    std::atomic_bool m_running{false};
    std::thread m_thread;
    mutable std::mutex m_mutex;
    std::vector<FileChange> m_pendingChanges;
    Snapshot m_snapshot;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_FILE_SYSTEM_WATCHER_H
