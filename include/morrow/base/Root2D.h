//
// Root2D — 2D UI 树的根节点（公共）。
//
// 引擎内部维护的窗口不再对外暴露：应用经 Engine::getRootWidget() 取得本根，
// 向其 addChild 搭建界面。Root2D 本身是轻量 UIWidget 容器，后续如需输入
// 路由 / 分层管理等根级能力在此扩展。
// （Window 与 UI 根解耦：docs/road_map/PUBLIC_API_ENCAPSULATION_PROPOSAL.md §6.7）
//

#ifndef MORROW_BASE_ROOT2D_H
#define MORROW_BASE_ROOT2D_H

#include "morrow/base/UIWidget.h"

namespace morrow {

class Root2D : public UIWidget {
public:
    Root2D() : UIWidget(false) {
        setWidgetName("Root2D");
    }
};

using Root2DSharedPtr = std::shared_ptr<Root2D>;

} // namespace morrow

#endif //MORROW_BASE_ROOT2D_H
