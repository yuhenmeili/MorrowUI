#include "AssetDatabase.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string unquote(const std::string& value) {
    const auto text = trim(value);
    if (text.size() < 2 || text.front() != '"' || text.back() != '"') {
        return text;
    }
    std::ostringstream output;
    for (size_t index = 1; index + 1 < text.size(); ++index) {
        if (text[index] == '\\' && index + 2 < text.size()) {
            const char escaped = text[++index];
            switch (escaped) {
                case 'n':
                    output << '\n';
                    break;
                case 'r':
                    output << '\r';
                    break;
                case 't':
                    output << '\t';
                    break;
                default:
                    output << escaped;
                    break;
            }
        } else {
            output << text[index];
        }
    }
    return output.str();
}

std::string extensionType(const std::filesystem::path& path) {
    const auto extension = path.extension().string();
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".basis") {
        return "Texture";
    }
    if (extension == ".otf" || extension == ".ttf")
        return "Font";
    if (extension == ".gltf" || extension == ".glb")
        return "GLTF";
    if (extension == ".vert" || extension == ".frag" || extension == ".glsl") {
        return "Shader";
    }
    if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
        return "Audio";
    }
    return "Unknown";
}

bool isImportable(const std::filesystem::path& path) {
    return extensionType(path) != "Unknown";
}

}  // namespace

namespace morrow::editor {

bool AssetDatabase::scan(const std::filesystem::path& projectRoot, const std::filesystem::path& assetRoot, std::string& error) {
    m_projectRoot = std::filesystem::weakly_canonical(projectRoot);
    m_assetRoot = std::filesystem::weakly_canonical(m_projectRoot / assetRoot);
    m_assets.clear();

    std::error_code filesystemError;
    if (!std::filesystem::exists(m_assetRoot, filesystemError)) {
        error = "asset root does not exist: " + m_assetRoot.string();
        return false;
    }

    std::unordered_set<std::string> assetIds;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_assetRoot)) {
        if (!entry.is_regular_file() || !isImportable(entry.path()))
            continue;

        AssetRecord asset;
        asset.type = extensionType(entry.path());
        asset.sourcePath = std::filesystem::relative(entry.path(), m_projectRoot, filesystemError);
        asset.importPath = asset.sourcePath;
        asset.importPath += ".import";

        const auto absoluteImportPath = m_projectRoot / asset.importPath;
        if (!std::filesystem::exists(absoluteImportPath, filesystemError)) {
            asset.error = "missing import metadata: " + absoluteImportPath.string();
            m_assets.push_back(std::move(asset));
            continue;
        }

        if (!loadImportFile(absoluteImportPath, asset.import, error)) {
            asset.error = error;
            error.clear();
            m_assets.push_back(std::move(asset));
            continue;
        }
        asset.assetId = asset.import.assetId;
        asset.imported = !asset.assetId.empty() && asset.error.empty();
        if (asset.assetId.empty()) {
            asset.error = "import metadata has no asset_id";
            asset.imported = false;
        } else if (!assetIds.insert(asset.assetId).second) {
            asset.error = "duplicate asset_id: " + asset.assetId;
            asset.imported = false;
        }
        m_assets.push_back(std::move(asset));
    }
    return true;
}

bool AssetDatabase::loadImportFile(const std::filesystem::path& importPath, ImportMetadata& metadata, std::string& error) const {
    std::ifstream input(importPath);
    if (!input.is_open()) {
        error = "failed to open import file: " + importPath.string();
        return false;
    }

    std::string section;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const auto content = trim(line);
        if (content.empty() || content.front() == '#')
            continue;
        if (content.front() == '[' && content.back() == ']') {
            section = content.substr(1, content.size() - 2);
            continue;
        }
        const auto separator = content.find('=');
        if (separator == std::string::npos) {
            error = "line " + std::to_string(lineNumber) + ": expected key=value";
            return false;
        }
        const auto key = trim(content.substr(0, separator));
        const auto value = unquote(content.substr(separator + 1));
        if (section == "import") {
            if (key == "format")
                metadata.format = std::stoi(value);
            else if (key == "asset_id")
                metadata.assetId = value;
            else if (key == "importer")
                metadata.importer = value;
            else if (key == "source_hash")
                metadata.sourceHash = value;
            else if (key == "importer_version") {
                metadata.importerVersion = std::stoi(value);
            }
        } else if (section == "options") {
            metadata.options[key] = value;
        } else if (section.rfind("platform.", 0) == 0) {
            metadata.artifacts[section.substr(9) + "." + key] = value;
        }
    }
    metadata.line = lineNumber;
    return true;
}

const AssetRecord* AssetDatabase::findById(const std::string& assetId) const {
    for (const auto& asset : m_assets) {
        if (asset.assetId == assetId)
            return &asset;
    }
    return nullptr;
}

const AssetRecord* AssetDatabase::findBySourcePath(const std::filesystem::path& sourcePath) const {
    const auto normalized = sourcePath.lexically_normal();
    for (const auto& asset : m_assets) {
        if (asset.sourcePath.lexically_normal() == normalized)
            return &asset;
    }
    return nullptr;
}

const std::vector<AssetRecord>& AssetDatabase::assets() const {
    return m_assets;
}

std::filesystem::path AssetDatabase::resolveSourcePath(const std::string& assetId) const {
    const auto asset = findById(assetId);
    return asset ? m_projectRoot / asset->sourcePath : std::filesystem::path{};
}

}  // namespace morrow::editor
