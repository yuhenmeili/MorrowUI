#ifndef MORROW_EDITOR_PROJECT_SETTINGS_H
#define MORROW_EDITOR_PROJECT_SETTINGS_H

#include <filesystem>
#include <map>
#include <string>

namespace morrow::editor {

class ProjectSettings {
public:
    bool load(const std::filesystem::path& projectPath, std::string& error);

    const std::filesystem::path& projectPath() const;

    const std::filesystem::path& projectRoot() const;

    std::string value(const std::string& name, const std::string& fallback = {}) const;

    std::filesystem::path pathValue(const std::string& name, const std::filesystem::path& fallback = {}) const;

    bool validate(std::string& error) const;

private:
    std::filesystem::path m_projectPath;
    std::filesystem::path m_projectRoot;
    std::map<std::string, std::string> m_values;
};

}  // namespace morrow::editor

#endif
