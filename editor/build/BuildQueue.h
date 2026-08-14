#ifndef MORROW_EDITOR_BUILD_QUEUE_H
#define MORROW_EDITOR_BUILD_QUEUE_H

#include <filesystem>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

namespace morrow::editor {

enum class BuildTaskKind { Configure, Build, BuildAndRun, Run };

struct BuildDiagnostic {
    std::filesystem::path file;
    int line = 0;
    int column = 0;
    bool error = false;
    std::string message;
};

struct BuildOutputChunk {
    bool stderrStream = false;
    std::string text;
};

struct BuildTaskResult {
    BuildTaskKind kind;
    bool success = false;
    int exitCode = -1;
    std::string command;
    std::string stdoutText;
    std::string stderrText;
    std::string output;
    bool cancelled = false;
    std::vector<BuildDiagnostic> diagnostics;
};

enum class BuildProcessState { Idle, Running, Succeeded, Failed, Cancelled };

class BuildQueue {
public:
    BuildTaskResult configure(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& generator = "Ninja") const;

    BuildTaskResult build(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target) const;

    BuildTaskResult buildAndRun(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target) const;

    BuildTaskResult runTarget(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory,
                              const std::vector<std::string>& arguments = {}) const;

    void cancel() const;
    BuildProcessState state() const;
    std::vector<BuildOutputChunk> drainOutput() const;

private:
    BuildTaskResult run(BuildTaskKind kind, const std::string& command, const std::filesystem::path& workingDirectory) const;
    void pushOutput(bool stderrStream, const char* data, size_t size) const;
    mutable std::atomic<bool> m_cancelRequested{false};
    mutable std::atomic<BuildProcessState> m_state{BuildProcessState::Idle};
    mutable std::mutex m_outputMutex;
    mutable std::vector<BuildOutputChunk> m_liveOutput;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_BUILD_QUEUE_H
