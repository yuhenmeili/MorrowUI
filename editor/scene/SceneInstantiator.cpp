#include "SceneInstantiator.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "SceneDocument.h"
#include "Vector3.h"
#include "Vector4.h"
#include "assets/AssetDatabase.h"
#include "assets/MaterialAsset.h"
#include "assets/ThemeAsset.h"
#include "morrow/base/Transform.h"
#include "morrow/base/MeshRenderer.h"
#include "morrow/base/UIWidget.h"
#include "morrow/elements/MR3DSceneView.h"
#include "morrow/elements/MRAnchorPointScale.h"
#include "morrow/elements/MRBounce.h"
#include "morrow/elements/MRBrakePedal.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRCanvasModulate.h"
#include "morrow/elements/MRCheckBox.h"
#include "morrow/elements/MRCheckButton.h"
#include "morrow/elements/MRColor.h"
#include "morrow/elements/MRFlowingLight.h"
#include "morrow/elements/MRFrameAnimation.h"
#include "morrow/elements/MRGearsIris.h"
#include "morrow/elements/MRGearsOpening.h"
#include "morrow/elements/MRGearsSelect.h"
#include "morrow/elements/MRGearsShine.h"
#include "morrow/elements/MRImage.h"
#include "morrow/elements/MRItemList.h"
#include "morrow/elements/MRLabel.h"
#include "morrow/elements/MRLineEdit.h"
#include "morrow/elements/MRMenuButton.h"
#include "morrow/elements/MRNinePatch.h"
#include "morrow/elements/MROptionButton.h"
#include "morrow/elements/MRParallax2D.h"
#include "morrow/elements/MRParallaxBackground.h"
#include "morrow/elements/MRParticles2D.h"
#include "morrow/elements/MRProgressBar.h"
#include "morrow/elements/MRScrollContainer.h"
#include "morrow/elements/MRSelectableButton.h"
#include "morrow/elements/MRSegmentedButton.h"
#include "morrow/elements/MRSpacer.h"
#include "morrow/elements/MRSpinBox.h"
#include "morrow/elements/MRTextEdit.h"
#include "morrow/layout/HBoxContainer.h"
#include "morrow/layout/GridContainer.h"
#include "morrow/layout/MarginContainer.h"
#include "morrow/layout/MRSplitContainer.h"
#include "morrow/layout/VBoxContainer.h"
#include "morrow/elements/MRPopup.h"
#include "morrow/elements/MRPopupMenu.h"
#include "morrow/elements/MRRadioButton.h"
#include "morrow/elements/MRRichTextLabel.h"
#include "morrow/elements/MRScrollBar.h"
#include "morrow/elements/MRSeparator.h"
#include "morrow/elements/MRSlider.h"
#include "morrow/elements/MRTextureButton.h"
#include "morrow/elements/MRToggle.h"
#include "morrow/elements/MRTree.h"
#include "morrow/elements/MRVideoStreamPlayer.h"
#include "morrow/layout/CenterContainer.h"
#include "morrow/layout/MRTabContainer.h"

namespace {

using morrow::Math::Vector3;
using morrow::Math::Vector4;

class SceneContainer2D final : public morrow::UIWidget {
public:
    SceneContainer2D() : UIWidget(false) {
        setWidgetType("SceneNode3D");
    }
};

class CatalogPlaceholderWidget final : public morrow::UIWidget {
public:
    explicit CatalogPlaceholderWidget(const std::string& type) : UIWidget(false) {
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

/// 控件属性通用解析辅助（5.11 属性覆盖面）。
bool parseFloatInto(const std::string& value, float& result) {
    try {
        result = std::stof(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseIntInto(const std::string& value, int& result) {
    try {
        result = std::stoi(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseColorInto(const std::string& value, Vector4& color) {
    std::vector<float> components;
    if (!parseVector(value, "Color", components) || components.size() != 4)
        return false;
    color = Vector4(components[0], components[1], components[2], components[3]);
    return true;
}

bool parseHorizontalAlign(const std::string& value, HorizontalAlignment& align) {
    if (value == "left")
        align = HorizontalAlignment::LEFT;
    else if (value == "center")
        align = HorizontalAlignment::CENTER;
    else if (value == "right")
        align = HorizontalAlignment::RIGHT;
    else
        return false;
    return true;
}

bool parseVerticalAlign(const std::string& value, VerticalAlignment& align) {
    if (value == "top")
        align = VerticalAlignment::TOP;
    else if (value == "center")
        align = VerticalAlignment::CENTER;
    else if (value == "bottom")
        align = VerticalAlignment::BOTTOM;
    else
        return false;
    return true;
}

bool parseProgressDirection(const std::string& value, morrow::ProgressDirection& direction) {
    if (value == "left_to_right")
        direction = morrow::ProgressDirection::LeftToRight;
    else if (value == "right_to_left")
        direction = morrow::ProgressDirection::RightToLeft;
    else if (value == "bottom_to_top")
        direction = morrow::ProgressDirection::BottomToTop;
    else if (value == "top_to_bottom")
        direction = morrow::ProgressDirection::TopToBottom;
    else
        return false;
    return true;
}

bool parseSplitOrientation(const std::string& value, morrow::SplitOrientation& orientation) {
    if (value == "horizontal")
        orientation = morrow::SplitOrientation::Horizontal;
    else if (value == "vertical")
        orientation = morrow::SplitOrientation::Vertical;
    else
        return false;
    return true;
}

/// 读取节点上另一枚举键的当前值（组合 setter 用，如 setAlign 需要同时两轴）。
std::string siblingPropertyValue(const morrow::editor::SceneNodeRecord& record, const std::string& key, const std::string& fallback) {
    const auto entry = record.properties.find(key);
    return entry == record.properties.end() ? fallback : entry->second;
}

std::wstring toWide(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

std::shared_ptr<morrow::Widget> createNode(const morrow::editor::SceneNodeRecord& record, std::string& error) {
    if (record.type == "SceneNode3D") {
        // Phase 1 scenes are 2D editor documents. Use a non-rendering 2D
        // container so UI descendants retain the normal Transform chain.
        return std::make_shared<SceneContainer2D>();
    }
    if (record.type == "MRSceneInstance") {
        // 场景实例：非渲染组节点，子场景内容在 instantiateSubScene 中挂载
        return std::make_shared<SceneContainer2D>();
    }
    if (record.type == "MRAnimationPlayer") {
        // 动画播放器：非渲染组节点，播放逻辑由 AnimationService 驱动
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
    if (record.type == "MRColor")
        return morrow::MRColor::create();
    if (record.type == "MRNinePatch")
        return morrow::MRNinePatch::create();
    if (record.type == "MRCheckBox")
        return morrow::MRCheckBox::create();
    if (record.type == "MRCheckButton")
        return morrow::MRCheckButton::create();
    if (record.type == "MRRadioButton")
        return morrow::MRRadioButton::create();
    if (record.type == "MRRadioGroup")
        return morrow::MRRadioGroup::create();
    if (record.type == "MRToggle")
        return morrow::MRToggle::create();
    if (record.type == "MRLineEdit")
        return morrow::MRLineEdit::create();
    if (record.type == "MRTextEdit")
        return morrow::MRTextEdit::create();
    if (record.type == "MRSpinBox")
        return morrow::MRSpinBox::create();
    if (record.type == "MRSlider")
        return morrow::MRSlider::create();
    if (record.type == "MRMenuButton")
        return morrow::MRMenuButton::create();
    if (record.type == "MROptionButton")
        return morrow::MROptionButton::create();
    if (record.type == "MRTextureButton")
        return morrow::MRTextureButton::create();
    if (record.type == "MRProgressBar")
        return morrow::MRProgressBar::create();
    if (record.type == "MRItemList")
        return morrow::MRItemList::create();
    if (record.type == "MRTree")
        return morrow::MRTree::create();
    if (record.type == "MRRichTextLabel")
        return morrow::MRRichTextLabel::create();
    if (record.type == "MRScrollBar")
        return morrow::MRScrollBar::create();
    if (record.type == "MRScrollContainer")
        return morrow::MRScrollContainer::create();
    if (record.type == "MRSpacer")
        return morrow::MRSpacer::create();
    if (record.type == "MRHSeparator")
        return morrow::MRHSeparator::create();
    if (record.type == "MRVSeparator")
        return morrow::MRVSeparator::create();
    if (record.type == "CenterContainer")
        return std::make_shared<morrow::CenterContainer>();
    if (record.type == "HBoxContainer")
        return std::make_shared<morrow::HBoxContainer>();
    if (record.type == "VBoxContainer")
        return std::make_shared<morrow::VBoxContainer>();
    if (record.type == "GridContainer")
        return std::make_shared<morrow::GridContainer>();
    if (record.type == "MarginContainer")
        return std::make_shared<morrow::MarginContainer>();
    if (record.type == "MRSegmentedButton")
        return morrow::MRSegmentedButton::create();
    if (record.type == "MRSplitContainer")
        return morrow::MRSplitContainer::create();
    if (record.type == "MRTabContainer")
        return morrow::MRTabContainer::create();
    if (record.type == "MRPopup")
        return morrow::MRPopup::create();
    if (record.type == "MRPopupPanel")
        return morrow::MRPopupPanel::create();
    if (record.type == "MRWindow")
        return morrow::MRWindow::create();
    if (record.type == "MRDialog")
        return morrow::MRDialog::create();
    if (record.type == "MRTooltip")
        return morrow::MRTooltip::create();
    if (record.type == "MRPopupMenu")
        return morrow::MRPopupMenu::create();
    if (record.type == "MR3DSceneView")
        return morrow::MR3DSceneView::create();
    if (record.type == "MRCanvasModulate")
        return morrow::MRCanvasModulate::create();
    if (record.type == "MRParallax2D")
        return morrow::MRParallax2D::create();
    if (record.type == "MRParallaxBackground")
        return morrow::MRParallaxBackground::create();
    if (record.type == "MRCPUParticles2D")
        return morrow::MRCPUParticles2D::create();
    if (record.type == "MRGPUParticles2D")
        return morrow::MRGPUParticles2D::create();
    if (record.type == "MRFrameAnimation")
        return morrow::MRFrameAnimation::create();
    if (record.type == "MRBounce")
        return morrow::MRBounce::create();
    if (record.type == "MRFlowingLight")
        return morrow::MRFlowingLight::create();
    if (record.type == "MRAnchorPointScale")
        return morrow::MRAnchorPointScale::create();
    if (record.type == "MRBrakePedal")
        return morrow::MRBrakePedal::create();
    if (record.type == "MRGearsIris")
        return morrow::MRGearsIris::create();
    if (record.type == "MRGearsOpening")
        return morrow::MRGearsOpening::create();
    if (record.type == "MRGearsSelect")
        return morrow::MRGearsSelect::create();
    if (record.type == "MRGearsShine")
        return morrow::MRGearsShine::create();
    if (record.type == "MRVideoStreamPlayer") {
        return morrow::MRVideoStreamPlayer::create(320, 180, 30.0f, 1);
    }
    if (record.type.rfind("MR", 0) == 0 || record.type.find("Container") != std::string::npos) {
        return std::make_shared<CatalogPlaceholderWidget>(record.type);
    }
    error = "unsupported scene node type '" + record.type + "'";
    return nullptr;
}

bool applyProperty(const morrow::editor::SceneNodeRecord& record, const std::shared_ptr<morrow::Widget>& widget, const std::string& key, const std::string& value,
                   const morrow::editor::SceneDocument& document, const morrow::editor::AssetDatabase* assets, std::string& error) {
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
            if (!parseVector(value, "Vector3", components) || components.size() != 3) {
                error = "property 'scale' must be Vector3(x, y, z)";
                return false;
            }
            transform->setScale(components[0], components[1], components[2]);
            return true;
        }
        if (key == "rotation") {
            std::vector<float> components;
            if (!parseVector(value, "Vector3", components) || components.size() != 3) {
                error = "property 'rotation' must be Vector3(x, y, z)";
                return false;
            }
            transform->setRotation(Vector3(0.0f, 0.0f, 1.0f), components[2] * 3.14159265359f / 180.0f);
            return true;
        }
        if (key == "renderer_enabled") {
            bool enabled = true;
            if (!parseBool(value, enabled)) {
                error = "property 'renderer_enabled' must be true or false";
                return false;
            }
            if (auto renderer = uiWidget->getComponent<morrow::MeshRenderer>())
                renderer->setEnabled(enabled);
            return true;
        }
        if (key == "material_color") {
            std::vector<float> components;
            if (!parseVector(value, "Color", components) || components.size() != 4) {
                error = "property 'material_color' must be Color(r, g, b, a)";
                return false;
            }
            if (auto renderer = uiWidget->getComponent<morrow::MeshRenderer>()) {
                if (auto material = renderer->getMaterial())
                    material->setVector("color", Vector4(components[0], components[1], components[2], components[3]));
            }
            return true;
        }
        if (key == "blend_enabled" || key == "double_sided") {
            bool enabled = false;
            if (!parseBool(value, enabled)) {
                error = "property '" + key + "' must be true or false";
                return false;
            }
            if (auto renderer = uiWidget->getComponent<morrow::MeshRenderer>()) {
                if (auto material = renderer->getMaterial()) {
                    if (key == "blend_enabled")
                        material->setBlendEnabled(enabled);
                    else
                        material->setDoubleSided(enabled);
                }
            }
            return true;
        }
        if (key == "material") {
            if (value.empty())
                return true;
            if (!assets) {
                error = "material property requires an asset database";
                return false;
            }
            const auto* asset = assets->findById(value);
            if (!asset || asset->type != "Material") {
                error = "material asset '" + value + "' was not found";
                return false;
            }
            morrow::editor::MaterialAsset materialAsset;
            if (!morrow::editor::loadMaterialAsset(assets->resolveSourcePath(value), materialAsset, error))
                return false;
            auto renderer = uiWidget->getComponent<morrow::MeshRenderer>();
            if (!renderer)
                return true;
            auto material = morrow::Material::create(materialAsset.shader);
            for (const auto& [propertyName, propertyValue] : materialAsset.properties) {
                if (propertyValue.rfind("Color(", 0) == 0) {
                    std::vector<float> components;
                    if (parseVector(propertyValue, "Color", components) && components.size() == 4)
                        material->setVector(propertyName, Vector4(components[0], components[1], components[2], components[3]));
                } else {
                    try {
                        material->setFloat(propertyName, std::stof(propertyValue));
                    } catch (...) {
                    }
                }
            }
            renderer->setMaterial(material);
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
        if (key == "hover_color" || key == "pressed_color" || key == "text_color" || key == "border_color") {
            Vector4 color;
            if (!parseColorInto(value, color)) {
                error = "property '" + key + "' must be Color(r, g, b, a)";
                return false;
            }
            if (key == "hover_color")
                button->setHoverColor(color);
            else if (key == "pressed_color")
                button->setPressedColor(color);
            else if (key == "text_color")
                button->setTextColor(color);
            else
                button->setBorderColor(color);
            return true;
        }
        if (key == "corner_radius") {
            float radius = 0.0f;
            if (!parseFloatInto(value, radius)) {
                error = "property 'corner_radius' must be a number";
                return false;
            }
            button->setCornerRadius(radius);
            return true;
        }
        if (key == "border_width") {
            float width = 0.0f;
            if (!parseFloatInto(value, width)) {
                error = "property 'border_width' must be a number";
                return false;
            }
            button->setBorderWidth(width);
            return true;
        }
        if (key == "text_align_h" || key == "text_align_v") {
            HorizontalAlignment horizontal = HorizontalAlignment::CENTER;
            VerticalAlignment vertical = VerticalAlignment::CENTER;
            bool parsed = true;
            if (key == "text_align_h") {
                parsed = parseHorizontalAlign(value, horizontal);
                parseVerticalAlign(siblingPropertyValue(record, "text_align_v", "center"), vertical);
            } else {
                parsed = parseVerticalAlign(value, vertical);
                parseHorizontalAlign(siblingPropertyValue(record, "text_align_h", "center"), horizontal);
            }
            if (!parsed) {
                error = "property '" + key + "' must be left/center/right or top/center/bottom";
                return false;
            }
            button->setTextAlign(horizontal, vertical);
            return true;
        }
        if (key == "auto_wrap") {
            bool enabled = false;
            if (!parseBool(value, enabled)) {
                error = "property 'auto_wrap' must be true or false";
                return false;
            }
            button->setAutoWrap(enabled);
            return true;
        }
        if (key == "background_texture") {
            constexpr const char* prefix = "resource(\"";
            if (value.compare(0, std::strlen(prefix), prefix) != 0 || value.size() <= std::strlen(prefix) + 2 || value.back() != ')') {
                error = "property 'background_texture' must reference resource(\"id\")";
                return false;
            }
            const auto localId = value.substr(std::strlen(prefix), value.size() - std::strlen(prefix) - 2);
            const auto resource = document.findExternalResource(localId);
            if (!resource) {
                error = "external resource '" + localId + "' was not found";
                return false;
            }
            if (!assets || resource->assetId.empty()) {
                error = "external resource '" + localId + "' has no resolvable asset_id";
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
            if (value.compare(0, std::strlen(prefix), prefix) != 0 || value.size() <= std::strlen(prefix) + 2 || value.back() != ')') {
                error = "property 'style' must reference sub_resource(\"id\")";
                return false;
            }
            const auto styleId = value.substr(std::strlen(prefix), value.size() - std::strlen(prefix) - 2);
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
                    if (!parseVector(styleValue, "Color", components) || components.size() != 4) {
                        error = "style background_color must be Color(r, g, b, a)";
                        return false;
                    }
                    button->setBackgroundColor(Vector4(components[0], components[1], components[2], components[3]));
                }
            }
            return true;
        }
    }

    if (auto ninePatch = std::dynamic_pointer_cast<morrow::MRNinePatch>(widget)) {
        if (key == "texture_asset") {
            if (value.empty() || !assets) {
                ninePatch->setTexture(nullptr);
                return true;
            }
            const auto* asset = assets->findById(value);
            if (!asset || asset->type != "Texture" || !asset->error.empty()) {
                ninePatch->setTexture(nullptr);
                return true;
            }
            auto texture = morrow::Texture::create();
            texture->setImageUrl(assets->resolveSourcePath(value).string());
            ninePatch->setTexture(texture);
            return true;
        }
        if (key == "inset") {
            float inset = 12.0f;
            if (!parseFloatInto(value, inset)) {
                error = "property 'inset' must be a number";
                return false;
            }
            ninePatch->setInset(inset);
            return true;
        }
    }

    if (auto image = std::dynamic_pointer_cast<morrow::MRImage>(widget)) {
        if (key == "texture_asset") {
            if (value.empty()) {
                image->setTexture(nullptr);
                return true;
            }
            if (!assets) {
                image->setTexture(nullptr);
                return true;
            }
            const auto* asset = assets->findById(value);
            if (!asset || asset->type != "Texture" || !asset->error.empty()) {
                image->setTexture(nullptr);
                return true;
            }
            auto texture = morrow::Texture::create();
            texture->setImageUrl(assets->resolveSourcePath(value).string());
            image->setTexture(texture);
            return true;
        }
        if (key == "rounding") {
            float rounding = 0.0f;
            if (!parseFloatInto(value, rounding)) {
                error = "property 'rounding' must be a number";
                return false;
            }
            image->setRounding(rounding);
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
        if (key == "font_color") {
            Vector4 color;
            if (!parseColorInto(value, color)) {
                error = "property 'font_color' must be Color(r, g, b, a)";
                return false;
            }
            label->setFontColor(color);
            return true;
        }
        if (key == "align_h" || key == "align_v") {
            HorizontalAlignment horizontal = HorizontalAlignment::LEFT;
            VerticalAlignment vertical = VerticalAlignment::TOP;
            bool parsed = true;
            if (key == "align_h") {
                parsed = parseHorizontalAlign(value, horizontal);
                parseVerticalAlign(siblingPropertyValue(record, "align_v", "top"), vertical);
            } else {
                parsed = parseVerticalAlign(value, vertical);
                parseHorizontalAlign(siblingPropertyValue(record, "align_h", "left"), horizontal);
            }
            if (!parsed) {
                error = "property '" + key + "' must be left/center/right or top/center/bottom";
                return false;
            }
            label->setAlign(horizontal, vertical);
            return true;
        }
        if (key == "auto_wrap") {
            bool enabled = false;
            if (!parseBool(value, enabled)) {
                error = "property 'auto_wrap' must be true or false";
                return false;
            }
            label->setAutoWrap(enabled);
            return true;
        }
        if (key == "max_lines") {
            int maxLines = 0;
            if (!parseIntInto(value, maxLines)) {
                error = "property 'max_lines' must be an integer";
                return false;
            }
            label->setMaxLines(maxLines);
            return true;
        }
        if (key == "line_spacing") {
            float spacing = 1.0f;
            if (!parseFloatInto(value, spacing)) {
                error = "property 'line_spacing' must be a number";
                return false;
            }
            label->setLineSpacing(spacing);
            return true;
        }
    }

    // ---- 5.11 属性覆盖面：按控件类型分发的外观/行为属性 ----
    if (auto selectable = std::dynamic_pointer_cast<morrow::MRSelectableButton>(widget)) {
        if (key == "checked") {
            bool checked = false;
            if (!parseBool(value, checked)) {
                error = "property 'checked' must be true or false";
                return false;
            }
            selectable->setChecked(checked);
            return true;
        }
    }

    if (auto progress = std::dynamic_pointer_cast<morrow::MRProgressBar>(widget)) {
        if (key == "progress") {
            float ratio = 0.0f;
            if (!parseFloatInto(value, ratio)) {
                error = "property 'progress' must be a number in 0..1";
                return false;
            }
            progress->setProgress(std::clamp(ratio, 0.0f, 1.0f));
            return true;
        }
        if (key == "direction") {
            morrow::ProgressDirection direction = morrow::ProgressDirection::LeftToRight;
            if (!parseProgressDirection(value, direction)) {
                error = "property 'direction' is not a known progress direction";
                return false;
            }
            progress->setDirection(direction);
            return true;
        }
        if (key == "track_color" || key == "fill_color") {
            Vector4 color;
            if (!parseColorInto(value, color)) {
                error = "property '" + key + "' must be Color(r, g, b, a)";
                return false;
            }
            if (key == "track_color")
                progress->setTrackColor(color);
            else
                progress->setFillColor(color);
            return true;
        }
    }

    if (auto spinBox = std::dynamic_pointer_cast<morrow::MRSpinBox>(widget)) {
        if (key == "value" || key == "step") {
            float number = 0.0f;
            if (!parseFloatInto(value, number)) {
                error = "property '" + key + "' must be a number";
                return false;
            }
            if (key == "value")
                spinBox->setValue(static_cast<double>(number));
            else
                spinBox->setStep(static_cast<double>(number));
            return true;
        }
        if (key == "min" || key == "max") {
            // min/max 组合生效：两键任一变化都按当前文档值重建区间
            float minimum = 0.0f;
            float maximum = 100.0f;
            const auto minEntry = record.properties.find("min");
            const auto maxEntry = record.properties.find("max");
            if (minEntry != record.properties.end() && !parseFloatInto(minEntry->second, minimum)) {
                error = "property 'min' must be a number";
                return false;
            }
            if (maxEntry != record.properties.end() && !parseFloatInto(maxEntry->second, maximum)) {
                error = "property 'max' must be a number";
                return false;
            }
            if (minimum > maximum)
                std::swap(minimum, maximum);
            spinBox->setRange(static_cast<double>(minimum), static_cast<double>(maximum));
            return true;
        }
        if (key == "decimals") {
            int decimals = 0;
            if (!parseIntInto(value, decimals)) {
                error = "property 'decimals' must be an integer";
                return false;
            }
            spinBox->setDecimals(decimals);
            return true;
        }
        if (key == "prefix" || key == "suffix") {
            if (key == "prefix")
                spinBox->setPrefix(toWide(value));
            else
                spinBox->setSuffix(toWide(value));
            return true;
        }
    }

    if (auto textEdit = std::dynamic_pointer_cast<morrow::MRTextEdit>(widget)) {
        if (key == "text") {
            textEdit->setText(toWide(value));
            return true;
        }
        if (key == "placeholder") {
            textEdit->setPlaceholder(toWide(value));
            return true;
        }
        if (key == "read_only") {
            bool readOnly = false;
            if (!parseBool(value, readOnly)) {
                error = "property 'read_only' must be true or false";
                return false;
            }
            textEdit->setReadOnly(readOnly);
            return true;
        }
        if (key == "auto_wrap") {
            bool enabled = false;
            if (!parseBool(value, enabled)) {
                error = "property 'auto_wrap' must be true or false";
                return false;
            }
            textEdit->setAutoWrap(enabled);
            return true;
        }
    }

    if (auto lineEdit = std::dynamic_pointer_cast<morrow::MRLineEdit>(widget)) {
        if (key == "password_mode") {
            bool enabled = false;
            if (!parseBool(value, enabled)) {
                error = "property 'password_mode' must be true or false";
                return false;
            }
            lineEdit->setPasswordMode(enabled);
            return true;
        }
    }

    if (auto scrollContainer = std::dynamic_pointer_cast<morrow::MRScrollContainer>(widget)) {
        if (key == "scroll_step") {
            float step = 48.0f;
            if (!parseFloatInto(value, step)) {
                error = "property 'scroll_step' must be a number";
                return false;
            }
            scrollContainer->setScrollStep(step);
            return true;
        }
    }

    if (auto colorWidget = std::dynamic_pointer_cast<morrow::MRColor>(widget)) {
        if (key == "color") {
            Vector4 color;
            if (!parseColorInto(value, color)) {
                error = "property 'color' must be Color(r, g, b, a)";
                return false;
            }
            colorWidget->setColor(color);
            return true;
        }
        if (key == "rounding") {
            float rounding = 0.0f;
            if (!parseFloatInto(value, rounding)) {
                error = "property 'rounding' must be a number";
                return false;
            }
            colorWidget->setRounding(rounding);
            return true;
        }
    }

    if (auto hbox = std::dynamic_pointer_cast<morrow::HBoxContainer>(widget)) {
        if (key == "spacing") {
            float spacing = 0.0f;
            if (!parseFloatInto(value, spacing)) {
                error = "property 'spacing' must be a number";
                return false;
            }
            hbox->setSpacing(spacing);
            return true;
        }
    }

    if (auto vbox = std::dynamic_pointer_cast<morrow::VBoxContainer>(widget)) {
        if (key == "spacing") {
            float spacing = 0.0f;
            if (!parseFloatInto(value, spacing)) {
                error = "property 'spacing' must be a number";
                return false;
            }
            vbox->setSpacing(spacing);
            return true;
        }
    }

    if (auto grid = std::dynamic_pointer_cast<morrow::GridContainer>(widget)) {
        if (key == "columns" || key == "rows") {
            int number = 0;
            if (!parseIntInto(value, number)) {
                error = "property '" + key + "' must be an integer";
                return false;
            }
            if (key == "columns")
                grid->setColumns(number);
            else
                grid->setRows(number);
            return true;
        }
        if (key == "spacing") {
            float spacing = 0.0f;
            if (!parseFloatInto(value, spacing)) {
                error = "property 'spacing' must be a number";
                return false;
            }
            grid->setSpacing(spacing);
            return true;
        }
    }

    if (auto marginContainer = std::dynamic_pointer_cast<morrow::MarginContainer>(widget)) {
        if (key == "margin_left" || key == "margin_top" || key == "margin_right" || key == "margin_bottom") {
            float number = 0.0f;
            if (!parseFloatInto(value, number)) {
                error = "property '" + key + "' must be a number";
                return false;
            }
            if (key == "margin_left")
                marginContainer->setMarginLeft(number);
            else if (key == "margin_top")
                marginContainer->setMarginTop(number);
            else if (key == "margin_right")
                marginContainer->setMarginRight(number);
            else
                marginContainer->setMarginBottom(number);
            return true;
        }
    }

    if (auto split = std::dynamic_pointer_cast<morrow::MRSplitContainer>(widget)) {
        if (key == "split_ratio") {
            float ratio = 0.5f;
            if (!parseFloatInto(value, ratio)) {
                error = "property 'split_ratio' must be a number";
                return false;
            }
            split->setSplitRatio(std::clamp(ratio, 0.0f, 1.0f));
            return true;
        }
        if (key == "handle_width") {
            float width = 0.0f;
            if (!parseFloatInto(value, width)) {
                error = "property 'handle_width' must be a number";
                return false;
            }
            split->setHandleWidth(width);
            return true;
        }
        if (key == "orientation") {
            morrow::SplitOrientation orientation = morrow::SplitOrientation::Horizontal;
            if (!parseSplitOrientation(value, orientation)) {
                error = "property 'orientation' must be horizontal or vertical";
                return false;
            }
            split->setOrientation(orientation);
            return true;
        }
    }

    if (auto spacer = std::dynamic_pointer_cast<morrow::MRSpacer>(widget)) {
        if (key == "flex") {
            float flex = 1.0f;
            if (!parseFloatInto(value, flex)) {
                error = "property 'flex' must be a number";
                return false;
            }
            spacer->setFlex(flex);
            return true;
        }
    }

    // 编辑器元数据只进文档不进运行时（锁定标记等），静默跳过避免误报 unsupported
    if (key == "locked" || key == "animation_id" || key == "autoplay" || key == "speed") {
        return true;
    }
    // 锚点三键由 updateNode 的 applyAnchors 组合应用，逐键循环里跳过
    if (key == "anchor_min" || key == "anchor_max" || key == "anchor_offset") {
        return true;
    }

    error = "unsupported property '" + key + "' on node '" + record.type + "'";
    return false;
}

/// 组合应用锚点：anchor_min + anchor_max 同时存在才启用，缺失则显式关闭。
void applyAnchors(const morrow::editor::SceneNodeRecord& record, const std::shared_ptr<morrow::Widget>& instance) {
    auto uiWidget = std::dynamic_pointer_cast<morrow::UIWidget>(instance);
    if (!uiWidget)
        return;
    const auto transform = uiWidget->getTransform();
    const auto& properties = record.properties;
    const auto minEntry = properties.find("anchor_min");
    const auto maxEntry = properties.find("anchor_max");
    if (minEntry == properties.end() || maxEntry == properties.end()) {
        transform->setAnchorEnabled(false);
        return;
    }
    std::vector<float> minComponents;
    std::vector<float> maxComponents;
    if (!parseVector(minEntry->second, "Vector2", minComponents) || minComponents.size() != 2 || !parseVector(maxEntry->second, "Vector2", maxComponents) ||
        maxComponents.size() != 2) {
        transform->setAnchorEnabled(false);
        return;
    }
    transform->setAnchor(minComponents[0], minComponents[1], maxComponents[0], maxComponents[1]);
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    const auto offsetEntry = properties.find("anchor_offset");
    if (offsetEntry != properties.end()) {
        std::vector<float> offsets;
        if (parseVector(offsetEntry->second, "Vector4", offsets) && offsets.size() == 4) {
            left = offsets[0];
            top = offsets[1];
            right = offsets[2];
            bottom = offsets[3];
        }
    }
    transform->setAnchorOffset(left, top, right, bottom);
    transform->setAnchorEnabled(true);
    // 已挂父级时立即生效（live 更新路径）；instantiate 阶段挂父后由帧循环应用
    transform->updateAnchoredPosition();
}

}  // namespace

namespace morrow::editor {

/// 场景实例：MRSceneInstance 节点按 scene_path（相对父场景文件目录）加载子场景
/// 并整体实例化到该组节点下；实例链阻止递归引用，主题向子场景传递。
bool instantiateSubScene(const SceneDocument& parentDocument, const SceneNodeRecord& record, const std::shared_ptr<Widget>& groupWidget, const AssetDatabase* assets,
                         std::string& error, const std::vector<std::filesystem::path>& instanceChain, const ThemeAsset* theme) {
    const auto scenePathEntry = record.properties.find("scene_path");
    if (scenePathEntry == record.properties.end() || scenePathEntry->second.empty())
        return true;  // 未配置子场景的实例节点保持为空组
    const std::filesystem::path baseDirectory = parentDocument.sourcePath().empty() ? std::filesystem::current_path() : parentDocument.sourcePath().parent_path();
    std::filesystem::path subPath = (baseDirectory / scenePathEntry->second).lexically_normal();
    if (subPath.is_relative())
        subPath = (std::filesystem::current_path() / subPath).lexically_normal();
    if (!std::filesystem::exists(subPath)) {
        error = "scene instance '" + record.name + "' references missing scene: " + scenePathEntry->second;
        return false;
    }
    for (const auto& active : instanceChain) {
        if (active == subPath) {
            error = "recursive scene instance detected at: " + subPath.string();
            return false;
        }
    }
    SceneDocument subDocument;
    if (!SceneDocument::loadFromFile(subPath, subDocument, error))
        return false;
    std::vector<std::filesystem::path> childChain = instanceChain;
    childChain.push_back(subPath);
    return SceneInstantiator::instantiate(subDocument, groupWidget, assets, error, nullptr, &childChain, theme);
}

bool SceneInstantiator::instantiate(const SceneDocument& document, const std::shared_ptr<Widget>& stage, const AssetDatabase* assets, std::string& error,
                                    SceneInstanceMap* instancesOut, const std::vector<std::filesystem::path>* instanceChain, const ThemeAsset* theme) {
    if (!stage) {
        error = "cannot instantiate scene without a stage widget";
        return false;
    }

    std::unordered_map<std::string, std::shared_ptr<Widget>> instances;
    instances.reserve(document.nodes().size());
    for (const auto& sourceRecord : document.nodes()) {
        // 主题默认值先注入（不覆盖节点显式属性），兄弟键组合 setter 也能看到主题值
        SceneNodeRecord record = sourceRecord;
        if (theme && !theme->empty())
            theme->applyDefaults(record);
        auto instance = createNode(record, error);
        if (!instance) {
            error = "line " + std::to_string(record.line) + ": " + error;
            return false;
        }
        instance->setWidgetName(record.name);
        if (!updateNode(document, record, instance, assets, error)) {
            error = "line " + std::to_string(record.line) + ": " + error;
            return false;
        }
        if (record.type == "MRSceneInstance") {
            if (!instantiateSubScene(document, record, instance, assets, error, instanceChain ? *instanceChain : std::vector<std::filesystem::path>{}, theme))
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

bool SceneInstantiator::updateNode(const SceneDocument& document, const SceneNodeRecord& record, const std::shared_ptr<Widget>& instance, const AssetDatabase* assets,
                                   std::string& error) {
    if (!instance) {
        error = "cannot update a null scene instance";
        return false;
    }
    instance->setWidgetName(record.name);
    // 锚点三键（anchor_min/anchor_max/anchor_offset）需组合应用，先于逐键循环
    applyAnchors(record, instance);
    for (const auto& [key, value] : record.properties) {
        if (!applyProperty(record, instance, key, value, document, assets, error)) {
            return false;
        }
    }
    return true;
}

bool SceneInstantiator::applyRuntimeProperty(const std::string& nodeType, const std::shared_ptr<Widget>& instance, const std::string& key, const std::string& value,
                                             const AssetDatabase* assets, std::string& error) {
    if (!instance) {
        error = "cannot apply a property to a null instance";
        return false;
    }
    // 动画采样直写：临时记录只含当前键，走与文档属性相同的分发路径
    SceneNodeRecord temp;
    temp.type = nodeType;
    temp.name = "animation";
    temp.properties[key] = value;
    return applyProperty(temp, instance, key, value, SceneDocument{}, assets, error);
}

bool SceneInstantiator::updateNodeTransform(const SceneNodeRecord& record, const std::shared_ptr<Widget>& instance, std::string& error) {
    if (!instance) {
        error = "cannot update a null scene instance";
        return false;
    }
    static const std::vector<std::string> properties = {"position", "size", "scale", "rotation", "visible", "display_layer"};
    SceneDocument emptyDocument;
    for (const auto& key : properties) {
        const auto value = record.properties.find(key);
        if (value == record.properties.end())
            continue;
        if (!applyProperty(record, instance, key, value->second, emptyDocument, nullptr, error)) {
            return false;
        }
    }
    return true;
}

}  // namespace morrow::editor
