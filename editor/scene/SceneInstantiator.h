#ifndef MORROW_EDITOR_SCENE_INSTANTIATOR_H
#define MORROW_EDITOR_SCENE_INSTANTIATOR_H

#include <memory>
#include <string>
#include <unordered_map>

namespace morrow {
class Widget;

namespace editor {
class SceneDocument;
class AssetDatabase;
struct SceneNodeRecord;

using SceneInstanceMap =
    std::unordered_map<std::string, std::shared_ptr<Widget>>;

class SceneInstantiator {
public:
    static bool instantiate(const SceneDocument& document,
                            const std::shared_ptr<Widget>& stage,
                            const AssetDatabase* assets,
                            std::string& error,
                            SceneInstanceMap* instancesOut = nullptr);

    static bool updateNode(
        const SceneDocument& document,
        const SceneNodeRecord& record,
        const std::shared_ptr<Widget>& instance,
        const AssetDatabase* assets,
        std::string& error);

    static bool updateNodeTransform(
        const SceneNodeRecord& record,
        const std::shared_ptr<Widget>& instance,
        std::string& error);
};

}  // namespace editor
}  // namespace morrow

#endif  // MORROW_EDITOR_SCENE_INSTANTIATOR_H
