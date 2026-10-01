//
// Scene2D — 2D 场景根节点（2D UI 树根，公共）。
//
// 引擎内部维护的窗口不再对外暴露：应用经 Engine::getScene2D() 取得本根，
// 向其 addChild 搭建界面（对象添加到场景的通用心智模型）。Scene2D 本身是
// 轻量 UIWidget 容器，后续如需输入路由 / 分层管理等根级能力在此扩展。
// 3D 场景树节点是 SceneNode3D（经 MR3DSceneView 装入 2D 场景），两者分属
// 不同树。
// （Window 与 UI 根解耦：docs/road_map/PUBLIC_API_ENCAPSULATION_PROPOSAL.md §6.7）
//

#ifndef MORROW_BASE_SCENE2D_H
#define MORROW_BASE_SCENE2D_H

#include "morrow/base/UIWidget.h"

namespace morrow {

class Scene2D : public UIWidget {
public:
    Scene2D() : UIWidget(false) {
        setWidgetName("Scene2D");
    }
};

using Scene2DSharedPtr = std::shared_ptr<Scene2D>;

} // namespace morrow

#endif //MORROW_BASE_SCENE2D_H
