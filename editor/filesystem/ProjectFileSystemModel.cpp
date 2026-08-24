#include "ProjectFileSystemModel.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include "assets/AssetDatabase.h"

namespace {

std::string lowercase(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool isInside(
    const std::filesystem::path& root,
    const std::filesystem::path& candidate) {
    std::error_code error;
    const auto relative = std::filesystem::relative(candidate, root, error);
    if (error || relative.empty())
        return false;
    const auto text = relative.generic_string();
    return text != ".." && text.rfind("../", 0) != 0;
}

}  // namespace

namespace morrow::editor {

bool ProjectFileSystemModel::scan(
    const std::filesystem::path& projectRoot,
    const AssetDatabase* assets,
    std::string& error) {
    std::error_code filesystemError;
    const auto canonicalRoot =
        std::filesystem::weakly_canonical(projectRoot, filesystemError);
    if (filesystemError || canonicalRoot.empty() ||
        !std::filesystem::is_directory(canonicalRoot, filesystemError)) {
        error = "project root is not a readable directory: " +
                projectRoot.string();
        return false;
    }

    m_projectRoot = canonicalRoot;
    m_assets = assets;
    m_entries.clear();
    m_nextId = 0;

    ProjectFileEntry root;
    root.id = m_nextId++;
    root.parentId = -1;
    root.relativePath = ".";
    root.name = m_projectRoot.filename().string();
    root.directory = true;
    root.modifiedTime =
        std::filesystem::last_write_time(m_projectRoot, filesystemError);
    m_entries.push_back(std::move(root));

    if (!appendDirectory(m_projectRoot, {}, 0, error)) {
        m_entries.clear();
        return false;
    }
    return true;
}

bool ProjectFileSystemModel::refresh(std::string& error) {
    if (m_projectRoot.empty()) {
        error = "project file system has not been scanned";
        return false;
    }
    return scan(m_projectRoot, m_assets, error);
}

const std::filesystem::path& ProjectFileSystemModel::projectRoot() const {
    return m_projectRoot;
}

const std::vector<ProjectFileEntry>& ProjectFileSystemModel::entries() const {
    return m_entries;
}

const ProjectFileEntry* ProjectFileSystemModel::findById(int id) const {
    for (const auto& entry : m_entries) {
        if (entry.id == id)
            return &entry;
    }
    return nullptr;
}

const ProjectFileEntry* ProjectFileSystemModel::findByPath(
    const std::filesystem::path& relativePath) const {
    const auto normalized = relativePath.lexically_normal();
    for (const auto& entry : m_entries) {
        if (entry.relativePath.lexically_normal() == normalized)
            return &entry;
    }
    return nullptr;
}

std::vector<const ProjectFileEntry*> ProjectFileSystemModel::filteredEntries(
    const std::string& query) const {
    if (query.empty()) {
        std::vector<const ProjectFileEntry*> all;
        all.reserve(m_entries.size());
        for (const auto& entry : m_entries)
            all.push_back(&entry);
        return all;
    }

    const std::string normalizedQuery = lowercase(query);
    std::unordered_set<int> includedIds;
    for (const auto& entry : m_entries) {
        const std::string searchable =
            lowercase(entry.name + " " + entry.relativePath.generic_string());
        if (searchable.find(normalizedQuery) == std::string::npos)
            continue;
        const ProjectFileEntry* current = &entry;
        while (current) {
            if (!includedIds.insert(current->id).second)
                break;
            current = findById(current->parentId);
        }
    }

    std::vector<const ProjectFileEntry*> filtered;
    filtered.reserve(includedIds.size());
    for (const auto& entry : m_entries) {
        if (includedIds.count(entry.id) != 0)
            filtered.push_back(&entry);
    }
    return filtered;
}

bool ProjectFileSystemModel::appendDirectory(
    const std::filesystem::path& absoluteDirectory,
    const std::filesystem::path& relativeDirectory,
    int parentId,
    std::string& error) {
    std::error_code filesystemError;
    std::vector<std::filesystem::directory_entry> children;
    for (std::filesystem::directory_iterator iterator(
             absoluteDirectory,
             std::filesystem::directory_options::skip_permission_denied,
             filesystemError);
         !filesystemError && iterator != std::filesystem::directory_iterator();
         iterator.increment(filesystemError)) {
        if (!shouldSkip(*iterator))
            children.push_back(*iterator);
    }
    if (filesystemError) {
        error = "failed to enumerate directory '" +
                absoluteDirectory.string() + "': " + filesystemError.message();
        return false;
    }

    std::sort(
        children.begin(), children.end(),
        [](const auto& left, const auto& right) {
            std::error_code leftError;
            std::error_code rightError;
            const bool leftDirectory = left.is_directory(leftError);
            const bool rightDirectory = right.is_directory(rightError);
            if (leftDirectory != rightDirectory)
                return leftDirectory;
            return lowercase(left.path().filename().string()) <
                   lowercase(right.path().filename().string());
        });

    for (const auto& child : children) {
        std::error_code typeError;
        const bool directory = child.is_directory(typeError);
        if (typeError || child.is_symlink(typeError))
            continue;

        const auto childRelative =
            (relativeDirectory / child.path().filename()).lexically_normal();
        const auto absoluteChild =
            std::filesystem::weakly_canonical(child.path(), typeError);
        if (typeError || !isInside(m_projectRoot, absoluteChild))
            continue;

        ProjectFileEntry entry;
        entry.id = m_nextId++;
        entry.parentId = parentId;
        entry.relativePath = childRelative;
        entry.name = child.path().filename().string();
        entry.directory = directory;
        entry.modifiedTime = child.last_write_time(typeError);
        if (!directory) {
            entry.size = child.file_size(typeError);
            applyAssetState(entry);
        }
        const int entryId = entry.id;
        m_entries.push_back(std::move(entry));

        if (directory &&
            !appendDirectory(
                absoluteChild, childRelative, entryId, error)) {
            return false;
        }
    }
    return true;
}

bool ProjectFileSystemModel::shouldSkip(
    const std::filesystem::directory_entry& entry) const {
    const std::string name = entry.path().filename().string();
    std::error_code error;
    if (entry.is_directory(error)) {
        return name == ".morrow" || name == ".git" ||
               name == ".idea" || name == ".vscode";
    }
    return entry.path().extension() == ".import";
}

void ProjectFileSystemModel::applyAssetState(ProjectFileEntry& entry) const {
    if (!m_assets)
        return;
    const auto* asset = m_assets->findBySourcePath(entry.relativePath);
    if (!asset)
        return;
    entry.assetId = asset->assetId;
    entry.assetType = asset->type;
    if (!asset->error.empty()) {
        entry.importState = FileImportState::Failed;
    } else if (asset->needsImport || !asset->imported) {
        entry.importState = FileImportState::NeedsImport;
    } else {
        entry.importState = FileImportState::Ready;
    }
}

const char* fileImportStateName(FileImportState state) {
    switch (state) {
        case FileImportState::Ready:
            return "ready";
        case FileImportState::NeedsImport:
            return "import required";
        case FileImportState::Failed:
            return "failed";
        case FileImportState::NotApplicable:
            return "file";
    }
    return "file";
}

}  // namespace morrow::editor
