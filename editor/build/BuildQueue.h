#ifndef MORROW_EDITOR_BUILD_QUEUE_H
#define MORROW_EDITOR_BUILD_QUEUE_H

#include <filesystem>
#include <string>

namespace morrow::editor {

enum class BuildTaskKind { Configure, Build, Run };

struct BuildTaskResult {
    BuildTaskKind kind;
    bool success = false;
    int exitCode = -1;
    std::string command;
    std::string output;
};

class BuildQueue {
public:
    BuildTaskResult configure(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& generator = "Ninja") const;

    BuildTaskResult build(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target) const;

private:
    BuildTaskResult run(BuildTaskKind kind, const std::string& command, const std::filesystem::path& workingDirectory) const;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_BUILD_QUEUE_H
