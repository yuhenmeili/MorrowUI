#include "scene/NodePropertySchema.h"

#include <algorithm>

namespace morrow::editor {
namespace {

/// 枚举取值表（序列化值小写，显示名可读）。
const std::vector<std::pair<std::string, std::string>>& horizontalAlignOptions() {
    static const std::vector<std::pair<std::string, std::string>> options = {{"left", "Left"}, {"center", "Center"}, {"right", "Right"}};
    return options;
}

const std::vector<std::pair<std::string, std::string>>& verticalAlignOptions() {
    static const std::vector<std::pair<std::string, std::string>> options = {{"top", "Top"}, {"center", "Center"}, {"bottom", "Bottom"}};
    return options;
}

const std::vector<std::pair<std::string, std::string>>& progressDirectionOptions() {
    static const std::vector<std::pair<std::string, std::string>> options = {
        {"left_to_right", "Left to Right"}, {"right_to_left", "Right to Left"}, {"bottom_to_top", "Bottom to Top"}, {"top_to_bottom", "Top to Bottom"}};
    return options;
}

const std::vector<std::pair<std::string, std::string>>& splitOrientationOptions() {
    static const std::vector<std::pair<std::string, std::string>> options = {{"horizontal", "Horizontal"}, {"vertical", "Vertical"}};
    return options;
}

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

/// MRButton 及其全部子类（CheckBox/Radio/Toggle/Option/Menu/Segmented…）共享的外观属性。
const std::vector<PropertyDescriptor>& buttonVisualTable() {
    static const std::vector<PropertyDescriptor> table = {
        {"text", "string", "Button", "Properties", "Text", true},
        {"font_size", "number", "18", "Properties", "Font Size", true, true, 8.0f, 72.0f, 1.0f},
        {"background_color", "Color", "Color(0.16, 0.31, 0.63, 1.0)", "Properties", "Background", true},
        {"hover_color", "Color", "Color(0.30, 0.55, 0.95, 1.0)", "Properties", "Hover Color", true},
        {"pressed_color", "Color", "Color(0.10, 0.22, 0.48, 1.0)", "Properties", "Pressed Color", true},
        {"text_color", "Color", "Color(1.0, 1.0, 1.0, 1.0)", "Properties", "Text Color", true},
        {"corner_radius", "number", "0", "Properties", "Corner Radius", true, true, 0.0f, 50.0f, 1.0f},
        {"border_width", "number", "0", "Properties", "Border Width", true, true, 0.0f, 20.0f, 1.0f},
        {"border_color", "Color", "Color(0.0, 0.0, 0.0, 1.0)", "Properties", "Border Color", true},
        {"text_align_h", "enum", "center", "Properties", "Text Align H", true, false, 0.0f, 0.0f, 1.0f, horizontalAlignOptions()},
        {"text_align_v", "enum", "center", "Properties", "Text Align V", true, false, 0.0f, 0.0f, 1.0f, verticalAlignOptions()},
        {"auto_wrap", "bool", "false", "Properties", "Auto Wrap", true},
    };
    return table;
}

/// 可选中类按钮 = 按钮外观 + checked。
const std::vector<PropertyDescriptor>& selectableTable() {
    static const std::vector<PropertyDescriptor> table = [] {
        auto result = buttonVisualTable();
        result.push_back({"checked", "bool", "false", "Properties", "Checked", true});
        return result;
    }();
    return table;
}

/// 进度/滑杆共享（值为 0..1 归一化）。
const std::vector<PropertyDescriptor>& progressTable() {
    static const std::vector<PropertyDescriptor> table = {
        {"progress", "number", "0.0", "Properties", "Progress", true, true, 0.0f, 1.0f, 0.01f},
        {"direction", "enum", "left_to_right", "Properties", "Direction", true, false, 0.0f, 0.0f, 1.0f, progressDirectionOptions()},
        {"track_color", "Color", "Color(0.20, 0.22, 0.26, 1.0)", "Properties", "Track Color", true},
        {"fill_color", "Color", "Color(0.20, 0.55, 0.95, 1.0)", "Properties", "Fill Color", true},
    };
    return table;
}

/// 按节点类型分发的专属属性。只登记 SceneInstantiator 真正能应用的键，
/// 避免出现"编辑器能改但运行时不生效"的假属性。
const std::vector<PropertyDescriptor>* typeTable(const std::string& nodeType) {
    static const std::vector<PropertyDescriptor> button = buttonVisualTable();
    static const std::vector<PropertyDescriptor> label = {
        {"text", "string", "Label", "Properties", "Text", true},
        {"font_size", "number", "22", "Properties", "Font Size", true, true, 8.0f, 72.0f, 1.0f},
        {"font_color", "Color", "Color(1.0, 1.0, 1.0, 1.0)", "Properties", "Font Color", true},
        {"align_h", "enum", "left", "Properties", "Align H", true, false, 0.0f, 0.0f, 1.0f, horizontalAlignOptions()},
        {"align_v", "enum", "top", "Properties", "Align V", true, false, 0.0f, 0.0f, 1.0f, verticalAlignOptions()},
        {"auto_wrap", "bool", "false", "Properties", "Auto Wrap", true},
        {"max_lines", "number", "0", "Properties", "Max Lines", true, true, 0.0f, 20.0f, 1.0f},
        {"line_spacing", "number", "1.0", "Properties", "Line Spacing", true, true, 0.0f, 5.0f, 0.1f},
    };
    static const std::vector<PropertyDescriptor> image = {
        {"texture_asset", "TextureAsset", "", "Properties", "Source Image", true},
        {"rounding", "number", "0", "Properties", "Corner Radius", true, true, 0.0f, 50.0f, 1.0f},
    };
    static const std::vector<PropertyDescriptor> colorRect = {
        {"color", "Color", "Color(1.0, 1.0, 1.0, 1.0)", "Properties", "Color", true},
        {"rounding", "number", "0", "Properties", "Corner Radius", true, true, 0.0f, 50.0f, 1.0f},
    };
    static const std::vector<PropertyDescriptor> boxSpacing = {
        {"spacing", "number", "0", "Properties", "Spacing", true, true, 0.0f, 100.0f, 1.0f},
    };
    static const std::vector<PropertyDescriptor> grid = {
        {"columns", "number", "2", "Properties", "Columns", true, true, 1.0f, 16.0f, 1.0f},
        {"spacing", "number", "8", "Properties", "Spacing", true, true, 0.0f, 100.0f, 1.0f},
    };
    static const std::vector<PropertyDescriptor> margin = {
        {"margin_left", "number", "0", "Properties", "Margin Left", true, true, 0.0f, 200.0f, 1.0f},
        {"margin_top", "number", "0", "Properties", "Margin Top", true, true, 0.0f, 200.0f, 1.0f},
        {"margin_right", "number", "0", "Properties", "Margin Right", true, true, 0.0f, 200.0f, 1.0f},
        {"margin_bottom", "number", "0", "Properties", "Margin Bottom", true, true, 0.0f, 200.0f, 1.0f},
    };
    static const std::vector<PropertyDescriptor> split = {
        {"split_ratio", "number", "0.5", "Properties", "Split Ratio", true, true, 0.0f, 1.0f, 0.01f},
        {"handle_width", "number", "5", "Properties", "Handle Width", true, true, 0.0f, 20.0f, 1.0f},
        {"orientation", "enum", "horizontal", "Properties", "Orientation", true, false, 0.0f, 0.0f, 1.0f, splitOrientationOptions()},
    };
    static const std::vector<PropertyDescriptor> spacer = {
        {"flex", "number", "1.0", "Properties", "Flex", true, true, 0.0f, 10.0f, 0.1f},
    };
    static const std::vector<PropertyDescriptor> spinBox = {
        {"value", "number", "0", "Properties", "Value", true},
        {"min", "number", "0", "Properties", "Minimum", true},
        {"max", "number", "100", "Properties", "Maximum", true},
        {"step", "number", "1", "Properties", "Step", true},
        {"decimals", "number", "0", "Properties", "Decimals", true, true, 0.0f, 4.0f, 1.0f},
        {"prefix", "string", "", "Properties", "Prefix", true},
        {"suffix", "string", "", "Properties", "Suffix", true},
    };
    static const std::vector<PropertyDescriptor> lineEdit = {
        {"text", "string", "", "Properties", "Text", true},
        {"placeholder", "string", "", "Properties", "Placeholder", true},
        {"password_mode", "bool", "false", "Properties", "Password Mode", true},
        {"read_only", "bool", "false", "Properties", "Read Only", true},
    };
    static const std::vector<PropertyDescriptor> textEdit = {
        {"text", "string", "", "Properties", "Text", true},
        {"placeholder", "string", "", "Properties", "Placeholder", true},
        {"read_only", "bool", "false", "Properties", "Read Only", true},
        {"auto_wrap", "bool", "false", "Properties", "Auto Wrap", true},
    };
    static const std::vector<PropertyDescriptor> scroll = {
        {"scroll_step", "number", "48", "Properties", "Scroll Step", true, true, 1.0f, 200.0f, 1.0f},
    };

    if (nodeType == "MRButton")
        return &button;
    if (nodeType == "MRLabel")
        return &label;
    if (nodeType == "MRImage")
        return &image;
    if (nodeType == "MRColor")
        return &colorRect;
    if (nodeType == "MRHSeparator" || nodeType == "MRVSeparator")
        return &colorRect;
    if (nodeType == "HBoxContainer" || nodeType == "VBoxContainer")
        return &boxSpacing;
    if (nodeType == "GridContainer")
        return &grid;
    if (nodeType == "MarginContainer")
        return &margin;
    if (nodeType == "MRSplitContainer")
        return &split;
    if (nodeType == "MRSpacer")
        return &spacer;
    if (nodeType == "MRProgressBar")
        return &progressTable();
    if (nodeType == "MRSlider")
        return &progressTable();
    if (nodeType == "MRSpinBox")
        return &spinBox;
    if (nodeType == "MRLineEdit")
        return &lineEdit;
    if (nodeType == "MRTextEdit")
        return &textEdit;
    if (nodeType == "MRScrollContainer")
        return &scroll;
    if (nodeType == "MRCheckBox" || nodeType == "MRCheckButton" || nodeType == "MRToggle" || nodeType == "MRRadioButton" || nodeType == "MRSelectableButton")
        return &selectableTable();
    if (nodeType == "MRMenuButton" || nodeType == "MROptionButton" || nodeType == "MRTextureButton")
        return &button;
    return nullptr;
}

}  // namespace

const std::vector<PropertyDescriptor>& commonProperties(bool sceneNode3D) {
    (void)sceneNode3D;
    return commonTable();
}

const std::vector<PropertyDescriptor>& meshRendererProperties() {
    return meshRendererTable();
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

}  // namespace morrow::editor
