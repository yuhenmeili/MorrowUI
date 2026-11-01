#include "panels/InspectorPanel.h"

#include <algorithm>
#include <array>
#include <codecvt>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "EditorShell.h"
#include "MainThreadDispatcher.h"
#include "morrow/Engine.h"
#include "assets/MaterialAsset.h"
#include "morrow/base/Interaction.h"
#include "morrow/base/TouchEvent.h"
#include "morrow/base/Transform.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRCheckBox.h"
#include "morrow/elements/MRColor.h"
#include "morrow/elements/MRLabel.h"
#include "morrow/elements/MRLineEdit.h"
#include "morrow/elements/MRPopupMenu.h"
#include "morrow/elements/MRPopup.h"
#include "morrow/elements/MROptionButton.h"
#include "morrow/elements/MRSlider.h"
#include "filesystem/ProjectFileSystemModel.h"
#include "ui/CommandPalette.h"
#include "panels/OutputPanel.h"
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

/// number 范围属性：实际值 ↔ 滑块 0..1 归一化与步长量化。
float rangeNormalize(float value, const morrow::editor::InspectorProperty& property) {
    const float span = property.maxValue - property.minValue;
    return span > 0.0f ? std::clamp((value - property.minValue) / span, 0.0f, 1.0f) : 0.0f;
}

float rangeQuantize(const morrow::editor::InspectorProperty& property, float normalized) {
    const float raw = property.minValue + normalized * (property.maxValue - property.minValue);
    const float step = property.stepValue > 0.0f ? property.stepValue : 1.0f;
    return std::round(raw / step) * step;
}

std::string formatNumber(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
    return buffer;
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
        if (!targetWidget || !targetWidget->getWorldSpaceAABB().Contains(x, y))
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
    selectedAssetId.clear();
    selectedAssetType.clear();
    refresh(true);
}

/// 搜索框是 Inspector 的常驻子控件（标题下第一行），资产视图时隐藏。
/// 编辑复用 legacy 属性编辑通道（属性名 "__search__"），避免侵入键盘路由。
void InspectorPanel::ensureSearchBox() {
    if (searchBox || !panel)
        return;
    searchBox = MRLineEdit::create();
    searchBox->setReadOnly(true);
    searchBox->setFontSize(13.0f);
    searchBox->setTextColor(Vector4(0.70f, 0.74f, 0.80f, 1.0f));
    searchBox->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
    searchBox->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
    const float panelWidth = std::max(180.0f, panel->getTransform()->getSize().x);
    searchBox->getTransform()->setPosition(12.0f, 36.0f, 0.0f);
    searchBox->getTransform()->setSize(panelWidth - 24.0f, 26.0f);
    panel->addChild(searchBox);
    if (auto interaction = searchBox->getComponent<Interaction>()) {
        searchClickConnection = interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [this](TouchEvent&) {
            beginLegacyPropertyEdit("__search__", searchFilter);
            searchBox->setText(L"");
        });
    }
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
        while (panel->m_children.size() > (searchBox ? 2u : 1u))
            panel->m_children.pop_back();
        editConnections.clear();
        checkConnections.clear();
        interactionConnections.clear();
        bindings.clear();
        if (searchBox)
            searchBox->setVisible(false);
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
                  << '\x1f' << (property.defaultValue.empty() ? '0' : '1') << '\n';
    }
    signature << "filter:" << searchFilter << '\n';
    const std::string schemaSignature = signature.str();
    if (!force && schemaSignature == lastSchemaSignature && !bindings.empty()) {
        updateValues(properties);
        return;
    }
    lastSchemaSignature = schemaSignature;

    ensureSearchBox();
    while (panel->m_children.size() > (searchBox ? 2u : 1u)) {
        panel->m_children.pop_back();
    }
    editConnections.clear();
    checkConnections.clear();
    sliderConnections.clear();
    optionConnections.clear();
    bindings.clear();
    if (searchBox) {
        searchBox->setVisible(true);
        searchBox->setText(wide(searchFilter));
        searchBox->setPlaceholder(L"Filter... (click, type, Enter)");
    }
    if (properties.empty())
        return;

    const float panelWidth = std::max(180.0f, panel->getTransform()->getSize().x);
    const float fieldWidth = panelWidth - 24.0f;
    const float labelWidth = std::clamp(fieldWidth * 0.34f, 88.0f, 124.0f);
    const float controlX = 12.0f + labelWidth;
    const float controlWidth = std::max(70.0f, fieldWidth - labelWidth);
    float y = 72.0f;
    const std::vector<std::string> componentPriority = {"Transform", "Layout", "Mesh Filter", "Mesh Renderer", "Properties"};
    const std::vector<std::string> propertyPriority = {"position",       "rotation",      "scale",        "size",    "mesh",          "renderer_enabled", "material",
                                                       "material_color", "blend_enabled", "double_sided", "visible", "locked",        "display_layer",    "texture_asset"};
    std::stable_sort(properties.begin(), properties.end(), [&componentPriority, &propertyPriority](const InspectorProperty& left, const InspectorProperty& right) {
        const auto order = [](const std::vector<std::string>& priority, const std::string& name) {
            const auto iterator = std::find(priority.begin(), priority.end(), name);
            return iterator == priority.end() ? priority.size() : static_cast<size_t>(std::distance(priority.begin(), iterator));
        };
        const auto leftComponent = order(componentPriority, left.component);
        const auto rightComponent = order(componentPriority, right.component);
        return leftComponent == rightComponent ? order(propertyPriority, left.name) < order(propertyPriority, right.name) : leftComponent < rightComponent;
    });

    // 搜索过滤 + 折叠分组：命中过滤词或位于已折叠分组的行不渲染
    const std::string filter = [&searchFilter = searchFilter]() {
        std::string lowered;
        for (const char character : searchFilter)
            lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
        return lowered;
    }();
    std::vector<InspectorProperty> shownProperties;
    for (const auto& property : properties) {
        if (collapsedComponents.count(property.component) != 0)
            continue;
        if (!filter.empty()) {
            std::string haystack;
            for (const char character : property.displayName + " " + property.name + " " + property.component)
                haystack.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
            if (haystack.find(filter) == std::string::npos)
                continue;
        }
        shownProperties.push_back(property);
    }
    if (shownProperties.empty()) {
        if (!filter.empty())
            addInspectorText(*this, "No matching properties", y);
        return;
    }

    std::string activeComponent;
    for (const auto& property : shownProperties) {
        if (property.component != activeComponent) {
            activeComponent = property.component;
            const bool collapsed = collapsedComponents.count(activeComponent) != 0;
            auto header = MRButton::create();
            header->setText(collapsed ? L">  " + wide(activeComponent) : L"v  " + wide(activeComponent), "default");
            header->setTextFontSize(15.0f);
            header->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
            header->setTextColor(Vector4(0.92f, 0.93f, 0.95f, 1.0f));
            header->setBackgroundColor(Vector4(0.20f, 0.21f, 0.23f, 1.0f));
            header->setHoverColor(Vector4(0.26f, 0.28f, 0.31f, 1.0f));
            header->setCornerRadius(2.0f);
            header->getTransform()->setPosition(6.0f, y, 0.0f);
            header->getTransform()->setSize(panelWidth - 12.0f, 26.0f);
            m_shell.m_buttonConnections.emplace_back(
                header->events().onClicked.connect([this, component = activeComponent](BaseButton&) { toggleComponentCollapsed(component); }));
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

        // 偏离默认值的可重置行右侧留出 ↺ 按钮，Color 行再留色板按钮，其余行占满整行
        const bool isColorRow = property.type == "Color" && !property.mixed && property.editable;
        const bool hasReset = !property.defaultValue.empty() && !property.mixed && property.type != "TextureAsset" && property.type != "MaterialAsset";
        const float rowControlWidth = controlWidth - ((hasReset ? 1u : 0u) + (isColorRow ? 1u : 0u)) * 30.0f;
        if (hasReset) {
            auto reset = MRButton::create();
            reset->setText(L"R", "default");
            reset->setTextFontSize(13.0f);
            reset->setTextColor(Vector4(0.95f, 0.75f, 0.35f, 1.0f));
            reset->setBackgroundColor(Vector4(0.16f, 0.16f, 0.18f, 1.0f));
            reset->setHoverColor(Vector4(0.30f, 0.26f, 0.16f, 1.0f));
            reset->setPressedColor(Vector4(0.12f, 0.10f, 0.06f, 1.0f));
            reset->getTransform()->setPosition(controlX + controlWidth - 26.0f, y, 0.0f);
            reset->getTransform()->setSize(26.0f, 28.0f);
            m_shell.m_buttonConnections.emplace_back(reset->events().onClicked.connect([this, property](BaseButton&) {
                applyValue(property.name, property.defaultValue);
                m_shell.setStatus("Reset " + property.displayName + " to default");
            }));
            panel->addChild(reset);
        }

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
            checkBox->getTransform()->setSize(rowControlWidth, 28.0f);
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
                assetMenu->addItem(wide(pathText), itemId);
                    assetMenuIds[itemId] = asset.assetId;
                    ++itemId;
                }
                assetEditProperty = property.name;
                assetEditNodeIds = m_shell.m_session->model().selection().nodeIds;
                assetMenu->attachTo(m_shell.m_shellRoot);
                assetMenu->popupBelow(source.getWorldSpaceAABB());
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
            const float width = (rowControlWidth - gap * static_cast<float>(components.size() - 1)) / static_cast<float>(components.size());
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
            if (property.type == "Color") {
                // 当前颜色色板：点击弹出取色器（色板 MVP），位于 R 按钮左侧
                auto swatch = MRColor::create();
                Vector4 current(1.0f, 1.0f, 1.0f, 1.0f);
                std::vector<std::string> rgba;
                if (parseTypedComponents(property.value, "Color", rgba) && rgba.size() == 4) {
                    try {
                        current = Vector4(std::stof(rgba[0]), std::stof(rgba[1]), std::stof(rgba[2]), std::stof(rgba[3]));
                    } catch (...) {
                    }
                }
                swatch->setColor(current);
                swatch->setRounding(3.0f);
                const float swatchX = controlX + controlWidth - (hasReset ? 56.0f : 26.0f);
                swatch->getTransform()->setPosition(swatchX, y, 0.0f);
                swatch->getTransform()->setSize(26.0f, 28.0f);
                if (auto interaction = swatch->getComponent<Interaction>()) {
                    interactionConnections.emplace_back(interaction->addEventListener(
                        TOUCH_EVENT_TYPE_RELEASE, [this, property](TouchEvent& event) { openColorPicker(property, event.positionX, event.positionY); }));
                }
                panel->addChild(swatch);
            }
        } else if (property.type == "enum" && !property.enumOptions.empty() && !property.mixed && property.editable) {
            // 枚举属性：下拉选择，选中项 id 复用枚举表下标
            auto option = MROptionButton::create();
            option->setTextFontSize(13.0f);
            option->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
            option->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            option->setHoverColor(Vector4(0.14f, 0.20f, 0.29f, 1.0f));
            option->setCornerRadius(3.0f);
            option->getTransform()->setPosition(controlX, y, 0.0f);
            option->getTransform()->setSize(rowControlWidth, 28.0f);
            for (size_t index = 0; index < property.enumOptions.size(); ++index) {
                option->addOption(wide(property.enumOptions[index].second), static_cast<int>(index));
                if (property.enumOptions[index].first == property.value)
                    option->setSelected(static_cast<int>(index));
            }
            if (option->getSelectedId() < 0)
                option->setText(property.value.empty() ? L"<unset>" : wide(property.value), "default");
            optionConnections.emplace_back(option->selectionEvents().onSelected.connect([this, property](MROptionButton&, int id, const std::wstring&) {
                if (m_applyingInspectorValue)
                    return;  // 程序化 setSelected 回写触发的通知,忽略避免 refresh 循环
                if (id < 0 || id >= static_cast<int>(property.enumOptions.size()))
                    return;
                applyValue(property.name, property.enumOptions[static_cast<size_t>(id)].first);
            }));
            panel->addChild(option);
            InspectorBinding& binding = bindings[property.name];
            binding.property = property.name;
            binding.type = property.type;
            binding.optionButton = std::move(option);
        } else if (property.type == "number" && property.hasRange && !property.mixed && property.editable) {
            // 带范围的数值：滑块 + 紧凑数值输入，拖动连续合并为单条撤销
            const float sliderWidth = std::max(60.0f, rowControlWidth - 58.0f);
            float currentValue = property.minValue;
            try {
                currentValue = std::stof(property.value);
            } catch (...) {
            }
            auto slider = MRSlider::create();
            slider->getTransform()->setPosition(controlX, y + 2.0f, 0.0f);
            slider->getTransform()->setSize(sliderWidth, 24.0f);
            slider->setValue(rangeNormalize(currentValue, property));
            sliderConnections.emplace_back(slider->events().onValueChanged.connect([this, property](MRSlider& source, float value) {
                if (m_applyingInspectorValue)
                    return;
                m_applyingInspectorValue = true;
                const float stepped = rangeQuantize(property, value);
                applyValue(property.name, formatNumber(stepped), true);
                source.setValue(rangeNormalize(stepped, property));
                m_applyingInspectorValue = false;
            }));
            panel->addChild(slider);
            auto edit = MRLineEdit::create();
            edit->setFontSize(13.0f);
            edit->setText(wide(property.value));
            edit->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
            edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
            edit->getTransform()->setPosition(controlX + sliderWidth + 4.0f, y, 0.0f);
            edit->getTransform()->setSize(52.0f, 28.0f);
            editConnections.emplace_back(edit->events().onSubmitted.connect([this, propertyName = property.name](MRTextEdit&, const std::wstring& text) {
                const std::string value(text.begin(), text.end());
                try {
                    size_t consumed = 0;
                    std::stof(value, &consumed);
                    if (consumed != value.size())
                        throw std::invalid_argument("number");
                } catch (...) {
                    m_shell.setStatus("Property must be numeric");
                    return;
                }
                m_shell.m_engine->mainThreadDispatcher().post([this, propertyName, value] { applyValue(propertyName, value); });
            }));
            panel->addChild(edit);
            InspectorBinding& binding = bindings[property.name];
            binding.property = property.name;
            binding.type = property.type;
            binding.edits.push_back(edit);
            binding.slider = std::move(slider);
        } else {
            auto edit = MRLineEdit::create();
            edit->setFontSize(13.0f);
            const std::string initial = property.mixed ? std::string{} : property.value;
            edit->setText(wide(initial));
            if (property.mixed)
                edit->setPlaceholder(L"<mixed>");
            edit->setReadOnly(!property.editable);
            edit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
            edit->setFocusedBackgroundColor(Vector4(0.10f, 0.13f, 0.18f, 1.0f));
            edit->setTextColor(property.editable ? Vector4(0.88f, 0.90f, 0.94f, 1.0f) : Vector4(0.60f, 0.62f, 0.66f, 1.0f));
            edit->getTransform()->setPosition(controlX, y, 0.0f);
            edit->getTransform()->setSize(rowControlWidth, 28.0f);
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
                binding.edits[index]->setText(wide(components[index]));
            }
            continue;
        }
        if (binding.slider) {
            float value = 0.0f;
            try {
                value = std::stof(property->value);
            } catch (...) {
            }
            m_applyingInspectorValue = true;
            binding.slider->setValue(rangeNormalize(value, *property));
            m_applyingInspectorValue = false;
        }
        if (binding.optionButton && !property->enumOptions.empty()) {
            m_applyingInspectorValue = true;
            for (size_t index = 0; index < property->enumOptions.size(); ++index) {
                if (property->enumOptions[index].first == property->value) {
                    binding.optionButton->setSelected(static_cast<int>(index));
                    break;
                }
            }
            m_applyingInspectorValue = false;
        }
        if (!binding.edits.empty()) {
            const std::string value = property->mixed ? std::string{} : property->value;
            binding.edits.front()->setText(std::wstring(value.begin(), value.end()));
            binding.edits.front()->setPlaceholder(property->mixed ? L"<mixed>" : L"");
        }
    }
}

void InspectorPanel::applyValue(const std::string& property, const std::string& value, bool continuous) {
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
        if (!m_shell.m_session->setProperty(nodeId, property, value, continuous, error)) {
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
    if (!continuous) {
        refresh();
        m_shell.setStatus("Applied " + property + " = " + value);
    } else {
        // 拖动中只轻量回填既有控件值，整面板重建会销毁正在交互的滑块
        updateValues(m_shell.m_session->model().inspectSelected());
    }
}

void InspectorPanel::beginLegacyPropertyEdit(const std::string& property, const std::string& value) {
    legacyEditProperty = property;
    legacyEditValue = value;
    m_shell.setStatus("Editing " + property + ": type a value and press Enter");
}

void InspectorPanel::toggleComponentCollapsed(const std::string& component) {
    if (collapsedComponents.count(component) != 0)
        collapsedComponents.erase(component);
    else
        collapsedComponents.insert(component);
    refresh(true);
}

void InspectorPanel::commitLegacyPropertyEdit() {
    if (legacyEditProperty.empty())
        return;
    // 搜索框借用 legacy 编辑通道：Enter 提交过滤词而非属性值
    if (legacyEditProperty == "__search__") {
        searchFilter = legacyEditValue;
        legacyEditProperty.clear();
        legacyEditValue.clear();
        refresh(true);
        m_shell.setStatus(searchFilter.empty() ? "Property filter cleared" : "Property filter: " + searchFilter);
        return;
    }
    // 命令面板：Enter 先按新词过滤，词未变时执行首条命令
    if (legacyEditProperty == "__palette__") {
        const std::string value = legacyEditValue;
        legacyEditProperty.clear();
        legacyEditValue.clear();
        if (m_shell.m_commandPalette && m_shell.m_commandPalette->isOpen()) {
            if (value != m_shell.m_commandPalette->filter()) {
                m_shell.m_commandPalette->setFilter(value);
                m_shell.m_inspector->beginLegacyPropertyEdit("__palette__", value);
            } else {
                m_shell.m_commandPalette->executeFirst();
            }
        }
        return;
    }
    // Output 过滤框
    if (legacyEditProperty == "__logfilter__") {
        const std::string value = legacyEditValue;
        legacyEditProperty.clear();
        legacyEditValue.clear();
        m_shell.m_output->setFilter(value);
        m_shell.setStatus(value.empty() ? "Output filter cleared" : "Output filter: " + value);
        return;
    }
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

void InspectorPanel::cancelLegacyEdit() {
    legacyEditProperty.clear();
    legacyEditValue.clear();
}

namespace {
/// 取色器预设色板（UI 常用色，两行 × 8）。
const Vector4 (&colorPalette())[16] {
    static const Vector4 palette[16] = {
        Vector4(1.00f, 1.00f, 1.00f, 1.0f), Vector4(0.75f, 0.77f, 0.80f, 1.0f), Vector4(0.50f, 0.52f, 0.56f, 1.0f), Vector4(0.25f, 0.27f, 0.30f, 1.0f),
        Vector4(0.06f, 0.07f, 0.09f, 1.0f), Vector4(0.86f, 0.20f, 0.20f, 1.0f), Vector4(0.95f, 0.55f, 0.15f, 1.0f), Vector4(0.98f, 0.75f, 0.19f, 1.0f),
        Vector4(0.93f, 0.90f, 0.30f, 1.0f), Vector4(0.30f, 0.70f, 0.30f, 1.0f), Vector4(0.20f, 0.70f, 0.65f, 1.0f), Vector4(0.30f, 0.80f, 0.95f, 1.0f),
        Vector4(0.25f, 0.45f, 0.90f, 1.0f), Vector4(0.40f, 0.35f, 0.80f, 1.0f), Vector4(0.65f, 0.35f, 0.80f, 1.0f), Vector4(0.95f, 0.55f, 0.75f, 1.0f),
    };
    return palette;
}

std::string colorValue(const Vector4& color, float alpha) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Color(%.3f, %.3f, %.3f, %.3f)", color.x, color.y, color.z, alpha);
    return buffer;
}

Vector4 parseColorValue(const std::string& value) {
    std::vector<std::string> rgba;
    if (value.rfind("Color(", 0) == 0 && value.back() == ')') {
        std::istringstream stream(value.substr(6, value.size() - 7));
        std::string component;
        while (std::getline(stream, component, ','))
            rgba.push_back(component);
    }
    if (rgba.size() != 4)
        return Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    try {
        return Vector4(std::stof(rgba[0]), std::stof(rgba[1]), std::stof(rgba[2]), std::stof(rgba[3]));
    } catch (...) {
        return Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}
}  // namespace

void InspectorPanel::pushRecentColor(const std::string& value) {
    recentColors.erase(std::remove(recentColors.begin(), recentColors.end(), value), recentColors.end());
    recentColors.insert(recentColors.begin(), value);
    if (recentColors.size() > 8)
        recentColors.resize(8);
}

void InspectorPanel::ensureColorPopup() {
    if (colorPopup || !m_shell.m_shellRoot)
        return;
    colorPopup = MRPopup::create();
    colorPopup->getTransform()->setSize(248.0f, 208.0f);
    colorPopup->attachTo(m_shell.m_shellRoot);
    // 内容容器：重建取色器时只清它，不触碰 MRPopup 自身的背景子节点
    colorPopupContent = std::make_shared<UIWidget>(false);
    colorPopupContent->getTransform()->setSize(248.0f, 208.0f);
    colorPopup->addChild(colorPopupContent);
}

void InspectorPanel::openColorPicker(const InspectorProperty& property, float worldX, float worldY) {
    ensureColorPopup();
    if (!colorPopup)
        return;
    colorEditProperty = property.name;
    colorEditAlpha = parseColorValue(property.value).w;
    const Vector4 current = parseColorValue(property.value);

    // 重建弹窗内容：预设两行 + 当前色预览 + 最近使用行（只清内容容器）
    colorPopupConnections.clear();
    colorPopupContent->m_children.clear();
    auto backdrop = MRColor::create();
    backdrop->setColor(Vector4(0.13f, 0.15f, 0.19f, 0.98f));
    backdrop->setRounding(6.0f);
    backdrop->getTransform()->setSize(248.0f, 208.0f);
    colorPopupContent->addChild(backdrop);
    const auto pick = [this](const Vector4& color) {
        const std::string value = colorValue(color, colorEditAlpha);
        applyValue(colorEditProperty, value, false);
        pushRecentColor(value);
        colorPopup->hide();
        m_shell.setStatus("Picked " + value);
    };
    auto addSwatch = [this, &pick](const Vector4& color, float x, float y) {
        auto swatch = MRColor::create();
        swatch->setColor(color);
        swatch->setRounding(3.0f);
        swatch->getTransform()->setPosition(x, y, 0.0f);
        swatch->getTransform()->setSize(24.0f, 24.0f);
        if (auto interaction = swatch->getComponent<Interaction>()) {
            colorPopupConnections.emplace_back(interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [pick, color](TouchEvent&) { pick(color); }));
        }
        colorPopupContent->addChild(swatch);
    };
    const auto& palette = colorPalette();
    for (size_t index = 0; index < 16; ++index)
        addSwatch(palette[index], 10.0f + static_cast<float>(index % 8) * 29.0f, 8.0f + static_cast<float>(index / 8) * 29.0f);

    auto preview = MRColor::create();
    preview->setColor(current);
    preview->setRounding(3.0f);
    preview->getTransform()->setPosition(10.0f, 68.0f, 0.0f);
    preview->getTransform()->setSize(24.0f, 24.0f);
    colorPopupContent->addChild(preview);
    auto caption = std::make_shared<MRLabel>();
    caption->setText(L"Current / Recent", "default");
    caption->setFontSize(12.0f);
    caption->setFontColor(0.70f, 0.74f, 0.80f, 1.0f);
    caption->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    caption->getTransform()->setPosition(42.0f, 68.0f, 0.0f);
    caption->getTransform()->setSize(160.0f, 24.0f);
    colorPopupContent->addChild(caption);
    for (size_t index = 0; index < recentColors.size() && index < 8; ++index)
        addSwatch(parseColorValue(recentColors[index]), 10.0f + static_cast<float>(index) * 29.0f, 98.0f);

    // 定位：世界坐标换算 shellRoot 局部系（左上原点），向左展开并夹在窗口内
    const Math::Rect rootBounds = m_shell.m_shellRoot->getWorldSpaceAABB();
    float localX = worldX - rootBounds.Min.x - 248.0f;
    float localY = rootBounds.Max.y - worldY;
    localX = std::clamp(localX, 4.0f, std::max(4.0f, rootBounds.GetWidth() - 252.0f));
    localY = std::clamp(localY, 4.0f, std::max(4.0f, rootBounds.GetHeight() - 212.0f));
    colorPopup->getTransform()->setSize(248.0f, 208.0f);
    colorPopup->popup(localX, localY);
    m_shell.setStatus("Pick a color for " + property.displayName);
}

}  // namespace morrow::editor
