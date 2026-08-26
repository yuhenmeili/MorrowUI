#ifndef MORROW_EDITOR_IMPORT_QUEUE_H
#define MORROW_EDITOR_IMPORT_QUEUE_H

#include <filesystem>
#include <string>
#include <vector>

#include "AssetDatabase.h"

namespace morrow::editor {
struct ImportTaskResult {
    std::string assetId;
    bool success = false;
    std::filesystem::path artifact;
    bool skipped = false;
    int attempts = 0;
    std::string error;
};

class ImportQueue {
public:
    bool importAll(const AssetDatabase& database, const std::filesystem::path& projectRoot, const std::string& platform, std::vector<ImportTaskResult>& results,
                   std::string& error) const;

    bool importOne(const AssetRecord& asset, const std::filesystem::path& projectRoot, const std::string& platform, ImportTaskResult& result, std::string& error) const;

    bool retry(const AssetRecord& asset, const std::filesystem::path& projectRoot, const std::string& platform, ImportTaskResult& result, std::string& error,
               int maxAttempts = 2) const;
};
} // namespace morrow::editor

#endif  // MORROW_EDITOR_IMPORT_QUEUE_H