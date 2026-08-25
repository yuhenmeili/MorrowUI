#include "SceneDocument.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
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

std::string quote(const std::string& value) {
    std::ostringstream output;
    output << '"';
    for (const char character : value) {
        switch (character) {
            case '\\':
                output << "\\\\";
                break;
            case '"':
                output << "\\\"";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << character;
                break;
        }
    }
    output << '"';
    return output.str();
}

bool isNumber(const std::string& value) {
    if (value.empty())
        return false;
    char* end = nullptr;
    std::strtod(value.c_str(), &end);
    return end && *end == '\0';
}

bool isTypedValue(const std::string& value) {
    static const std::vector<std::string> prefixes = {"Vector2(", "Vector3(", "Vector4(", "Color(", "Quaternion(", "Rect(", "Enum(", "resource(", "sub_resource("};
    return std::any_of(prefixes.begin(), prefixes.end(),
                       [&value](const std::string& prefix) { return value.compare(0, prefix.size(), prefix) == 0 && !value.empty() && value.back() == ')'; });
}

std::string serializePropertyValue(const std::string& value) {
    if (value == "true" || value == "false" || isNumber(value) || isTypedValue(value)) {
        return value;
    }
    return quote(value);
}

bool readQuoted(const std::string& input, size_t& cursor, std::string& value) {
    if (cursor >= input.size() || input[cursor] != '"')
        return false;
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
                case 'n':
                    output << '\n';
                    break;
                case 'r':
                    output << '\r';
                    break;
                case 't':
                    output << '\t';
                    break;
                case '\\':
                    output << '\\';
                    break;
                case '"':
                    output << '"';
                    break;
                default:
                    output << escaped;
                    break;
            }
        } else {
            output << character;
        }
    }
    return false;
}

bool parseAttributes(const std::string& input, std::map<std::string, std::string>& attributes, std::string& error) {
    size_t cursor = 0;
    while (cursor < input.size()) {
        while (cursor < input.size() && std::isspace(static_cast<unsigned char>(input[cursor]))) {
            ++cursor;
        }
        if (cursor >= input.size())
            break;

        const size_t keyStart = cursor;
        while (cursor < input.size() && !std::isspace(static_cast<unsigned char>(input[cursor])) && input[cursor] != '=') {
            ++cursor;
        }
        const auto key = input.substr(keyStart, cursor - keyStart);
        if (key.empty()) {
            error = "empty attribute name";
            return false;
        }
        while (cursor < input.size() && std::isspace(static_cast<unsigned char>(input[cursor]))) {
            ++cursor;
        }
        if (cursor >= input.size() || input[cursor] != '=') {
            error = "expected '=' after attribute '" + key + "'";
            return false;
        }
        ++cursor;
        while (cursor < input.size() && std::isspace(static_cast<unsigned char>(input[cursor]))) {
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
            while (cursor < input.size() && !std::isspace(static_cast<unsigned char>(input[cursor]))) {
                ++cursor;
            }
            value = input.substr(valueStart, cursor - valueStart);
        }
        attributes[key] = value;
    }
    return true;
}

}  // namespace

namespace morrow::editor {

bool SceneDocument::loadFromFile(const std::filesystem::path& path, SceneDocument& document, std::string& error) {
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
        if (content.empty())
            continue;

        if (content.front() == '[' && content.back() == ']') {
            const auto section = content.substr(1, content.size() - 2);
            const auto separator = section.find_first_of(" \t");
            const auto sectionName = section.substr(0, separator);
            const auto attributesText = separator == std::string::npos ? "" : trim(section.substr(separator));
            std::map<std::string, std::string> attributes;
            if (!parseAttributes(attributesText, attributes, error)) {
                error = "line " + std::to_string(lineNumber) + ": " + error;
                return false;
            }

            if (sectionName == "morrow_scene") {
                if (attributes["format"] != "1") {
                    error = "line " + std::to_string(lineNumber) + ": unsupported scene format";
                    return false;
                }
                currentNode = nullptr;
                currentSubResource = nullptr;
            } else if (sectionName == "external_resource") {
                SceneResourceRecord resource;
                resource.id = attributes["id"];
                resource.assetId = attributes["asset_id"];
                resource.type = attributes["type"];
                resource.path = attributes["path"];
                if (resource.id.empty() || resource.type.empty() || (resource.assetId.empty() && resource.path.empty())) {
                    error = "line " + std::to_string(lineNumber) + ": external_resource requires id, type and asset_id or path";
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
                    error = "line " + std::to_string(lineNumber) + ": node requires id, type and name";
                    return false;
                }
                if (std::any_of(parsed.m_nodes.begin(), parsed.m_nodes.end(), [&node](const SceneNodeRecord& existing) { return existing.id == node.id; })) {
                    error = "line " + std::to_string(lineNumber) + ": duplicate node id '" + node.id + "'";
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
                    error = "line " + std::to_string(lineNumber) + ": sub_resource requires id and type";
                    return false;
                }
                if (std::any_of(parsed.m_subResources.begin(), parsed.m_subResources.end(),
                                [&resource](const SceneSubResourceRecord& existing) { return existing.id == resource.id; })) {
                    error = "line " + std::to_string(lineNumber) + ": duplicate sub_resource id '" + resource.id + "'";
                    return false;
                }
                parsed.m_subResources.push_back(std::move(resource));
                currentNode = nullptr;
                currentSubResource = &parsed.m_subResources.back();
            } else {
                error = "line " + std::to_string(lineNumber) + ": unsupported section '" + sectionName + "'";
                return false;
            }
            continue;
        }

        if ((!currentNode && !currentSubResource) || content.compare(0, 9, "property ") != 0) {
            error = "line " + std::to_string(lineNumber) + ": expected a property inside a node";
            return false;
        }
        const auto assignment = content.find('=');
        if (assignment == std::string::npos) {
            error = "line " + std::to_string(lineNumber) + ": property requires '='";
            return false;
        }
        const auto propertyName = trim(content.substr(9, assignment - 9));
        auto propertyValue = trim(content.substr(assignment + 1));
        if (propertyName.empty() || propertyValue.empty()) {
            error = "line " + std::to_string(lineNumber) + ": property requires a name and value";
            return false;
        }
        if (propertyValue.size() >= 2 && propertyValue.front() == '"' && propertyValue.back() == '"') {
            size_t cursor = 0;
            std::string unquoted;
            if (!readQuoted(propertyValue, cursor, unquoted) || cursor != propertyValue.size()) {
                error = "line " + std::to_string(lineNumber) + ": invalid quoted property value";
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

bool SceneDocument::saveToFile(const std::filesystem::path& path, std::string& error) const {
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        error = "failed to open scene file for writing: " + path.string();
        return false;
    }

    output << "[morrow_scene format=1]\n";
    for (const auto& resource : m_externalResources) {
        output << "\n[external_resource id=" << quote(resource.id) << " type=" << quote(resource.type);
        if (!resource.assetId.empty()) {
            output << " asset_id=" << quote(resource.assetId);
        }
        if (!resource.path.empty()) {
            output << " path=" << quote(resource.path);
        }
        output << "]\n";
    }
    for (const auto& resource : m_subResources) {
        output << "\n[sub_resource type=" << quote(resource.type) << " id=" << quote(resource.id) << "]\n";
        for (const auto& [property, value] : resource.properties) {
            output << "property " << property << " = " << serializePropertyValue(value) << "\n";
        }
    }
    for (const auto& node : m_nodes) {
        output << "\n[node id=" << quote(node.id) << " type=" << quote(node.type);
        if (!node.parentId.empty()) {
            output << " parent=" << quote(node.parentId);
        }
        output << " name=" << quote(node.name) << "]\n";
        for (const auto& [property, value] : node.properties) {
            output << "property " << property << " = " << serializePropertyValue(value) << "\n";
        }
    }
    if (!output.good()) {
        error = "failed while writing scene file: " + path.string();
        return false;
    }
    return true;
}

SceneNodeRecord* SceneDocument::findNode(const std::string& id) {
    const auto iterator = std::find_if(m_nodes.begin(), m_nodes.end(), [&id](const SceneNodeRecord& node) { return node.id == id; });
    return iterator == m_nodes.end() ? nullptr : &*iterator;
}

const SceneNodeRecord* SceneDocument::findNode(const std::string& id) const {
    const auto iterator = std::find_if(m_nodes.begin(), m_nodes.end(), [&id](const SceneNodeRecord& node) { return node.id == id; });
    return iterator == m_nodes.end() ? nullptr : &*iterator;
}

const SceneResourceRecord* SceneDocument::findExternalResource(const std::string& id) const {
    const auto iterator = std::find_if(m_externalResources.begin(), m_externalResources.end(), [&id](const SceneResourceRecord& resource) { return resource.id == id; });
    return iterator == m_externalResources.end() ? nullptr : &*iterator;
}

const SceneSubResourceRecord* SceneDocument::findSubResource(const std::string& id) const {
    const auto iterator = std::find_if(m_subResources.begin(), m_subResources.end(), [&id](const SceneSubResourceRecord& resource) { return resource.id == id; });
    return iterator == m_subResources.end() ? nullptr : &*iterator;
}

bool SceneDocument::setNodeProperty(const std::string& nodeId, const std::string& property, std::string value, std::string& error) {
    auto node = findNode(nodeId);
    if (!node) {
        error = "node '" + nodeId + "' was not found";
        return false;
    }
    if (property.empty()) {
        error = "property name cannot be empty";
        return false;
    }
    node->properties[property] = std::move(value);
    return true;
}

bool SceneDocument::removeNodeProperty(const std::string& nodeId, const std::string& property, std::string& error) {
    auto node = findNode(nodeId);
    if (!node) {
        error = "node '" + nodeId + "' was not found";
        return false;
    }
    node->properties.erase(property);
    return true;
}

bool SceneDocument::reparentNode(const std::string& nodeId, const std::string& parentId, std::string& error) {
    auto node = findNode(nodeId);
    if (!node) {
        error = "node '" + nodeId + "' was not found";
        return false;
    }
    if (nodeId == parentId) {
        error = "a node cannot be parented to itself";
        return false;
    }
    if (!parentId.empty() && !findNode(parentId)) {
        error = "parent node '" + parentId + "' was not found";
        return false;
    }

    for (auto ancestor = findNode(parentId); ancestor; ancestor = ancestor->parentId.empty() ? nullptr : findNode(ancestor->parentId)) {
        if (ancestor->id == nodeId) {
            error = "reparenting would create a cycle";
            return false;
        }
    }
    node->parentId = parentId;
    return true;
}

bool SceneDocument::renameNode(const std::string& nodeId, std::string name, std::string& error) {
    auto node = findNode(nodeId);
    if (!node) {
        error = "node '" + nodeId + "' was not found";
        return false;
    }
    if (name.empty()) {
        error = "node name cannot be empty";
        return false;
    }
    const bool duplicate = std::any_of(m_nodes.begin(), m_nodes.end(), [&nodeId, &name, &node](const SceneNodeRecord& existing) {
        return existing.id != nodeId && existing.parentId == node->parentId && existing.name == name;
    });
    if (duplicate) {
        error = "a sibling node named '" + name + "' already exists";
        return false;
    }
    node->name = std::move(name);
    return true;
}

bool SceneDocument::addNode(SceneNodeRecord node, std::string& error) {
    if (node.id.empty() || node.type.empty() || node.name.empty()) {
        error = "node requires id, type and name";
        return false;
    }
    if (findNode(node.id)) {
        error = "node '" + node.id + "' already exists";
        return false;
    }
    if (!node.parentId.empty() && !findNode(node.parentId)) {
        error = "parent node '" + node.parentId + "' was not found";
        return false;
    }
    m_nodes.push_back(std::move(node));
    return true;
}

bool SceneDocument::removeNodeSubtree(const std::string& nodeId, std::vector<SceneNodeRecord>& removed, std::string& error) {
    if (!findNode(nodeId)) {
        error = "node '" + nodeId + "' was not found";
        return false;
    }
    std::vector<std::string> ids{nodeId};
    for (size_t index = 0; index < ids.size(); ++index) {
        for (const auto& node : m_nodes) {
            if (node.parentId == ids[index])
                ids.push_back(node.id);
        }
    }
    for (const auto& id : ids) {
        const auto node = findNode(id);
        if (node)
            removed.push_back(*node);
    }
    m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), [&ids](const SceneNodeRecord& node) { return std::find(ids.begin(), ids.end(), node.id) != ids.end(); }),
                  m_nodes.end());
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

}  // namespace morrow::editor
