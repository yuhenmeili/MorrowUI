#include "NodeTypeCatalog.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace {

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

std::string baseName(const std::string& type) {
    if (type.rfind("MR", 0) == 0)
        return type.substr(2);
    if (type == "SceneNode")
        return "Node";
    return type;
}

}  // namespace

namespace morrow::editor {

NodeTypeCatalog::NodeTypeCatalog() {
    const std::map<std::string, std::string> common = {
        {"position", "Vector3(80.0, 80.0, 0.0)"},
        {"rotation", "Vector3(0.0, 0.0, 0.0)"},
        {"scale", "Vector3(1.0, 1.0, 1.0)"},
        {"size", "Vector2(180.0, 80.0)"},
        {"visible", "true"},
    };
    const auto add = [this, &common](
                         const std::string& type,
                         const std::string& name,
                         const std::string& category,
                         const std::string& description,
                         std::map<std::string, std::string> properties = {}) {
        if (type != "SceneNode") {
            for (const auto& [key, value] : common)
                properties.emplace(key, value);
        }
        m_types.push_back(
            {type, name, category, description, std::move(properties)});
    };

    add("SceneNode", "Node", "Core",
        "A non-rendering scene container used to organize child nodes.");

    add("MRButton", "Button", "Basic Controls",
        "A clickable text button.",
        {{"background_color", "Color(0.16, 0.31, 0.63, 1.0)"},
         {"font_size", "18"}, {"size", "Vector2(180.0, 48.0)"},
         {"text", "Button"}});
    add("MRLabel", "Label", "Basic Controls",
        "A text display control.",
        {{"font_size", "22"}, {"size", "Vector2(180.0, 40.0)"},
         {"text", "Label"}});
    add("MRImage", "Image", "Basic Controls",
        "A rectangular image control.",
        {{"size", "Vector2(180.0, 120.0)"},
         {"texture_asset", ""}});
    add("MRColor", "Color Rect", "Basic Controls",
        "A solid-color rectangular control.");

    add("MRCheckBox", "Check Box", "Input Controls",
        "A selectable check box control.");
    add("MRCheckButton", "Check Button", "Input Controls",
        "A compact selectable check button.");
    add("MRRadioButton", "Radio Button", "Input Controls",
        "A mutually-exclusive selectable button.");
    add("MRToggle", "Toggle", "Input Controls",
        "An on/off toggle control.");
    add("MRLineEdit", "Line Edit", "Input Controls",
        "A single-line text editor.");
    add("MRTextEdit", "Text Edit", "Input Controls",
        "A multi-line text editor.");
    add("MRSpinBox", "Spin Box", "Input Controls",
        "A numeric input with increment and decrement controls.");
    add("MRSlider", "Slider", "Input Controls",
        "A draggable value slider.");
    add("MRMenuButton", "Menu Button", "Input Controls",
        "A button that opens a popup menu.");
    add("MROptionButton", "Option Button", "Input Controls",
        "A button used to choose one option.");
    add("MRTextureButton", "Texture Button", "Input Controls",
        "A button whose states are represented by textures.");

    add("MRProgressBar", "Progress Bar", "Data Display",
        "A progress value display.");
    add("MRItemList", "Item List", "Data Display",
        "A scrollable item list.");
    add("MRTree", "Tree", "Data Display",
        "A hierarchical tree control.");
    add("MRRichTextLabel", "Rich Text Label", "Data Display",
        "A text control supporting rich text.");
    add("MRScrollBar", "Scroll Bar", "Data Display",
        "A standalone scroll bar.");

    add("CenterContainer", "Center Container", "Layout",
        "Centers its children inside the available area.");
    add("HBoxContainer", "HBox Container", "Layout",
        "Places children from left to right.");
    add("VBoxContainer", "VBox Container", "Layout",
        "Places children from top to bottom.");
    add("MarginContainer", "Margin Container", "Layout",
        "Adds margins around a child.");
    add("MRScrollContainer", "Scroll Container", "Layout",
        "Provides a scrollable content area.");
    add("MRSplitContainer", "Split Container", "Layout",
        "Divides an area into two resizable panes.");
    add("MRTabContainer", "Tab Container", "Layout",
        "Displays one child page at a time using tabs.");
    add("MRSpacer", "Spacer", "Layout",
        "Consumes flexible layout space.");
    add("MRHSeparator", "Horizontal Separator", "Layout",
        "A horizontal visual separator.",
        {{"size", "Vector2(180.0, 2.0)"}});
    add("MRVSeparator", "Vertical Separator", "Layout",
        "A vertical visual separator.",
        {{"size", "Vector2(2.0, 120.0)"}});

    add("MRPopup", "Popup", "Popups",
        "A basic popup surface.");
    add("MRPopupPanel", "Popup Panel", "Popups",
        "A popup with a titled content panel.");
    add("MRWindow", "Window", "Popups",
        "A closeable popup window.");
    add("MRDialog", "Dialog", "Popups",
        "A confirmation and cancellation dialog.");
    add("MRTooltip", "Tooltip", "Popups",
        "A contextual tooltip popup.");
    add("MRPopupMenu", "Popup Menu", "Popups",
        "A popup list of commands.");

    add("MR3DSceneView", "3D Scene View", "Rendering & Effects",
        "Displays a 3D scene using an offscreen render target.");
    add("MRCanvasModulate", "Canvas Modulate", "Rendering & Effects",
        "Applies a color modulation overlay.");
    add("MRParallax2D", "Parallax 2D", "Rendering & Effects",
        "A single 2D parallax layer.");
    add("MRParallaxBackground", "Parallax Background",
        "Rendering & Effects", "A container for parallax layers.");
    add("MRCPUParticles2D", "CPU Particles 2D",
        "Rendering & Effects", "A CPU-driven 2D particle emitter.");
    add("MRGPUParticles2D", "GPU Particles 2D",
        "Rendering & Effects", "A GPU-driven 2D particle emitter.");
    add("MRVideoStreamPlayer", "Video Stream Player",
        "Rendering & Effects", "A widget for presenting streamed video.");
    add("MRFrameAnimation", "Frame Animation",
        "Rendering & Effects", "A frame-based image animation.");
    add("MRBounce", "Bounce Effect", "Rendering & Effects",
        "A configurable bounce visual effect.");
    add("MRFlowingLight", "Flowing Light", "Rendering & Effects",
        "An animated flowing-light effect.");
    add("MRAnchorPointScale", "Anchor Point Scale",
        "Rendering & Effects", "An anchor-based scale effect.");
    add("MRBrakePedal", "Brake Pedal", "Rendering & Effects",
        "A specialized brake pedal visual.");
    add("MRGearsIris", "Gears Iris", "Rendering & Effects",
        "An animated gears iris effect.");
    add("MRGearsOpening", "Gears Opening", "Rendering & Effects",
        "An animated gears opening effect.");
    add("MRGearsSelect", "Gears Select", "Rendering & Effects",
        "An animated gears selection effect.");
    add("MRGearsShine", "Gears Shine", "Rendering & Effects",
        "An animated gears shine effect.");
}

const std::vector<NodeTypeDescriptor>& NodeTypeCatalog::types() const {
    return m_types;
}

const NodeTypeDescriptor* NodeTypeCatalog::find(const std::string& type) const {
    const auto iterator = std::find_if(m_types.begin(), m_types.end(), [&type](const NodeTypeDescriptor& descriptor) { return descriptor.type == type; });
    return iterator == m_types.end() ? nullptr : &*iterator;
}

std::vector<const NodeTypeDescriptor*> NodeTypeCatalog::filter(const std::string& query) const {
    const std::string normalized = lowercase(query);
    std::vector<const NodeTypeDescriptor*> result;
    for (const auto& descriptor : m_types) {
        const std::string searchable = lowercase(descriptor.type + " " + descriptor.displayName + " " + descriptor.category + " " + descriptor.description);
        if (normalized.empty() || searchable.find(normalized) != std::string::npos) {
            result.push_back(&descriptor);
        }
    }
    return result;
}

SceneNodeRecord NodeTypeCatalog::createNode(const NodeTypeDescriptor& descriptor, const std::string& parentId, const SceneDocument& document) const {
    SceneNodeRecord node;
    node.type = descriptor.type;
    node.parentId = parentId;
    node.properties = descriptor.defaultProperties;

    for (int index = 1;; ++index) {
        std::ostringstream id;
        id << "node_" << std::setw(4) << std::setfill('0') << index;
        if (!document.findNode(id.str())) {
            node.id = id.str();
            break;
        }
    }

    const std::string base = baseName(descriptor.type);
    node.name = base;
    for (int suffix = 2;; ++suffix) {
        const bool collision = std::any_of(document.nodes().begin(), document.nodes().end(),
                                           [&node](const SceneNodeRecord& existing) { return existing.parentId == node.parentId && existing.name == node.name; });
        if (!collision)
            break;
        node.name = base + std::to_string(suffix);
    }
    return node;
}

}  // namespace morrow::editor
