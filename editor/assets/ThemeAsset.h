#ifndef MORROW_EDITOR_THEME_ASSET_H
#define MORROW_EDITOR_THEME_ASSET_H

#include <filesystem>
#include <map>
#include <string>

#include "scene/SceneDocument.h"

namespace morrow::editor {

/// 主题资产（.theme）：按节点类型组织的属性默认值表。
/// 格式（类 INI）：
///   [theme]
///   format = 1
///
///   [MRButton]
///   background_color = Color(0.16, 0.31, 0.63, 1.0)
///
///   [MRLabel]
///   font_color = Color(1.0, 1.0, 1.0, 1.0)
///
/// 应用时机：场景实例化时，节点未显式设置的属性先取主题默认值
/// （节点自身属性仍然优先，等同 Godot 的 theme 与 override 关系）。
struct ThemeAsset {
    /// type → (property → 序列化值)
    std::map<std::string, std::map<std::string, std::string>> typeProperties;

    static bool loadFromFile(const std::filesystem::path& path, ThemeAsset& theme, std::string& error);

    bool empty() const {
        return typeProperties.empty();
    }

    /// 把主题默认值 emplace 进节点属性（不覆盖已显式设置的键）。
    void applyDefaults(SceneNodeRecord& record) const;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_THEME_ASSET_H
