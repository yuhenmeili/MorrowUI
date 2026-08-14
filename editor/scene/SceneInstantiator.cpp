#include "SceneInstantiator.h"

#include <sstream>
#include <unordered_map>
#include <vector>

#include "SceneDocument.h"
#include "Vector3.h"
#include "Vector4.h"
#include "base/SceneNode.h"
#include "base/Transform.h"
#include "base/UIWidget.h"
#include "elements/MRButton.h"
#include "elements/MRImage.h"
#include "elements/MRLabel.h"

namespace {

using morrow::Math::Vector3;
using morrow::Math::Vector4;

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

bool applyProperty(const morrow::editor::SceneNodeRecord& record, const std::shared_ptr<morrow::Widget>& widget, const std::string& key, const std::string& value,
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
            button->setBackgroundColor(Vector4(components[0], components[1], components[2], components[3]));
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

bool SceneInstantiator::instantiate(const SceneDocument& document, const std::shared_ptr<Widget>& stage, std::string& error) {
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
        for (const auto& [key, value] : record.properties) {
            if (!applyProperty(record, instance, key, value, error)) {
                error = "line " + std::to_string(record.line) + ": " + error;
                return false;
            }
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
    return true;
}

}  // namespace morrow::editor
