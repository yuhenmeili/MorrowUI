#ifndef MORROW_EDITOR_ANIMATION_PANEL_H
#define MORROW_EDITOR_ANIMATION_PANEL_H

#include <memory>
#include <string>
#include <vector>

#include "morrow/core/Observable.h"
#include "morrow/base/EventDispatcher.h"
#include "panels/EditorPanel.h"
#include "scene/AnimationService.h"

namespace morrow {
class MRButton;
class MRCheckBox;
class MRColor;
class MRLabel;
class MRLineEdit;
class MRTextEdit;
class TouchEvent;
class UIWidget;
}  // namespace morrow

namespace morrow::editor {
class EditorShell;

/// 动画 Dock（底部页签）：transport + 播放头拖动 + 轨道/关键帧（菱形）编辑。
/// 编辑写回场景 sub_resource（不经命令历史，MVP 限制），预览经 AnimationService 直写运行时 Widget。
class AnimationPanel : public EditorPanel {
public:
    explicit AnimationPanel(EditorShell& shell);

    /// 重建传输条与轨道行（场景重建/选择变化后调用）。
    void refresh();

    /// 每帧回调（播放中移动播放头）。
    void onTick();

    /// 指针交互（由 EditorShell 路由：命中本面板或拖拽进行中）。
    void handlePointer(const TouchEvent& event);

private:
    friend class EditorShell;

    std::shared_ptr<UIWidget>& panel = m_root;
    std::shared_ptr<morrow::MRButton> playPauseButton;
    std::shared_ptr<morrow::MRButton> stopButton;
    std::shared_ptr<morrow::MRButton> keyButton;
    std::shared_ptr<morrow::MRButton> deleteKeyButton;
    std::shared_ptr<morrow::MRButton> loopButton;
    std::shared_ptr<morrow::MRButton> interpButton;
    std::shared_ptr<morrow::MRLineEdit> durationEdit;
    std::shared_ptr<morrow::MRLabel> timeLabel;
    std::shared_ptr<morrow::MRColor> playhead;
    std::shared_ptr<morrow::MRColor> trackBackground;
    std::vector<std::shared_ptr<morrow::MRLabel>> trackLabels;
    /// 轨道行复用的菱形键帧（track index → diamonds）。
    std::vector<std::vector<std::shared_ptr<morrow::MRColor>>> keyDiamonds;
    std::vector<Observable<morrow::MRTextEdit&, const std::wstring&>::Connection> editConnections;

    /// 当前编辑的播放器/片段（工作副本，写穿到文档）。
    std::string activePlayerId;
    std::string activeAnimationId;
    AnimationClip workingClip;
    bool workingValid = false;
    /// 选中的关键帧（点击菱形后）。
    int selectedTrack = -1;
    int selectedKey = -1;
    // 拖拽状态
    bool draggingPlayhead = false;
    bool draggingKey = false;
    float playheadTime = 0.0f;

    /// 面板局部几何：时间轴区。
    float timelineStartX() const;
    float timelineWidth() const;
    float trackStartY() const;
    float timeToX(float time) const;
    float xToTime(float x) const;

    /// 解析当前活动播放器（选中优先，否则文档第一个）。
    void resolveActivePlayer();

    /// 工作副本写回文档 sub_resource + 同步服务 + 重建轨道行。
    void commitClip();

    /// 只重建轨道行与菱形（不动传输条）。
    void rebuildTracks();

    /// 更新播放头位置与时间标签。
    void updatePlayheadVisual();

    /// Key Selected：为选中节点（非播放器）在播放头时刻打 position/rotation/scale 键。
    void keySelected();

    /// 删除选中的关键帧。
    void deleteSelectedKey();

    EditorShell& m_shell;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_ANIMATION_PANEL_H
