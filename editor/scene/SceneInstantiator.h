#ifndef MORROW_EDITOR_SCENE_INSTANTIATOR_H
#define MORROW_EDITOR_SCENE_INSTANTIATOR_H

#include <memory>
#include <string>

namespace morrow {
class Widget;

namespace editor {
class SceneDocument;

class SceneInstantiator {
public:
    static bool instantiate(const SceneDocument& document, const std::shared_ptr<Widget>& stage, std::string& error);
};

}  // namespace editor
}  // namespace morrow

#endif  // MORROW_EDITOR_SCENE_INSTANTIATOR_H
