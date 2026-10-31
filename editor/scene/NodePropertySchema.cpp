#include "scene/NodePropertySchema.h"

#include <algorithm>

namespace morrow::editor {
namespace {

const std::vector<PropertyDescriptor>& commonTable() {
    static const std::vector<PropertyDescriptor> table = {
        {"position", "Vector3", "Vector3(0.0, 0.0, 0.0)", "Transform", "Position", true},
        {"rotation", "Vector3", "Vector3(0.0, 0.0, 0.0)", "Transform", "Rotation", true},
        {"scale", "Vector3", "Vector3(1.0, 1.0, 1.0)", "Transform", "Scale", true},
        {"size", "Vector2", "Vector2(100.0, 40.0)", "Transform", "Size", true},
        {"visible", "bool", "true", "Properties", "Visible", true},
        {"locked", "bool", "false", "Properties", "Locked", true},
        {"display_layer", "number", "0", "Properties", "Display Layer", true},
    };
    return table;
}

const std::vector<PropertyDescriptor>& meshRendererTable() {
    static const std::vector<PropertyDescriptor> table = {
        {"mesh", "string", "Generated UI Quad", "Mesh Filter", "Mesh", false},
        {"renderer_enabled", "bool", "true", "Mesh Renderer", "Enabled", true},
        {"material", "MaterialAsset", "Default UI Material", "Mesh Renderer", "Material", false},
        {"material_color", "Color", "Color(1.0, 1.0, 1.0, 1.0)", "Mesh Renderer", "Color", true},
        {"blend_enabled", "bool", "true", "Mesh Renderer", "Blend", true},
        {"double_sided", "bool", "false", "Mesh Renderer", "Double Sided", true},
    };
    return table;
}

/// 按节点类型分发的专属属性。只登记 SceneInstantiator 真正能应用的键，
/// 避免出现"编辑器能改但运行时不生效"的假属性。
const std::vector<PropertyDescriptor>* typeTable(const std::string& nodeType) {
    static const std::vector<PropertyDescriptor> button = {
        {"text", "string", "Button", "Properties", "Text", true},
        {"font_size", "number", "18", "Properties", "Font Size", true},
    };
    static const std::vector<PropertyDescriptor> label = {
        {"text", "string", "Label", "Properties", "Text", true},
        {"font_size", "number", "22", "Properties", "Font Size", true},
    };
    static const std::vector<PropertyDescriptor> image = {
        {"texture_asset", "TextureAsset", "", "Properties", "Source Image", true},
    };
    if (nodeType == "MRButton")
        return &button;
    if (nodeType == "MRLabel")
        return &label;
    if (nodeType == "MRImage")
        return &image;
    return nullptr;
}

}  // namespace

const std::vector<PropertyDescriptor>& commonProperties(bool sceneNode3D) {
    (void)sceneNode3D;
    return commonTable();
}

const std::vector<PropertyDescriptor>* typeProperties(const std::string& nodeType) {
    return typeTable(nodeType);
}

const PropertyDescriptor* findProperty(const std::string& nodeType, bool sceneNode3D, const std::string& name) {
    if (!sceneNode3D) {
        const auto& meshRenderer = meshRendererProperties();
        const auto renderer = std::find_if(meshRenderer.begin(), meshRenderer.end(), [&name](const PropertyDescriptor& p) { return p.name == name; });
        if (renderer != meshRenderer.end())
            return &*renderer;
    }
    const auto& common = commonTable();
    const auto entry = std::find_if(common.begin(), common.end(), [&name](const PropertyDescriptor& p) { return p.name == name; });
    if (entry != common.end())
        return &*entry;
    const auto* typeSpecific = typeTable(nodeType);
    if (!typeSpecific)
        return nullptr;
    const auto typed = std::find_if(typeSpecific->begin(), typeSpecific->end(), [&name](const PropertyDescriptor& p) { return p.name == name; });
    return typed == typeSpecific->end() ? nullptr : &*typed;
}

const std::vector<PropertyDescriptor>& meshRendererProperties() {
    return meshRendererTable();
}

}  // namespace morrow::editor
