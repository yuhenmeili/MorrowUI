#ifndef MORROW_EDITOR_PROJECT_FILE_SYSTEM_MODEL_H
#define MORROW_EDITOR_PROJECT_FILE_SYSTEM_MODEL_H

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace morrow::editor {
class AssetDatabase;

enum class FileImportState {
    NotApplicable,
    Ready,
    NeedsImport,
    Failed,
};

enum class FileSortMode {
    Name,
    Type,
    Modified,
    Size,
};

struct ProjectFileEntry {
    int id = -1;
    int parentId = -1;
    std::filesystem::path relativePath;
    std::string name;
    bool directory = false;
    uintmax_t size = 0;
    std::filesystem::file_time_type modifiedTime{};
    std::string assetId;
    std::string assetType;
    FileImportState importState = FileImportState::NotApplicable;
};

class ProjectFileSystemModel {
public:
    bool scan(const std::filesystem::path& projectRoot, const AssetDatabase* assets, std::string& error);

    bool refresh(std::string& error);

    void setSortMode(FileSortMode mode, bool ascending = true);

    FileSortMode sortMode() const;

    bool sortAscending() const;

    const std::filesystem::path& projectRoot() const;

    const std::vector<ProjectFileEntry>& entries() const;

    const ProjectFileEntry* findById(int id) const;

    const ProjectFileEntry* findByPath(const std::filesystem::path& relativePath) const;

    std::vector<const ProjectFileEntry*> filteredEntries(const std::string& query) const;

    bool select(int id, bool additive);

    void clearSelection();

    bool isSelected(const std::filesystem::path& relativePath) const;

    std::vector<const ProjectFileEntry*> selectedEntries() const;

private:
    bool appendDirectory(const std::filesystem::path& absoluteDirectory, const std::filesystem::path& relativeDirectory, int parentId, std::string& error);

    bool shouldSkip(const std::filesystem::directory_entry& entry) const;

    void applyAssetState(ProjectFileEntry& entry) const;

    std::filesystem::path m_projectRoot;
    const AssetDatabase* m_assets = nullptr;
    std::vector<ProjectFileEntry> m_entries;
    std::set<std::filesystem::path> m_selectedPaths;
    FileSortMode m_sortMode = FileSortMode::Name;
    bool m_sortAscending = true;
    int m_nextId = 0;
};

const char* fileImportStateName(FileImportState state);

const char* fileSortModeName(FileSortMode mode);
} // namespace morrow::editor

#endif  // MORROW_EDITOR_PROJECT_FILE_SYSTEM_MODEL_H