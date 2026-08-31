#ifndef MORROW_EDITOR_MATERIAL_ASSET_H
#define MORROW_EDITOR_MATERIAL_ASSET_H

#include <filesystem>
#include <map>
#include <string>

namespace morrow::editor {
struct MaterialAsset {
    int format = 1;
    std::string shader;
    std::map<std::string, std::string> properties;
};

bool loadMaterialAsset(const std::filesystem::path& path, MaterialAsset& material, std::string& error);

bool saveMaterialAsset(const std::filesystem::path& path, const MaterialAsset& material, std::string& error);
}

#endif