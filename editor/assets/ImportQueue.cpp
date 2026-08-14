#include "ImportQueue.h"

#include <fstream>

namespace morrow::editor {

bool ImportQueue::importOne(const AssetRecord& asset, const std::filesystem::path& projectRoot, const std::string& platform, ImportTaskResult& result, std::string& error) const {
    result.assetId = asset.assetId;
    if (!asset.imported || asset.assetId.empty()) {
        result.error = asset.error.empty() ? "asset is not importable" : asset.error;
        error = result.error;
        return false;
    }

    const auto source = projectRoot / asset.sourcePath;
    if (!std::filesystem::exists(source)) {
        result.error = "source file does not exist: " + source.string();
        error = result.error;
        return false;
    }

    const auto artifact = projectRoot / ".morrow" / "imported" / asset.assetId / platform / (source.filename().string() + ".artifact");
    std::error_code filesystemError;
    std::filesystem::create_directories(artifact.parent_path(), filesystemError);
    if (filesystemError) {
        result.error = "failed to create artifact directory: " + filesystemError.message();
        error = result.error;
        return false;
    }

    // Phase 1 importer contract: create a deterministic derived artifact.
    // Specialized texture/font/model conversion can replace this copy step
    // without changing AssetDatabase or the queue API.
    std::ifstream input(source, std::ios::binary);
    std::ofstream output(artifact, std::ios::binary | std::ios::trunc);
    if (!input.is_open() || !output.is_open()) {
        result.error = "failed to open import source or artifact";
        error = result.error;
        return false;
    }
    output << input.rdbuf();
    if (!output.good()) {
        result.error = "failed to write artifact: " + artifact.string();
        error = result.error;
        return false;
    }
    result.success = true;
    result.artifact = artifact;
    return true;
}

bool ImportQueue::importAll(const AssetDatabase& database, const std::filesystem::path& projectRoot, const std::string& platform, std::vector<ImportTaskResult>& results,
                            std::string& error) const {
    results.clear();
    bool success = true;
    for (const auto& asset : database.assets()) {
        ImportTaskResult result;
        if (!importOne(asset, projectRoot, platform, result, error)) {
            success = false;
        }
        results.push_back(std::move(result));
    }
    return success;
}

}  // namespace morrow::editor
