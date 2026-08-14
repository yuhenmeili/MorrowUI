#include "BuildQueue.h"

#include <cstdlib>

namespace morrow::editor {

BuildTaskResult BuildQueue::run(BuildTaskKind kind, const std::string& command, const std::filesystem::path& workingDirectory) const {
    BuildTaskResult result{kind, false, -1, command, {}};
    const auto previous = std::filesystem::current_path();
    std::error_code filesystemError;
    std::filesystem::current_path(workingDirectory, filesystemError);
    if (filesystemError) {
        result.output = filesystemError.message();
        return result;
    }
    result.exitCode = std::system(command.c_str());
    std::filesystem::current_path(previous, filesystemError);
    result.success = result.exitCode == 0;
    return result;
}

BuildTaskResult BuildQueue::configure(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& generator) const {
    const auto command = "cmake -S \"" + projectRoot.string() + "\" -B \"" + buildRoot.string() + "\" -G \"" + generator + "\"";
    return run(BuildTaskKind::Configure, command, projectRoot);
}

BuildTaskResult BuildQueue::build(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target) const {
    const auto command = "cmake --build \"" + buildRoot.string() + "\" --target \"" + target + "\"";
    return run(BuildTaskKind::Build, command, projectRoot);
}

}  // namespace morrow::editor
