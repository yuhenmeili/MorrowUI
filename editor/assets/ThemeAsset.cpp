#include "assets/ThemeAsset.h"

#include <fstream>

namespace {
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

/// 主题基类继承：子类控件没有专属表时回落到基类表
/// （applyProperty 按 cast 分发，基类属性对子类控件天然可应用）。
std::string themeBaseType(const std::string& type) {
    if (type == "MRCheckBox" || type == "MRCheckButton" || type == "MRToggle" || type == "MRRadioButton" || type == "MRMenuButton" || type == "MROptionButton" ||
        type == "MRSelectableButton" || type == "MRTextureButton")
        return "MRButton";
    if (type == "MRSlider")
        return "MRProgressBar";
    if (type == "MRHSeparator" || type == "MRVSeparator")
        return "MRColor";
    if (type == "MRLineEdit")
        return "MRTextEdit";
    return {};
}
}  // namespace

namespace morrow::editor {

bool ThemeAsset::loadFromFile(const std::filesystem::path& path, ThemeAsset& theme, std::string& error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        error = "failed to open theme file: " + path.string();
        return false;
    }

    ThemeAsset parsed;
    std::string line;
    std::string currentType;
    while (std::getline(input, line)) {
        const auto content = trim(line);
        if (content.empty() || content[0] == '#')
            continue;
        if (content.front() == '[' && content.back() == ']') {
            currentType = trim(content.substr(1, content.size() - 2));
            if (currentType.empty()) {
                error = "theme section name cannot be empty";
                return false;
            }
            if (currentType != "theme")
                parsed.typeProperties[currentType];
            continue;
        }
        const auto assignment = content.find('=');
        if (assignment == std::string::npos) {
            error = "theme property requires '=': " + content;
            return false;
        }
        const auto key = trim(content.substr(0, assignment));
        const auto value = trim(content.substr(assignment + 1));
        if (key.empty() || value.empty()) {
            error = "theme property requires a name and value: " + content;
            return false;
        }
        if (currentType.empty() || currentType == "theme") {
            // [theme] 头只放 format 等元数据，属性忽略
            continue;
        }
        parsed.typeProperties[currentType][key] = value;
    }
    theme = std::move(parsed);
    return true;
}

void ThemeAsset::applyDefaults(SceneNodeRecord& record) const {
    // 先精确类型表，再基类回落表（均不覆盖节点显式属性）
    const auto applyTable = [this, &record](const std::string& type) {
        const auto typeEntry = typeProperties.find(type);
        if (typeEntry == typeProperties.end())
            return;
        for (const auto& [key, value] : typeEntry->second)
            record.properties.emplace(key, value);
    };
    applyTable(record.type);
    if (const std::string base = themeBaseType(record.type); !base.empty() && base != record.type)
        applyTable(base);
}

}  // namespace morrow::editor
