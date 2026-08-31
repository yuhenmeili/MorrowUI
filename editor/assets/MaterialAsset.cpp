#include "MaterialAsset.h"

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

std::string unquote(const std::string& value) {
    const auto text = trim(value);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
        return text.substr(1, text.size() - 2);
    return text;
}

std::string quote(const std::string& value) {
    std::string result = "\"";
    for (const char c : value) {
        if (c == '"' || c == '\\')
            result.push_back('\\');
        result.push_back(c);
    }
    result.push_back('"');
    return result;
}
}  // namespace

namespace morrow::editor {

bool loadMaterialAsset(const std::filesystem::path& path, MaterialAsset& material, std::string& error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        error = "failed to open material: " + path.string();
        return false;
    }
    MaterialAsset parsed;
    std::string section;
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line.front() == '#')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            error = "material line " + std::to_string(lineNumber) + " requires '='";
            return false;
        }
        const auto key = trim(line.substr(0, separator));
        const auto value = unquote(line.substr(separator + 1));
        if (section == "material") {
            if (key == "format")
                parsed.format = std::stoi(value);
            else if (key == "shader")
                parsed.shader = value;
        } else if (section == "properties") {
            parsed.properties[key] = value;
        }
    }
    if (parsed.format != 1) {
        error = "material must define format=1";
        return false;
    }
    material = std::move(parsed);
    return true;
}

bool saveMaterialAsset(const std::filesystem::path& path, const MaterialAsset& material, std::string& error) {
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        error = "failed to write material: " + path.string();
        return false;
    }
    output << "[material]\nformat = " << material.format << "\nshader = " << quote(material.shader) << "\n\n[properties]\n";
    for (const auto& [key, value] : material.properties)
        output << key << " = " << value << '\n';
    if (!output.good()) {
        error = "failed while writing material: " + path.string();
        return false;
    }
    return true;
}

}  // namespace morrow::editor
