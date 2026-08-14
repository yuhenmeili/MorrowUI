#include "ImportQueue.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::string hashFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
        return {};
    uint64_t hash = 1469598103934665603ull;
    char buffer[8192];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
        for (std::streamsize i = 0; i < input.gcount(); ++i) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= 1099511628211ull;
        }
    }
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << hash;
    return output.str();
}

void updateSourceHash(const std::filesystem::path& importPath, const std::string& hash) {
    std::ifstream input(importPath);
    if (!input.is_open())
        return;
    std::ostringstream contents;
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("source_hash", 0) == 0)
            contents << "source_hash = \"" << hash << "\"\n";
        else
            contents << line << '\n';
    }
    std::ofstream output(importPath, std::ios::trunc);
    if (output.is_open())
        output << contents.str();
}
}  // namespace

namespace morrow::editor {

bool ImportQueue::importOne(const AssetRecord& asset, const std::filesystem::path& projectRoot, const std::string& platform, ImportTaskResult& result, std::string& error) const {
    result.assetId = asset.assetId;
    result.attempts++;
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
    if (!asset.needsImport && std::filesystem::exists(artifact)) {
        result.success = true;
        result.skipped = true;
        result.artifact = artifact;
        return true;
    }
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
    updateSourceHash(projectRoot / asset.importPath, hashFile(source));
    return true;
}

bool ImportQueue::retry(const AssetRecord& asset, const std::filesystem::path& projectRoot, const std::string& platform, ImportTaskResult& result,
                        std::string& error, int maxAttempts) const {
    result = {};
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        if (importOne(asset, projectRoot, platform, result, error))
            return true;
    }
    return false;
}

bool ImportQueue::importAll(const AssetDatabase& database, const std::filesystem::path& projectRoot, const std::string& platform, std::vector<ImportTaskResult>& results,
                            std::string& error) const {
    results.clear();
    bool success = true;
    for (const auto& asset : database.assets()) {
        ImportTaskResult result;
        if (!retry(asset, projectRoot, platform, result, error)) {
            success = false;
        }
        results.push_back(std::move(result));
    }
    return success;
}

}  // namespace morrow::editor
