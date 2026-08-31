#ifndef MORROW_EDITOR_ASSET_TYPE_CATALOG_H
#define MORROW_EDITOR_ASSET_TYPE_CATALOG_H

#include <string>
#include <vector>

namespace morrow::editor {
struct AssetTypeDescriptor {
    std::string type;
    std::string displayName;
    std::string extension;
};

inline const std::vector<AssetTypeDescriptor>& assetTypes() {
    static const std::vector<AssetTypeDescriptor> types = {{"Material", "Material", ".mat"}, {"Shader", "Shader", ".glsl"},};
    return types;
}
}

#endif