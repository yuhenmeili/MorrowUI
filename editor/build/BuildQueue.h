#ifndef MORROW_EDITOR_BUILD_QUEUE_H
#define MORROW_EDITOR_BUILD_QUEUE_H

#include <filesystem>
#include <atomic>
#include <string>
#include <vector>

namespace morrow::editor {

enum class BuildTaskKind { Configure, Build, Run };

struct BuildTaskResult {
    BuildTaskKind kind;
    bool success = false;
    int exitCode = -1;
    std::string command;
    std::string stdoutText;
    std::string stderrText;
    std::string output;
    bool cancelled = false;
};

enum class BuildProcessState { Idle, Running, Succeeded, Failed, Cancelled };

class BuildQueue {
public:
    BuildTaskResult configure(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& generator = "Ninja") const;

    BuildTaskResult build(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target) const;

    BuildTaskResult runTarget(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory,
                              const std::vector<std::string>& arguments = {}) const;

    void cancel() const;
    BuildProcessState state() const;

private:
    BuildTaskResult run(BuildTaskKind kind, const std::string& command, const std::filesystem::path& workingDirectory) const;
    mutable std::atomic<bool> m_cancelRequested{false};
    mutable std::atomic<BuildProcessState> m_state{BuildProcessState::Idle};
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_BUILD_QUEUE_H
