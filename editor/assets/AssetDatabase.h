#ifndef MORROW_EDITOR_ASSET_DATABASE_H
#define MORROW_EDITOR_ASSET_DATABASE_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace morrow::editor {

struct ImportMetadata {
    int format = 1;
    std::string assetId;
    std::string importer;
    std::string sourceHash;
    int importerVersion = 1;
    std::map<std::string, std::string> options;
    std::map<std::string, std::string> artifacts;
    size_t line = 0;
};

struct AssetRecord {
    std::string assetId;
    std::string type;
    std::filesystem::path sourcePath;
    std::filesystem::path importPath;
    ImportMetadata import;
    bool imported = false;
    std::string error;
};

class AssetDatabase {
public:
    bool scan(const std::filesystem::path& projectRoot, const std::filesystem::path& assetRoot, std::string& error);

    const AssetRecord* findById(const std::string& assetId) const;

    const AssetRecord* findBySourcePath(const std::filesystem::path& sourcePath) const;

    const std::vector<AssetRecord>& assets() const;

    std::filesystem::path resolveSourcePath(const std::string& assetId) const;

private:
    bool loadImportFile(const std::filesystem::path& importPath, ImportMetadata& metadata, std::string& error) const;

    std::vector<AssetRecord> m_assets;
    std::filesystem::path m_projectRoot;
    std::filesystem::path m_assetRoot;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_ASSET_DATABASE_H
