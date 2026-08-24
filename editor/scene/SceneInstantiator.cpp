#include "SceneInstantiator.h"

#include <cstring>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "SceneDocument.h"
#include "assets/AssetDatabase.h"
#include "Vector3.h"
#include "Vector4.h"
#include "base/Transform.h"
#include "base/UIWidget.h"
#include "elements/MRButton.h"
#include "elements/MR3DSceneView.h"
#include "elements/MRAnchorPointScale.h"
#include "elements/MRBounce.h"
#include "elements/MRBrakePedal.h"
#include "elements/MRCanvasModulate.h"
#include "elements/MRCheckBox.h"
#include "elements/MRCheckButton.h"
#include "elements/MRColor.h"
#include "elements/MRFlowingLight.h"
#include "elements/MRFrameAnimation.h"
#include "elements/MRGearsIris.h"
#include "elements/MRGearsOpening.h"
#include "elements/MRGearsSelect.h"
#include "elements/MRGearsShine.h"
#include "elements/MRImage.h"
#include "elements/MRItemList.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRMenuButton.h"
#include "elements/MROptionButton.h"
#include "elements/MRParallax2D.h"
#include "elements/MRParallaxBackground.h"
#include "elements/MRParticles2D.h"
#include "elements/MRPopup.h"
#include "elements/MRPopupMenu.h"
#include "elements/MRProgressBar.h"
#include "elements/MRRadioButton.h"
#include "elements/MRRichTextLabel.h"
#include "elements/MRScrollBar.h"
#include "elements/MRScrollContainer.h"
#include "elements/MRSeparator.h"
#include "elements/MRSlider.h"
#include "elements/MRSpacer.h"
#include "elements/MRSpinBox.h"
#include "elements/MRTextEdit.h"
#include "elements/MRTextureButton.h"
#include "elements/MRToggle.h"
#include "elements/MRTree.h"
#include "elements/MRVideoStreamPlayer.h"
#include "layout/CenterContainer.h"
#include "layout/HBoxContainer.h"
#include "layout/MarginContainer.h"
#include "layout/MRSplitContainer.h"
#include "layout/MRTabContainer.h"
#include "layout/VBoxContainer.h"

namespace {

using morrow::Math::Vector3;
using morrow::Math::Vector4;

class SceneContainer2D final : public morrow::UIWidget {
public:
    SceneContainer2D() : UIWidget(false) {
        setWidgetType("SceneNode");
    }
};

class CatalogPlaceholderWidget final : public morrow::UIWidget {
public:
    explicit CatalogPlaceholderWidget(const std::string& type)
        : UIWidget(false) {
        setWidgetType(type);
    }
};

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parseVector(const std::string& value, const std::string& type, std::vector<float>& components) {
    if (value.size() <= type.size() + 2 || value.compare(0, type.size(), type) != 0 || value[type.size()] != '(' || value.back() != ')') {
        return false;
    }

    std::stringstream stream(value.substr(type.size() + 1, value.size() - type.size() - 2));
    std::string component;
    while (std::getline(stream, component, ',')) {
        try {
            components.push_back(std::stof(trim(component)));
        } catch (...) {
            return false;
        }
    }
    return !components.empty();
}

bool parseBool(const std::string& value, bool& result) {
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

std::wstring toWide(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

std::shared_ptr<morrow::Widget> createNode(const morrow::editor::SceneNodeRecord& record, std::string& error) {
    if (record.type == "SceneNode") {
        // Phase 1 scenes are 2D editor documents. Use a non-rendering 2D
        // container so UI descendants retain the normal Transform chain.
        return std::make_shared<SceneContainer2D>();
    }
    if (record.type == "MRButton") {
        return morrow::MRButton::create();
    }
    if (record.type == "MRLabel") {
        return std::make_shared<morrow::MRLabel>();
    }
    if (record.type == "MRImage") {
        return morrow::MRImage::create();
    }
    if (record.type == "MRColor") return morrow::MRColor::create();
    if (record.type == "MRCheckBox") return morrow::MRCheckBox::create();
    if (record.type == "MRCheckButton") return morrow::MRCheckButton::create();
    if (record.type == "MRRadioButton") return morrow::MRRadioButton::create();
    if (record.type == "MRToggle") return morrow::MRToggle::create();
    if (record.type == "MRLineEdit") return morrow::MRLineEdit::create();
    if (record.type == "MRTextEdit") return morrow::MRTextEdit::create();
    if (record.type == "MRSpinBox") return morrow::MRSpinBox::create();
    if (record.type == "MRSlider") return morrow::MRSlider::create();
    if (record.type == "MRMenuButton") return morrow::MRMenuButton::create();
    if (record.type == "MROptionButton") return morrow::MROptionButton::create();
    if (record.type == "MRTextureButton") return morrow::MRTextureButton::create();
    if (record.type == "MRProgressBar") return morrow::MRProgressBar::create();
    if (record.type == "MRItemList") return morrow::MRItemList::create();
    if (record.type == "MRTree") return morrow::MRTree::create();
    if (record.type == "MRRichTextLabel") return morrow::MRRichTextLabel::create();
    if (record.type == "MRScrollBar") return morrow::MRScrollBar::create();
    if (record.type == "MRScrollContainer") return morrow::MRScrollContainer::create();
    if (record.type == "MRSpacer") return morrow::MRSpacer::create();
    if (record.type == "MRHSeparator") return morrow::MRHSeparator::create();
    if (record.type == "MRVSeparator") return morrow::MRVSeparator::create();
    if (record.type == "CenterContainer") return std::make_shared<morrow::CenterContainer>();
    if (record.type == "HBoxContainer") return std::make_shared<morrow::HBoxContainer>();
    if (record.type == "VBoxContainer") return std::make_shared<morrow::VBoxContainer>();
    if (record.type == "MarginContainer") return std::make_shared<morrow::MarginContainer>();
    if (record.type == "MRSplitContainer") return morrow::MRSplitContainer::create();
    if (record.type == "MRTabContainer") return morrow::MRTabContainer::create();
    if (record.type == "MRPopup") return morrow::MRPopup::create();
    if (record.type == "MRPopupPanel") return morrow::MRPopupPanel::create();
    if (record.type == "MRWindow") return morrow::MRWindow::create();
    if (record.type == "MRDialog") return morrow::MRDialog::create();
    if (record.type == "MRTooltip") return morrow::MRTooltip::create();
    if (record.type == "MRPopupMenu") return morrow::MRPopupMenu::create();
    if (record.type == "MR3DSceneView") return morrow::MR3DSceneView::create();
    if (record.type == "MRCanvasModulate") return morrow::MRCanvasModulate::create();
    if (record.type == "MRParallax2D") return morrow::MRParallax2D::create();
    if (record.type == "MRParallaxBackground") return morrow::MRParallaxBackground::create();
    if (record.type == "MRCPUParticles2D") return morrow::MRCPUParticles2D::create();
    if (record.type == "MRGPUParticles2D") return morrow::MRGPUParticles2D::create();
    if (record.type == "MRFrameAnimation") return morrow::MRFrameAnimation::create();
    if (record.type == "MRBounce") return morrow::MRBounce::create();
    if (record.type == "MRFlowingLight") return morrow::MRFlowingLight::create();
    if (record.type == "MRAnchorPointScale") return morrow::MRAnchorPointScale::create();
    if (record.type == "MRBrakePedal") return morrow::MRBrakePedal::create();
    if (record.type == "MRGearsIris") return morrow::MRGearsIris::create();
    if (record.type == "MRGearsOpening") return morrow::MRGearsOpening::create();
    if (record.type == "MRGearsSelect") return morrow::MRGearsSelect::create();
    if (record.type == "MRGearsShine") return morrow::MRGearsShine::create();
    if (record.type == "MRVideoStreamPlayer") {
        return morrow::MRVideoStreamPlayer::create(320, 180, 30.0f, 1);
    }
    if (record.type.rfind("MR", 0) == 0 ||
        record.type.find("Container") != std::string::npos) {
        return std::make_shared<CatalogPlaceholderWidget>(record.type);
    }
    error = "unsupported scene node type '" + record.type + "'";
    return nullptr;
}

bool applyProperty(const morrow::editor::SceneNodeRecord& record,
                   const std::shared_ptr<morrow::Widget>& widget,
                   const std::string& key,
                   const std::string& value,
                   const morrow::editor::SceneDocument& document,
                   const morrow::editor::AssetDatabase* assets,
                   std::string& error) {
    if (key == "visible") {
        bool visible = true;
        if (!parseBool(value, visible)) {
            error = "property 'visible' must be true or false";
            return false;
        }
        widget->setVisible(visible);
        return true;
    }
    if (key == "display_layer") {
        try {
            widget->setDisplayLayer(std::stoi(value));
        } catch (...) {
            error = "property 'display_layer' must be an integer";
            return false;
        }
        return true;
    }

    if (auto uiWidget = std::dynamic_pointer_cast<morrow::UIWidget>(widget)) {
        auto transform = uiWidget->getTransform();
        if (key == "position") {
            std::vector<float> components;
            if (!parseVector(value, "Vector3", components) || components.size() != 3) {
                error = "property 'position' must be Vector3(x, y, z)";
                return false;
            }
            transform->setPosition(Vector3(components[0], components[1], components[2]));
            return true;
        }
        if (key == "size") {
            std::vector<float> components;
            if (!parseVector(value, "Vector2", components) || components.size() != 2) {
                error = "property 'size' must be Vector2(width, height)";
                return false;
            }
            transform->setSize(Vector3(components[0], components[1], 0.0f));
            return true;
        }
        if (key == "scale") {
            std::vector<float> components;
            if (!parseVector(value, "Vector3", components) ||
                components.size() != 3) {
                error = "property 'scale' must be Vector3(x, y, z)";
                return false;
            }
            transform->setScale(
                components[0], components[1], components[2]);
            return true;
        }
        if (key == "rotation") {
            std::vector<float> components;
            if (!parseVector(value, "Vector3", components) ||
                components.size() != 3) {
                error = "property 'rotation' must be Vector3(x, y, z)";
                return false;
            }
            transform->setRotation(
                Vector3(0.0f, 0.0f, 1.0f),
                components[2] * 3.14159265359f / 180.0f);
            return true;
        }
    }

    if (auto button = std::dynamic_pointer_cast<morrow::MRButton>(widget)) {
        if (key == "text") {
            button->setText(toWide(value), "default");
            return true;
        }
        if (key == "font_size") {
            try {
                button->setTextFontSize(std::stof(value));
            } catch (...) {
                error = "property 'font_size' must be a number";
                return false;
            }
            return true;
        }
        if (key == "background_color") {
            std::vector<float> components;
            if (!parseVector(value, "Color", components) || components.size() != 4) {
                error = "property 'background_color' must be Color(r, g, b, a)";
                return false;
            }
            button->setBackgroundColor(Vector4(components[0], components[1], components[2], components[3]));
            return true;
        }
        if (key == "background_texture") {
            constexpr const char* prefix = "resource(\"";
            if (value.compare(0, std::strlen(prefix), prefix) != 0 ||
                value.size() <= std::strlen(prefix) + 2 ||
                value.back() != ')') {
                error = "property 'background_texture' must reference resource(\"id\")";
                return false;
            }
            const auto localId = value.substr(
                std::strlen(prefix), value.size() - std::strlen(prefix) - 2);
            const auto resource = document.findExternalResource(localId);
            if (!resource) {
                error = "external resource '" + localId + "' was not found";
                return false;
            }
            if (!assets || resource->assetId.empty()) {
                error = "external resource '" + localId +
                        "' has no resolvable asset_id";
                return false;
            }
            const auto asset = assets->findById(resource->assetId);
            if (!asset || asset->type != "Texture") {
                error = "texture asset '" + resource->assetId + "' was not found";
                return false;
            }
            auto texture = morrow::Texture::create();
            texture->setImageUrl(assets->resolveSourcePath(asset->assetId).string());
            button->setBackgroundImage(texture);
            return true;
        }
        if (key == "style") {
            constexpr const char* prefix = "sub_resource(\"";
            if (value.compare(0, std::strlen(prefix), prefix) != 0 ||
                value.size() <= std::strlen(prefix) + 2 ||
                value.back() != ')') {
                error = "property 'style' must reference sub_resource(\"id\")";
                return false;
            }
            const auto styleId = value.substr(
                std::strlen(prefix), value.size() - std::strlen(prefix) - 2);
            const auto style = document.findSubResource(styleId);
            if (!style) {
                error = "sub_resource '" + styleId + "' was not found";
                return false;
            }
            for (const auto& [styleKey, styleValue] : style->properties) {
                if (styleKey == "corner_radius") {
                    button->setCornerRadius(std::stof(styleValue));
                } else if (styleKey == "background_color") {
                    std::vector<float> components;
                    if (!parseVector(styleValue, "Color", components) ||
                        components.size() != 4) {
                        error = "style background_color must be Color(r, g, b, a)";
                        return false;
                    }
                    button->setBackgroundColor(Vector4(
                        components[0], components[1],
                        components[2], components[3]));
                }
            }
            return true;
        }
    }

    if (auto image = std::dynamic_pointer_cast<morrow::MRImage>(widget)) {
        if (key == "texture_asset") {
            if (value.empty())
                return true;
            if (!assets) {
                error = "texture_asset requires an asset database";
                return false;
            }
            const auto* asset = assets->findById(value);
            if (!asset || asset->type != "Texture") {
                error = "texture asset '" + value + "' was not found";
                return false;
            }
            auto texture = morrow::Texture::create();
            texture->setImageUrl(
                assets->resolveSourcePath(value).string());
            image->setTexture(texture);
            return true;
        }
    }

    if (auto label = std::dynamic_pointer_cast<morrow::MRLabel>(widget)) {
        if (key == "text") {
            label->setText(toWide(value), "default");
            return true;
        }
        if (key == "font_size") {
            try {
                label->setFontSize(std::stof(value));
            } catch (...) {
                error = "property 'font_size' must be a number";
                return false;
            }
            return true;
        }
    }

    error = "unsupported property '" + key + "' on node '" + record.type + "'";
    return false;
}

}  // namespace

namespace morrow::editor {

bool SceneInstantiator::instantiate(const SceneDocument& document,
                                    const std::shared_ptr<Widget>& stage,
                                    const AssetDatabase* assets,
                                    std::string& error,
                                    SceneInstanceMap* instancesOut) {
    if (!stage) {
        error = "cannot instantiate scene without a stage widget";
        return false;
    }

    std::unordered_map<std::string, std::shared_ptr<Widget>> instances;
    instances.reserve(document.nodes().size());
    for (const auto& record : document.nodes()) {
        auto instance = createNode(record, error);
        if (!instance) {
            error = "line " + std::to_string(record.line) + ": " + error;
            return false;
        }
        instance->setWidgetName(record.name);
        if (!updateNode(
                document, record, instance, assets, error)) {
            error = "line " + std::to_string(record.line) + ": " + error;
            return false;
        }
        instances.emplace(record.id, std::move(instance));
    }

    for (const auto& record : document.nodes()) {
        const auto instance = instances.at(record.id);
        if (record.parentId.empty()) {
            stage->addChild(instance);
            continue;
        }
        const auto parent = instances.find(record.parentId);
        if (parent == instances.end()) {
            error = "line " + std::to_string(record.line) + ": parent node '" + record.parentId + "' was not found";
            return false;
        }
        parent->second->addChild(instance);
    }
    if (instancesOut)
        *instancesOut = instances;
    return true;
}

bool SceneInstantiator::updateNode(
    const SceneDocument& document,
    const SceneNodeRecord& record,
    const std::shared_ptr<Widget>& instance,
    const AssetDatabase* assets,
    std::string& error) {
    if (!instance) {
        error = "cannot update a null scene instance";
        return false;
    }
    instance->setWidgetName(record.name);
    for (const auto& [key, value] : record.properties) {
        if (!applyProperty(
                record, instance, key, value,
                document, assets, error)) {
            return false;
        }
    }
    return true;
}

bool SceneInstantiator::updateNodeTransform(
    const SceneNodeRecord& record,
    const std::shared_ptr<Widget>& instance,
    std::string& error) {
    if (!instance) {
        error = "cannot update a null scene instance";
        return false;
    }
    static const std::vector<std::string> properties = {
        "position", "size", "scale", "rotation",
        "visible", "display_layer"};
    SceneDocument emptyDocument;
    for (const auto& key : properties) {
        const auto value = record.properties.find(key);
        if (value == record.properties.end())
            continue;
        if (!applyProperty(
                record, instance, key, value->second,
                emptyDocument, nullptr, error)) {
            return false;
        }
    }
    return true;
}

}  // namespace morrow::editor
