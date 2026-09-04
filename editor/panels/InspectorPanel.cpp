#include "panels/InspectorPanel.h"

#include <algorithm>
#include <array>
#include <codecvt>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "EditorShell.h"
#include "Engine.h"
#include "assets/MaterialAsset.h"
#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRCheckBox.h"
#include "elements/MRLabel.h"
#include "elements/MRLineEdit.h"
#include "elements/MRPopupMenu.h"
#include "filesystem/ProjectFileSystemModel.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/SceneTreePanel.h"
#include "panels/ViewportPanel.h"
#include "FileSystemPanel.h"
#include "stb_image.h"

namespace {
bool parseTypedComponents(const std::string& value, const std::string& type, std::vector<std::string>& components) {
    if (value.rfind(type + "(", 0) != 0 || value.empty() || value.back() != ')')
        return false;
    std::istringstream stream(value.substr(type.size() + 1, value.size() - type.size() - 2));
    std::string component;
    while (std::getline(stream, component, ',')) {
        const auto first = component.find_first_not_of(" \t");
        const auto last = component.find_last_not_of(" \t");
        components.push_back(first == std::string::npos ? std::string{} : component.substr(first, last - first + 1));
    }
    return !components.empty();
}

std::string typedComponents(const std::string& type, const std::vector<std::string>& components) {
    std::ostringstream output;
    output << type << '(';
    for (size_t index = 0; index < components.size(); ++index) {
        if (index > 0)
            output << ", ";
        output << components[index];
    }
    output << ')';
    return output.str();
}

const morrow::editor::InspectorProperty* findInspectorProperty(const std::vector<morrow::editor::InspectorProperty>& properties, const std::string& name) {
    const auto iterator = std::find_if(properties.begin(), properties.end(), [&name](const auto& property) { return property.name == name; });
    return iterator == properties.end() ? nullptr : &*iterator;
}

std::wstring wide(const std::string& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
    } catch (...) {
        return std::wstring(text.begin(), text.end());
    }
}

void addInspectorText(morrow::editor::InspectorPanel& inspector, const std::string& text, float y) {
    auto label = std::make_shared<morrow::MRLabel>();
    label->setText(wide(text), "default");
    label->setFontSize(13.0f);
    label->setFontColor(0.82f, 0.84f, 0.88f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getTransform()->setPosition(12.0f, y, 0.0f);
    label->getTransform()->setSize(std::max(80.0f, inspector.root()->getTransform()->getSize().x * 0.34f), 28.0f);
    inspector.root()->addChild(label);
}
}  // namespace

namespace morrow::editor {

InspectorPanel::InspectorPanel(EditorShell& shell) : m_shell(shell) {
}

std::string InspectorPanel::assetPropertyAt(float x, float y, const AssetRecord& asset) const {
    for (const auto& [property, binding] : bindings) {
        const auto target = binding.dropTarget ? binding.dropTarget : std::static_pointer_cast<Widget>(binding.button);
        const auto targetWidget = std::dynamic_pointer_cast<UIWidget>(target);
        if (!targetWidget || !targetWidget->getScreenSpaceAABB().Contains(x, y))
            continue;
        if (binding.type == "TextureAsset" && asset.type == "Texture")
            return property;
        if (binding.type == "MaterialAsset" && asset.type == "Material")
            return property;
        if (binding.property == "material_shader" && asset.type == "Shader")
            return property;
    }
    return {};
}

void InspectorPanel::inspectAsset(const ProjectFileEntry& entry) {
    selectedAssetId = entry.assetId;
    selectedAssetType = entry.assetType;
    refresh(true);
}

void InspectorPanel::clearAsset() {
    if (selectedAssetId.empty())
        return;
    selectedAssetId.clear();
    selectedAssetType.clear();
    refresh(true);
}

void InspectorPanel::setAssetDropTarget(const std::string& property) {
    if (assetDropProperty == property)
        return;
    assetDropProperty = property;
    for (auto& [name, binding] : bindings) {
        if (binding.type != "TextureAsset" || !binding.button)
            continue;
        const bool active = name == property;
        binding.button->setBackgroundColor(active ? Vector4(0.10f, 0.38f, 0.28f, 1.0f) : Vector4(0.075f, 0.085f, 0.105f, 1.0f));
        binding.button->setHoverColor(active ? Vector4(0.15f, 0.52f, 0.38f, 1.0f) : Vector4(0.14f, 0.20f, 0.29f, 1.0f));
    }
}

void InspectorPanel::refresh(bool force) {
    if (!panel)
        return;
    if (!selectedAssetId.empty()) {
        while (panel->m_children.size() > 1)
            panel->m_children.pop_back();
        editConnections.clear();
        checkConnections.clear();
        interactionConnections.clear();
        bindings.clear();
        const auto* asset = m_shell.m_assets.findById(selectedAssetId);
        if (!asset) {
            selectedAssetId.clear();
            selectedAssetType.clear();
        } else if (asset->type == "Material") {
            addInspectorText(*this, "Material", 38.0f);
            addInspectorText(*this, asset->sourcePath.generic_string(), 66.0f);
            MaterialAsset material;
            std::string error;
            if (!loadMaterialAsset(m_shell.m_projectPath.parent_path() / asset->sourcePath, material, error)) {
                addInspectorText(*this, error, 104.0f);
                return;
            }
            addInspectorText(*this, "Shader", 104.0f);
            auto shader = MRButton::create();
            shader->setText(wide(material.shader.empty() ? "<empty>" : material.shader), "default");
            shader->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
            shader->setTextFontSize(13.0f);
            shader->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
            shader->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            shader->getTransform()->setPosition(124.0f, 104.0f, 0.0f);
            shader->getTransform()->setSize(std::max(70.0f, panel->getTransform()->getSize().x - 136.0f), 28.0f);
            panel->addChild(shader);
            bindings["material_shader"] = {"material_shader", "ShaderAsset", {}, shader};
            float y = 140.0f;
            for (const auto& [key, value] : material.properties) {
                addInspectorText(*this, key, y);
                auto edit = MRLineEdit::create();
                edit->setText(wide(value));
                edit->setFontSize(13.0f);
                edit->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
                edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
                edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
                edit->getTransform()->setPosition(124.0f, y, 0.0f);
                edit->getTransform()->setSize(std::max(70.0f, panel->getTransform()->getSize().x - 136.0f), 28.0f);
                panel->addChild(edit);
                editConnections.emplace_back(edit->events().onSubmitted.connect([this, assetId = selectedAssetId, key](MRTextEdit&, const std::wstring& text) {
                    const auto* selected = m_shell.m_assets.findById(assetId);
                    if (!selected)
                        return;
                    MaterialAsset material;
                    std::string error;
                    if (!loadMaterialAsset(m_shell.m_projectPath.parent_path() / selected->sourcePath, material, error))
                        return;
                    material.properties[key] = std::string(text.begin(), text.end());
                    if (!saveMaterialAsset(m_shell.m_projectPath.parent_path() / selected->sourcePath, material, error))
                        m_shell.setStatus(error);
                    else
                        m_shell.setStatus("Saved material " + selected->sourcePath.generic_string());
                }));
                y += 32.0f;
            }
            return;
        } else {
            addInspectorText(*this, selectedAssetType.empty() ? "Asset" : selectedAssetType, 38.0f);
            addInspectorText(*this, asset->sourcePath.generic_string(), 66.0f);
            if (asset->type == "Texture") {
                int imageWidth = 0;
                int imageHeight = 0;
                int components = 0;
                const auto path = m_shell.m_projectPath.parent_path() / asset->sourcePath;
                if (stbi_info(path.string().c_str(), &imageWidth, &imageHeight, &components)) {
                    addInspectorText(*this, "Size", 104.0f);
                    addInspectorText(*this, std::to_string(imageWidth) + " x " + std::to_string(imageHeight), 132.0f);
                    addInspectorText(*this, "Channels", 168.0f);
                    addInspectorText(*this, std::to_string(components), 196.0f);
                }
            }
            return;
        }
    }
    auto properties = m_shell.m_session->model().inspectSelected();
    std::ostringstream signature;
    for (const auto& nodeId : m_shell.m_session->model().selection().nodeIds) {
        signature << "node:" << nodeId << '\n';
    }
    for (const auto& property : properties) {
        signature << property.component << '\x1f' << property.name << '\x1f' << property.type << '\x1f' << (property.editable ? '1' : '0') << '\x1f' << (property.mixed ? '1' : '0')
                  << '\n';
    }
    const std::string schemaSignature = signature.str();
    if (!force && schemaSignature == lastSchemaSignature && !bindings.empty()) {
        updateValues(properties);
        return;
    }
    lastSchemaSignature = schemaSignature;

    while (panel->m_children.size() > 1) {
        panel->m_children.pop_back();
    }
    editConnections.clear();
    checkConnections.clear();
    bindings.clear();
    if (properties.empty())
        return;

    const float panelWidth = std::max(180.0f, panel->getTransform()->getSize().x);
    const float fieldWidth = panelWidth - 24.0f;
    const float labelWidth = std::clamp(fieldWidth * 0.34f, 88.0f, 124.0f);
    const float controlX = 12.0f + labelWidth;
    const float controlWidth = std::max(70.0f, fieldWidth - labelWidth);
    float y = 38.0f;
    const std::vector<std::string> componentPriority = {"Transform", "Mesh Filter", "Mesh Renderer", "Properties"};
    const std::vector<std::string> propertyPriority = {"position",       "rotation",      "scale",        "size",    "mesh",          "renderer_enabled", "material",
                                                       "material_color", "blend_enabled", "double_sided", "visible", "display_layer", "texture_asset"};
    std::stable_sort(properties.begin(), properties.end(), [&componentPriority, &propertyPriority](const InspectorProperty& left, const InspectorProperty& right) {
        const auto order = [](const std::vector<std::string>& priority, const std::string& name) {
            const auto iterator = std::find(priority.begin(), priority.end(), name);
            return iterator == priority.end() ? priority.size() : static_cast<size_t>(std::distance(priority.begin(), iterator));
        };
        const auto leftComponent = order(componentPriority, left.component);
        const auto rightComponent = order(componentPriority, right.component);
        return leftComponent == rightComponent ? order(propertyPriority, left.name) < order(propertyPriority, right.name) : leftComponent < rightComponent;
    });

    std::string activeComponent;
    for (const auto& property : properties) {
        if (property.component != activeComponent) {
            activeComponent = property.component;
            auto header = MRButton::create();
            header->setInteractive(false);
            header->setText(L"v  " + wide(activeComponent), "default");
            header->setTextFontSize(15.0f);
            header->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
            header->setTextColor(Vector4(0.92f, 0.93f, 0.95f, 1.0f));
            header->setBackgroundColor(Vector4(0.20f, 0.21f, 0.23f, 1.0f));
            header->setCornerRadius(2.0f);
            header->getTransform()->setPosition(6.0f, y, 0.0f);
            header->getTransform()->setSize(panelWidth - 12.0f, 26.0f);
            panel->addChild(header);
            y += 30.0f;
        }

        auto propertyLabel = std::make_shared<MRLabel>();
        propertyLabel->setText(wide(property.displayName), "default");
        propertyLabel->setFontSize(13.0f);
        propertyLabel->setFontColor(0.78f, 0.80f, 0.84f, 1.0f);
        propertyLabel->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        propertyLabel->getTransform()->setPosition(14.0f, y, 0.0f);
        propertyLabel->getTransform()->setSize(labelWidth - 8.0f, 28.0f);
        panel->addChild(propertyLabel);

        if (property.type == "bool" && !property.mixed) {
            auto checkBox = MRCheckBox::create();
            checkBox->setChecked(property.value == "true");
            checkBox->setTextFontSize(14.0f);
            checkBox->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
            checkBox->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            checkBox->setHoverColor(Vector4(0.12f, 0.15f, 0.20f, 1.0f));
            checkBox->setCheckedColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            checkBox->setCheckedHoverColor(Vector4(0.12f, 0.15f, 0.20f, 1.0f));
            checkBox->setCheckedPressedColor(Vector4(0.12f, 0.15f, 0.20f, 1.0f));
            checkBox->getTransform()->setPosition(controlX, y, 0.0f);
            checkBox->getTransform()->setSize(controlWidth, 28.0f);
            checkConnections.emplace_back(checkBox->selectionEvents().onCheckedChanged.connect([this, propertyName = property.name](MRSelectableButton& source, bool checked) {
                if (m_applyingInspectorValue)
                    return;
                m_applyingInspectorValue = true;
                applyValue(propertyName, checked ? "true" : "false");
                const auto current = m_shell.m_session->model().inspectSelected();
                if (const auto* value = findInspectorProperty(current, propertyName); value && value->value != (checked ? "true" : "false"))
                    source.setChecked(value->value == "true");
                m_applyingInspectorValue = false;
            }));
            panel->addChild(checkBox);
            InspectorBinding& binding = bindings[property.name];
            binding.property = property.name;
            binding.type = property.type;
            binding.checkBox = std::move(checkBox);
        } else if (property.type == "TextureAsset" || property.type == "MaterialAsset") {
            std::string display = "<empty>";
            if (!property.value.empty()) {
                display = property.value;
                if (const auto* asset = m_shell.m_assets.findById(property.value)) {
                    display = asset->sourcePath.generic_string();
                }
            }
            auto textField = MRLineEdit::create();
            textField->setText(wide(display));
            textField->setReadOnly(true);
            textField->setFontSize(13.0f);
            textField->setTextColor(Vector4(0.86f, 0.89f, 0.94f, 1.0f));
            textField->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            textField->getTransform()->setPosition(controlX, y, 0.0f);
            textField->getTransform()->setSize(std::max(40.0f, controlWidth - 30.0f), 28.0f);
            if (auto interaction = textField->getComponent<Interaction>()) {
                interactionConnections.emplace_back(interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [this, propertyName = property.name](TouchEvent&) {
                    const auto properties = m_shell.m_session->model().inspectSelected();
                    const auto* inspectorProperty = findInspectorProperty(properties, propertyName);
                    if (!inspectorProperty)
                        return;
                    const auto* current = m_shell.m_assets.findById(inspectorProperty->value);
                    if (current && m_shell.m_assetsPanel->view) {
                        if (m_shell.m_bottomTabs)
                            m_shell.m_bottomTabs->selectTab("filesystem");
                        m_shell.m_assetsPanel->view->selectAsset(current->assetId, false);
                        m_shell.setStatus("Located asset " + current->sourcePath.generic_string());
                    }
                }));
            }
            panel->addChild(textField);
            auto button = MRButton::create();
            button->setText(L"+", "default");
            button->setTextFontSize(18.0f);
            button->setTextColor(Vector4(0.96f, 0.98f, 1.0f, 1.0f));
            button->setBackgroundColor(Vector4(0.16f, 0.34f, 0.62f, 1.0f));
            button->setHoverColor(Vector4(0.24f, 0.48f, 0.80f, 1.0f));
            button->setPressedColor(Vector4(0.10f, 0.25f, 0.50f, 1.0f));
            button->getTransform()->setPosition(controlX + controlWidth - 28.0f, y, 0.0f);
            button->getTransform()->setSize(28.0f, 28.0f);
            m_shell.m_buttonConnections.emplace_back(button->events().onClicked.connect([this, property](BaseButton& source) {
                assetMenu->clear();
                assetMenuIds.clear();
                assetMenu->addItem(L"<empty>", 0);
                assetMenuIds[0] = "";
                int itemId = 1;
                for (const auto& asset : m_shell.m_assets.assets()) {
                    const bool acceptable = property.type == "TextureAsset" ? asset.type == "Texture" : asset.type == "Material";
                    if (!acceptable || !asset.error.empty())
                        continue;
                    const std::string pathText = asset.sourcePath.generic_string();
                    assetMenu->addItem(std::wstring(pathText.begin(), pathText.end()), itemId);
                    assetMenuIds[itemId] = asset.assetId;
                    ++itemId;
                }
                assetEditProperty = property.name;
                assetEditNodeIds = m_shell.m_session->model().selection().nodeIds;
                assetMenu->attachTo(m_shell.m_shellRoot);
                assetMenu->popupBelow(source.getScreenSpaceAABB());
            }));
            panel->addChild(button);
            bindings[property.name] = {property.name, property.type, {}, button};
            bindings[property.name].resourceField = textField;
            bindings[property.name].dropTarget = textField;
        } else if ((property.type == "Vector2" || property.type == "Vector3" || property.type == "Color") && !property.mixed) {
            std::vector<std::string> components;
            if (!parseTypedComponents(property.value, property.type, components)) {
                components.assign(property.type == "Vector2" ? 2 : (property.type == "Vector3" ? 3 : 4), "0.0");
            }
            const float gap = 3.0f;
            const float width = (controlWidth - gap * static_cast<float>(components.size() - 1)) / static_cast<float>(components.size());
            for (size_t index = 0; index < components.size(); ++index) {
                auto edit = MRLineEdit::create();
                edit->setFontSize(13.0f);
                edit->setText(std::wstring(components[index].begin(), components[index].end()));
                edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
                edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
                const std::array<Vector4, 4> componentColors = {Vector4(0.90f, 0.40f, 0.43f, 1.0f), Vector4(0.55f, 0.80f, 0.35f, 1.0f), Vector4(0.38f, 0.64f, 0.94f, 1.0f),
                                                                Vector4(0.78f, 0.78f, 0.80f, 1.0f)};
                edit->setTextColor(componentColors[std::min(index, componentColors.size() - 1)]);
                edit->getTransform()->setPosition(controlX + static_cast<float>(index) * (width + gap), y, 0.0f);
                edit->getTransform()->setSize(width, 28.0f);
                editConnections.emplace_back(
                    edit->events().onSubmitted.connect([this, propertyName = property.name, propertyType = property.type, index](MRTextEdit&, const std::wstring& text) {
                        const auto current = m_shell.m_session->model().inspectSelected();
                        const auto* currentProperty = findInspectorProperty(current, propertyName);
                        if (!currentProperty)
                            return;
                        std::vector<std::string> updated;
                        if (!parseTypedComponents(currentProperty->value, propertyType, updated) || index >= updated.size()) {
                            m_shell.setStatus("Current vector value is invalid");
                            return;
                        }
                        updated[index] = std::string(text.begin(), text.end());
                        try {
                            size_t consumed = 0;
                            std::stof(updated[index], &consumed);
                            if (consumed != updated[index].size())
                                throw std::invalid_argument("number");
                        } catch (...) {
                            m_shell.setStatus("Vector component must be numeric");
                            return;
                        }
                        const std::string value = typedComponents(propertyType, updated);
                        m_shell.m_engine->mainThreadDispatcher().post([this, propertyName, value] { applyValue(propertyName, value); });
                    }));
                panel->addChild(edit);
                bindings[property.name].property = property.name;
                bindings[property.name].type = property.type;
                bindings[property.name].edits.push_back(edit);
            }
        } else {
            auto edit = MRLineEdit::create();
            edit->setFontSize(13.0f);
            const std::string initial = property.mixed ? std::string{} : property.value;
            edit->setText(std::wstring(initial.begin(), initial.end()));
            if (property.mixed)
                edit->setPlaceholder(L"<mixed>");
            edit->setReadOnly(!property.editable);
            edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
            edit->setTextColor(property.editable ? Vector4(0.88f, 0.90f, 0.94f, 1.0f) : Vector4(0.60f, 0.62f, 0.66f, 1.0f));
            edit->getTransform()->setPosition(controlX, y, 0.0f);
            edit->getTransform()->setSize(controlWidth, 28.0f);
            if (property.editable) {
                editConnections.emplace_back(edit->events().onSubmitted.connect([this, property](MRTextEdit&, const std::wstring& text) {
                    const std::string value(text.begin(), text.end());
                    if (property.type == "number") {
                        try {
                            size_t consumed = 0;
                            std::stof(value, &consumed);
                            if (consumed != value.size())
                                throw std::invalid_argument("number");
                        } catch (...) {
                            m_shell.setStatus("Property must be numeric");
                            return;
                        }
                    }
                    m_shell.m_engine->mainThreadDispatcher().post([this, propertyName = property.name, value] { applyValue(propertyName, value); });
                }));
            }
            panel->addChild(edit);
            bindings[property.name] = {property.name, property.type, {edit}, {}};
        }
        y += 32.0f;
    }
}

void InspectorPanel::updateValues(const std::vector<InspectorProperty>& properties) {
    for (auto& [name, binding] : bindings) {
        const auto* property = findInspectorProperty(properties, name);
        if (!property)
            continue;
        if (binding.type == "bool") {
            if (binding.checkBox) {
                m_applyingInspectorValue = true;
                binding.checkBox->setChecked(property->value == "true");
                m_applyingInspectorValue = false;
            }
            continue;
        }
        if ((binding.type == "TextureAsset" || binding.type == "MaterialAsset") && binding.resourceField) {
            std::string display = "<empty>";
            if (!property->value.empty()) {
                display = property->value;
                if (const auto* asset = m_shell.m_assets.findById(property->value)) {
                    display = asset->sourcePath.generic_string();
                }
            }
            binding.resourceField->setText(wide(display));
            continue;
        }
        if (binding.type == "Vector2" || binding.type == "Vector3" || binding.type == "Color") {
            std::vector<std::string> components;
            if (!parseTypedComponents(property->value, binding.type, components)) {
                continue;
            }
            for (size_t index = 0; index < binding.edits.size() && index < components.size(); ++index) {
                binding.edits[index]->setText(std::wstring(components[index].begin(), components[index].end()));
            }
            continue;
        }
        if (!binding.edits.empty()) {
            const std::string value = property->mixed ? std::string{} : property->value;
            binding.edits.front()->setText(std::wstring(value.begin(), value.end()));
            binding.edits.front()->setPlaceholder(property->mixed ? L"<mixed>" : L"");
        }
    }
}

void InspectorPanel::applyValue(const std::string& property, const std::string& value) {
    if (!selectedAssetId.empty() && property == "material_shader") {
        const auto* asset = m_shell.m_assets.findById(selectedAssetId);
        if (!asset)
            return;
        MaterialAsset material;
        std::string error;
        if (!loadMaterialAsset(m_shell.m_projectPath.parent_path() / asset->sourcePath, material, error)) {
            m_shell.setStatus(error);
            return;
        }
        const auto* shader = m_shell.m_assets.findById(value);
        if (!shader || shader->type != "Shader") {
            m_shell.setStatus("Shader asset '" + value + "' was not found");
            return;
        }
        material.shader = value;
        if (saveMaterialAsset(m_shell.m_projectPath.parent_path() / asset->sourcePath, material, error)) {
            m_shell.setStatus("Assigned shader " + value);
            refresh(true);
        } else {
            m_shell.setStatus(error);
        }
        return;
    }
    std::string error;
    bool changed = false;
    for (const auto& nodeId : m_shell.m_session->model().selection().nodeIds) {
        if (!m_shell.m_session->setProperty(nodeId, property, value, false, error)) {
            break;
        }
        changed = true;
    }
    if (!changed) {
        if (!error.empty())
            m_shell.setStatus(error);
        return;
    }
    const bool transformOnly =
        property == "position" || property == "size" || property == "scale" || property == "rotation" || property == "visible" || property == "display_layer";
    m_shell.m_viewport->syncSelectedRuntimeNodes(transformOnly);
    m_shell.m_viewport->refreshSelectionOverlay();
    refresh();
    m_shell.setStatus("Applied " + property + " = " + value);
}

void InspectorPanel::beginLegacyPropertyEdit(const std::string& property, const std::string& value) {
    legacyEditProperty = property;
    legacyEditValue = value;
    m_shell.setStatus("Editing " + property + ": type a value and press Enter");
}

void InspectorPanel::commitLegacyPropertyEdit() {
    if (legacyEditProperty.empty())
        return;
    std::string error;
    bool changed = false;
    for (const auto& nodeId : m_shell.m_session->model().selection().nodeIds) {
        if (!m_shell.m_session->setProperty(nodeId, legacyEditProperty, legacyEditValue, false, error))
            break;
        changed = true;
    }
    if (changed) {
        const bool transformOnly = legacyEditProperty == "position" || legacyEditProperty == "size" || legacyEditProperty == "scale" || legacyEditProperty == "rotation" ||
                                   legacyEditProperty == "visible" || legacyEditProperty == "display_layer";
        m_shell.m_viewport->syncSelectedRuntimeNodes(transformOnly);
        m_shell.m_viewport->refreshSelectionOverlay();
        refresh();
        m_shell.setStatus("Applied " + legacyEditProperty + " = " + legacyEditValue);
    } else if (!error.empty()) {
        m_shell.setStatus(error);
    }
    legacyEditProperty.clear();
    legacyEditValue.clear();
}

void InspectorPanel::appendLegacyCharacter(unsigned int codepoint) {
    if (legacyEditProperty.empty() || codepoint < 32 || codepoint > 126)
        return;
    legacyEditValue.push_back(static_cast<char>(codepoint));
    m_shell.setStatus("Editing " + legacyEditProperty + ": " + legacyEditValue);
}

}  // namespace morrow::editor
