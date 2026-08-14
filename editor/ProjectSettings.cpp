#include "ProjectSettings.h"

#include <fstream>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}
}  // namespace

namespace morrow::editor {

bool ProjectSettings::load(const std::filesystem::path& projectPath, std::string& error) {
    std::ifstream input(projectPath);
    if (!input.is_open()) {
        error = "failed to open project: " + projectPath.string();
        return false;
    }
    m_projectPath = std::filesystem::weakly_canonical(projectPath);
    m_projectRoot = m_projectPath.parent_path();
    m_values.clear();
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const auto content = trim(line);
        if (content.empty() || content[0] == '#' || content[0] == '[')
            continue;
        constexpr const char* prefix = "property ";
        if (content.rfind(prefix, 0) != 0)
            continue;
        const auto separator = content.find('=');
        if (separator == std::string::npos) {
            error = "project line " + std::to_string(lineNumber) + ": expected property name = value";
            return false;
        }
        auto name = trim(content.substr(9, separator - 9));
        auto valueText = trim(content.substr(separator + 1));
        if (valueText.size() >= 2 && valueText.front() == '"' && valueText.back() == '"')
            valueText = valueText.substr(1, valueText.size() - 2);
        m_values[std::move(name)] = std::move(valueText);
    }
    return validate(error);
}

const std::filesystem::path& ProjectSettings::projectPath() const {
    return m_projectPath;
}
const std::filesystem::path& ProjectSettings::projectRoot() const {
    return m_projectRoot;
}

std::string ProjectSettings::value(const std::string& name, const std::string& fallback) const {
    const auto iterator = m_values.find(name);
    return iterator == m_values.end() ? fallback : iterator->second;
}

std::filesystem::path ProjectSettings::pathValue(const std::string& name, const std::filesystem::path& fallback) const {
    const auto configured = value(name, fallback.string());
    return (m_projectRoot / configured).lexically_normal();
}

bool ProjectSettings::validate(std::string& error) const {
    const auto platform = value("platform", "windows");
    if (platform != "windows") {
        error = "MorrowEditor only supports Windows projects; configured platform is '" + platform + "'";
        return false;
    }
    if (value("preview_target").empty()) {
        error = "project has no preview_target";
        return false;
    }
    if (value("asset_root").empty()) {
        error = "project has no asset_root";
        return false;
    }
    return true;
}

}  // namespace morrow::editor
