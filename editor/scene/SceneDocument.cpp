#include "SceneDocument.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "base/SceneNode.h"
#include "base/Transform.h"
#include "base/UIWidget.h"
#include "elements/MRButton.h"
#include "elements/MRImage.h"
#include "elements/MRLabel.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

namespace {

using morrow::Math::Vector2;
using morrow::Math::Vector3;
using morrow::Math::Vector4;

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string stripComment(const std::string& value) {
    bool quoted = false;
    for (size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '"' && (index == 0 || value[index - 1] != '\\')) {
            quoted = !quoted;
        } else if (value[index] == '#' && !quoted) {
            return value.substr(0, index);
        }
    }
    return value;
}

bool readQuoted(const std::string& input, size_t& cursor, std::string& value) {
    if (cursor >= input.size() || input[cursor] != '"') return false;
    ++cursor;
    std::ostringstream output;
    while (cursor < input.size()) {
        const char character = input[cursor++];
        if (character == '"') {
            value = output.str();
            return true;
        }
        if (character == '\\' && cursor < input.size()) {
            const char escaped = input[cursor++];
            switch (escaped) {
                case 'n': output << '\n'; break;
                case 'r': output << '\r'; break;
                case 't': output << '\t'; break;
                case '\\': output << '\\'; break;
                case '"': output << '"'; break;
                default: output << escaped; break;
            }
        } else {
            output << character;
        }
    }
    return false;
}

bool parseAttributes(const std::string& input,
                     std::map<std::string, std::string>& attributes,
                     std::string& error) {
    size_t cursor = 0;
    while (cursor < input.size()) {
        while (cursor < input.size() &&
               std::isspace(static_cast<unsigned char>(input[cursor]))) {
            ++cursor;
        }
        if (cursor >= input.size()) break;

        const size_t keyStart = cursor;
        while (cursor < input.size() &&
               !std::isspace(static_cast<unsigned char>(input[cursor])) &&
               input[cursor] != '=') {
            ++cursor;
        }
        const auto key = input.substr(keyStart, cursor - keyStart);
        if (key.empty()) {
            error = "empty attribute name";
            return false;
        }
        while (cursor < input.size() &&
               std::isspace(static_cast<unsigned char>(input[cursor]))) {
            ++cursor;
        }
        if (cursor >= input.size() || input[cursor] != '=') {
            error = "expected '=' after attribute '" + key + "'";
            return false;
        }
        ++cursor;
        while (cursor < input.size() &&
               std::isspace(static_cast<unsigned char>(input[cursor]))) {
            ++cursor;
        }

        std::string value;
        if (cursor < input.size() && input[cursor] == '"') {
            if (!readQuoted(input, cursor, value)) {
                error = "unterminated quoted value for '" + key + "'";
                return false;
            }
        } else {
            const size_t valueStart = cursor;
            while (cursor < input.size() &&
                   !std::isspace(static_cast<unsigned char>(input[cursor]))) {
                ++cursor;
            }
            value = input.substr(valueStart, cursor - valueStart);
        }
        attributes[key] = value;
    }
    return true;
}

bool parseVector(const std::string& value, const std::string& type,
                 std::vector<float>& components) {
    if (value.size() <= type.size() + 2 ||
        value.compare(0, type.size(), type) != 0 ||
        value[type.size()] != '(' ||
        value.back() != ')') {
        return false;
    }

    std::string body = value.substr(type.size() + 1, value.size() - type.size() - 2);
    std::stringstream stream(body);
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
    // Phase 1 accepts ASCII text. UTF-8 conversion belongs in the font/text pass.
    return std::wstring(value.begin(), value.end());
}

std::shared_ptr<morrow::Widget> createNode(const morrow::editor::SceneNodeRecord& record,
                                           std::string& error) {
    if (record.type == "SceneNode") {
        return std::make_shared<morrow::SceneNode>();
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
    error = "unsupported scene node type '" + record.type + "'";
    return nullptr;
}

bool applyProperty(const morrow::editor::SceneNodeRecord& record,
                   const std::shared_ptr<morrow::Widget>& widget,
                   const std::string& key,
                   const std::string& value,
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
            button->setBackgroundColor(Vector4(components[0], components[1],
                                                components[2], components[3]));
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

} // namespace

namespace morrow::editor {

bool SceneDocument::loadFromFile(const std::filesystem::path& path,
                                 SceneDocument& document,
                                 std::string& error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        error = "failed to open scene file: " + path.string();
        return false;
    }

    SceneDocument parsed;
    std::string line;
    size_t lineNumber = 0;
    SceneNodeRecord* currentNode = nullptr;
    SceneSubResourceRecord* currentSubResource = nullptr;
    while (std::getline(input, line)) {
        ++lineNumber;
        const auto content = trim(stripComment(line));
        if (content.empty()) continue;

        if (content.front() == '[' && content.back() == ']') {
            const auto section = content.substr(1, content.size() - 2);
            const auto separator = section.find_first_of(" \t");
            const auto sectionName = section.substr(0, separator);
            const auto attributesText =
                separator == std::string::npos ? "" : trim(section.substr(separator));
            std::map<std::string, std::string> attributes;
            if (!parseAttributes(attributesText, attributes, error)) {
                error = "line " + std::to_string(lineNumber) + ": " + error;
                return false;
            }

            if (sectionName == "morrow_scene") {
                if (attributes["format"] != "1") {
                    error = "line " + std::to_string(lineNumber) +
                            ": unsupported scene format";
                    return false;
                }
                currentNode = nullptr;
                currentSubResource = nullptr;
            } else if (sectionName == "external_resource") {
                SceneResourceRecord resource;
                resource.id = attributes["id"];
                resource.type = attributes["type"];
                resource.path = attributes["path"];
                if (resource.id.empty() || resource.type.empty() || resource.path.empty()) {
                    error = "line " + std::to_string(lineNumber) +
                            ": external_resource requires id, type and path";
                    return false;
                }
                parsed.m_externalResources.push_back(std::move(resource));
                currentNode = nullptr;
                currentSubResource = nullptr;
            } else if (sectionName == "node") {
                SceneNodeRecord node;
                node.id = attributes["id"];
                node.type = attributes["type"];
                node.parentId = attributes["parent"];
                node.name = attributes["name"];
                node.line = lineNumber;
                if (node.id.empty() || node.type.empty() || node.name.empty()) {
                    error = "line " + std::to_string(lineNumber) +
                            ": node requires id, type and name";
                    return false;
                }
                if (std::any_of(parsed.m_nodes.begin(), parsed.m_nodes.end(),
                                [&node](const SceneNodeRecord& existing) {
                                    return existing.id == node.id;
                                })) {
                    error = "line " + std::to_string(lineNumber) +
                            ": duplicate node id '" + node.id + "'";
                    return false;
                }
                parsed.m_nodes.push_back(std::move(node));
                currentNode = &parsed.m_nodes.back();
                currentSubResource = nullptr;
            } else if (sectionName == "sub_resource") {
                SceneSubResourceRecord resource;
                resource.id = attributes["id"];
                resource.type = attributes["type"];
                resource.line = lineNumber;
                if (resource.id.empty() || resource.type.empty()) {
                    error = "line " + std::to_string(lineNumber) +
                            ": sub_resource requires id and type";
                    return false;
                }
                if (std::any_of(parsed.m_subResources.begin(), parsed.m_subResources.end(),
                                [&resource](const SceneSubResourceRecord& existing) {
                                    return existing.id == resource.id;
                                })) {
                    error = "line " + std::to_string(lineNumber) +
                            ": duplicate sub_resource id '" + resource.id + "'";
                    return false;
                }
                parsed.m_subResources.push_back(std::move(resource));
                currentNode = nullptr;
                currentSubResource = &parsed.m_subResources.back();
            } else {
                error = "line " + std::to_string(lineNumber) +
                        ": unsupported section '" + sectionName + "'";
                return false;
            }
            continue;
        }

        if ((!currentNode && !currentSubResource) ||
            content.compare(0, 9, "property ") != 0) {
            error = "line " + std::to_string(lineNumber) +
                    ": expected a property inside a node";
            return false;
        }
        const auto assignment = content.find('=');
        if (assignment == std::string::npos) {
            error = "line " + std::to_string(lineNumber) +
                    ": property requires '='";
            return false;
        }
        const auto propertyName = trim(content.substr(9, assignment - 9));
        auto propertyValue = trim(content.substr(assignment + 1));
        if (propertyName.empty() || propertyValue.empty()) {
            error = "line " + std::to_string(lineNumber) +
                    ": property requires a name and value";
            return false;
        }
        if (propertyValue.size() >= 2 && propertyValue.front() == '"' &&
            propertyValue.back() == '"') {
            size_t cursor = 0;
            std::string unquoted;
            if (!readQuoted(propertyValue, cursor, unquoted) ||
                cursor != propertyValue.size()) {
                error = "line " + std::to_string(lineNumber) +
                        ": invalid quoted property value";
                return false;
            }
            propertyValue = std::move(unquoted);
        }
        if (currentNode) {
            currentNode->properties[propertyName] = std::move(propertyValue);
        } else {
            currentSubResource->properties[propertyName] = std::move(propertyValue);
        }
    }

    if (parsed.m_nodes.empty()) {
        error = "scene contains no nodes";
        return false;
    }
    document = std::move(parsed);
    return true;
}

bool SceneDocument::instantiate(const std::shared_ptr<Widget>& stage,
                                std::string& error) const {
    if (!stage) {
        error = "cannot instantiate scene without a stage widget";
        return false;
    }

    std::unordered_map<std::string, std::shared_ptr<Widget>> instances;
    instances.reserve(m_nodes.size());
    for (const auto& record : m_nodes) {
        auto instance = createNode(record, error);
        if (!instance) {
            error = "line " + std::to_string(record.line) + ": " + error;
            return false;
        }
        instance->setWidgetName(record.name);
        for (const auto& [key, value] : record.properties) {
            if (!applyProperty(record, instance, key, value, error)) {
                error = "line " + std::to_string(record.line) + ": " + error;
                return false;
            }
        }
        instances.emplace(record.id, std::move(instance));
    }

    for (const auto& record : m_nodes) {
        const auto instance = instances.at(record.id);
        if (record.parentId.empty()) {
            stage->addChild(instance);
            continue;
        }
        const auto parent = instances.find(record.parentId);
        if (parent == instances.end()) {
            error = "line " + std::to_string(record.line) +
                    ": parent node '" + record.parentId + "' was not found";
            return false;
        }
        parent->second->addChild(instance);
    }
    return true;
}

const std::vector<SceneResourceRecord>& SceneDocument::externalResources() const {
    return m_externalResources;
}

const std::vector<SceneSubResourceRecord>& SceneDocument::subResources() const {
    return m_subResources;
}

const std::vector<SceneNodeRecord>& SceneDocument::nodes() const {
    return m_nodes;
}

} // namespace morrow::editor
