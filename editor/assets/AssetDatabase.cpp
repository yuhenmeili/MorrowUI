#include "AssetDatabase.h"

#include <cctype>
#include <fstream>
#include <iomanip>
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

std::string fileHash(const std::filesystem::path& path) {
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

bool isImportable(const std::filesystem::path& path) {
    return extensionType(path) != "Unknown";
}

std::string importerName(const std::string& type) {
    if (type == "Texture")
        return "morrow.texture";
    if (type == "Font")
        return "morrow.font";
    if (type == "GLTF")
        return "morrow.gltf";
    if (type == "Shader")
        return "morrow.shader";
    if (type == "Audio")
        return "morrow.audio";
    return "morrow.asset";
}

std::string generatedAssetId(const std::filesystem::path& relativePath) {
    std::string id = "asset_";
    auto pathWithoutExtension = relativePath;
    pathWithoutExtension.replace_extension("");
    for (const auto character : pathWithoutExtension.generic_string()) {
        if (std::isalnum(static_cast<unsigned char>(character)))
            id.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
        else
            id.push_back('_');
    }
    while (id.size() > 1 && id.back() == '_')
        id.pop_back();
    return id;
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
        if (asset.imported) {
            const auto sourceHash = fileHash(entry.path());
            asset.needsImport = sourceHash.empty() || sourceHash != asset.import.sourceHash || asset.import.importerVersion != currentImporterVersion();
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

bool AssetDatabase::ensureImportMetadata(std::string& error) const {
    if (m_assetRoot.empty() || m_projectRoot.empty()) {
        error = "asset database has not been scanned";
        return false;
    }

    std::unordered_set<std::string> usedIds;
    for (const auto& asset : m_assets) {
        if (!asset.assetId.empty())
            usedIds.insert(asset.assetId);
    }

    std::error_code filesystemError;
    for (std::filesystem::recursive_directory_iterator iterator(m_assetRoot, std::filesystem::directory_options::skip_permission_denied, filesystemError);
         !filesystemError && iterator != std::filesystem::recursive_directory_iterator(); iterator.increment(filesystemError)) {
        const auto& entry = *iterator;
        if (!entry.is_regular_file(filesystemError) || !isImportable(entry.path()))
            continue;

        const auto relativePath = std::filesystem::relative(entry.path(), m_projectRoot, filesystemError);
        if (filesystemError)
            continue;
        const auto importPath = m_projectRoot / (relativePath.string() + ".import");
        if (std::filesystem::exists(importPath, filesystemError))
            continue;

        const std::string type = extensionType(entry.path());
        std::string assetId = generatedAssetId(relativePath);
        if (usedIds.count(assetId) != 0) {
            size_t suffix = 2;
            const std::string base = assetId;
            do {
                assetId = base + "_" + std::to_string(suffix++);
            } while (usedIds.count(assetId) != 0);
        }
        usedIds.insert(assetId);

        std::filesystem::create_directories(importPath.parent_path(), filesystemError);
        if (filesystemError) {
            error = "failed to create import metadata directory: " + filesystemError.message();
            return false;
        }
        std::ofstream output(importPath);
        if (!output.is_open()) {
            error = "failed to create import metadata: " + importPath.string();
            return false;
        }
        output << "[import]\n"
               << "format = 1\n"
               << "asset_id = \"" << assetId << "\"\n"
               << "importer = \"" << importerName(type) << "\"\n"
               << "source_hash = \"\"\n"
               << "importer_version = " << currentImporterVersion() << "\n\n"
               << "[options]\n\n"
               << "[platform.windows]\n"
               << "artifact = \".morrow/imported/" << assetId << "/windows/" << entry.path().filename().string() << ".artifact\"\n";
        if (!output.good()) {
            error = "failed to write import metadata: " + importPath.string();
            return false;
        }
    }
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

bool AssetDatabase::validateSourcePath(const std::filesystem::path& sourcePath, std::string& error) const {
    const auto normalized = sourcePath.lexically_normal();
    if (normalized.is_absolute()) {
        error = "asset path must stay inside the project: " + sourcePath.string();
        return false;
    }
    const auto absolute = std::filesystem::weakly_canonical(m_projectRoot / normalized);
    std::error_code relativeError;
    const auto relativeToAssetRoot = std::filesystem::relative(absolute, m_assetRoot, relativeError);
    if (relativeError || relativeToAssetRoot.empty() || relativeToAssetRoot == ".." ||
        relativeToAssetRoot.string().rfind(".." + std::string(1, std::filesystem::path::preferred_separator), 0) == 0) {
        error = "asset path is outside asset_root: " + sourcePath.string();
        return false;
    }
    if (!std::filesystem::exists(absolute)) {
        error = "asset source does not exist: " + sourcePath.string();
        return false;
    }
    return true;
}

bool AssetDatabase::validateAssetReference(const std::string& assetId, std::string& error) const {
    const auto asset = findById(assetId);
    if (!asset) {
        error = "unknown asset_id: " + assetId;
        return false;
    }
    if (!asset->error.empty()) {
        error = asset->error;
        return false;
    }
    return true;
}

}  // namespace morrow::editor
