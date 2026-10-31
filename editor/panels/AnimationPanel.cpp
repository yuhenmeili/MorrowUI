#include "panels/AnimationPanel.h"

#include <algorithm>
#include <cmath>
#include <codecvt>
#include <cstdio>
#include <locale>

#include "EditorShell.h"
#include "morrow/base/Interaction.h"
#include "morrow/base/TouchEvent.h"
#include "morrow/base/Transform.h"
#include "morrow/base/UIWidget.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRCheckBox.h"
#include "morrow/elements/MRColor.h"
#include "morrow/elements/MRLabel.h"
#include "morrow/elements/MRLineEdit.h"
#include "panels/SceneTreePanel.h"
#include "panels/ViewportPanel.h"
#include "scene/NodeTypeCatalog.h"

namespace {
std::wstring wide(const std::string& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
    } catch (...) {
        return std::wstring(text.begin(), text.end());
    }
}

std::string narrow(const std::wstring& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(text);
    } catch (...) {
        return std::string(text.begin(), text.end());
    }
}
}  // namespace

namespace morrow::editor {

AnimationPanel::AnimationPanel(EditorShell& shell) : m_shell(shell) {
}

float AnimationPanel::timelineStartX() const {
    return 170.0f;
}

float AnimationPanel::timelineWidth() const {
    const float panelWidth = panel ? panel->getTransform()->getSize().x : 800.0f;
    return std::max(120.0f, panelWidth - timelineStartX() - 16.0f);
}

float AnimationPanel::trackStartY() const {
    return 64.0f;
}

float AnimationPanel::timeToX(float time) const {
    const float duration = workingValid && workingClip.duration > 0.0f ? workingClip.duration : 1.0f;
    return timelineStartX() + std::clamp(time / duration, 0.0f, 1.0f) * timelineWidth();
}

float AnimationPanel::xToTime(float x) const {
    const float duration = workingValid && workingClip.duration > 0.0f ? workingClip.duration : 1.0f;
    return std::clamp((x - timelineStartX()) / timelineWidth(), 0.0f, 1.0f) * duration;
}

void AnimationPanel::resolveActivePlayer() {
    workingValid = false;
    activePlayerId.clear();
    activeAnimationId.clear();
    const auto& document = m_shell.m_session->document();
    const auto& selection = m_shell.m_session->model().selection().nodeIds;
    // 选中 MRAnimationPlayer 优先
    for (auto id = selection.rbegin(); id != selection.rend(); ++id) {
        const auto* node = document.findNode(*id);
        if (node && node->type == "MRAnimationPlayer") {
            activePlayerId = node->id;
            break;
        }
    }
    if (activePlayerId.empty()) {
        for (const auto& node : document.nodes()) {
            if (node.type == "MRAnimationPlayer") {
                activePlayerId = node.id;
                break;
            }
        }
    }
    if (activePlayerId.empty())
        return;
    const auto* player = document.findNode(activePlayerId);
    if (!player)
        return;
    const auto animationId = player->properties.find("animation_id");
    if (animationId == player->properties.end())
        return;
    activeAnimationId = animationId->second;
    if (activeAnimationId.empty())
        return;
    const auto* subResource = document.findSubResource(activeAnimationId);
    if (!subResource || subResource->type != "Animation")
        return;
    std::string error;
    if (!AnimationClip::fromSubResource(*subResource, workingClip, error))
        return;
    workingValid = true;
    selectedTrack = -1;
    selectedKey = -1;
    if (const auto* state = m_shell.m_animationService.find(activePlayerId))
        playheadTime = state->time;
    else
        playheadTime = 0.0f;
}

void AnimationPanel::commitClip() {
    if (!workingValid)
        return;
    m_shell.m_session->document().upsertSubResource(workingClip.toSubResource(activeAnimationId));
    m_shell.m_session->markDirty();
    m_shell.m_animationService.updateClip(activePlayerId, workingClip);
    rebuildTracks();
    updatePlayheadVisual();
}

void AnimationPanel::refresh() {
    if (!panel)
        return;
    // 传输条常驻（首次构建），之后只重建轨道区
    if (!playPauseButton) {
        playPauseButton = m_shell.addButton(panel, L"Play", 8.0f, 4.0f, 64.0f, 24.0f, [this] {
            if (!workingValid)
                return;
            const auto* state = m_shell.m_animationService.find(activePlayerId);
            const bool playing = state && state->playing;
            if (playing) {
                m_shell.m_animationService.pause(activePlayerId);
                playPauseButton->setText(L"Play", "default");
            } else {
                m_shell.m_animationService.play(activePlayerId);
                playPauseButton->setText(L"Pause", "default");
            }
        });
        stopButton = m_shell.addButton(panel, L"Stop", 76.0f, 4.0f, 56.0f, 24.0f, [this] {
            if (!workingValid)
                return;
            m_shell.m_animationService.stop(activePlayerId);
            playheadTime = 0.0f;
            m_shell.m_animationService.previewAt(
                activePlayerId, 0.0f, [this](const std::string& nodeId, const std::string& property, const std::string& value) {
                    m_shell.applyAnimationSample(nodeId, property, value);
                });
            updatePlayheadVisual();
            if (playPauseButton)
                playPauseButton->setText(L"Play", "default");
        });
        keyButton = m_shell.addButton(panel, L"Key Selected", 138.0f, 4.0f, 110.0f, 24.0f, [this] { keySelected(); });
        deleteKeyButton = m_shell.addButton(panel, L"Del Key", 252.0f, 4.0f, 76.0f, 24.0f, [this] { deleteSelectedKey(); });
        loopButton = m_shell.addButton(panel, L"Loop: On", 332.0f, 4.0f, 84.0f, 24.0f, [this] {
            workingClip.loop = !workingClip.loop;
            loopButton->setText(workingClip.loop ? L"Loop: On" : L"Loop: Off", "default");
            commitClip();
        });
        durationEdit = MRLineEdit::create();
        durationEdit->setFontSize(13.0f);
        durationEdit->setTextColor(Vector4(0.88f, 0.90f, 0.94f, 1.0f));
        durationEdit->setBackgroundColor(Vector4(0.075f, 0.085f, 0.105f, 1.0f));
        durationEdit->getTransform()->setPosition(452.0f, 4.0f, 0.0f);
        durationEdit->getTransform()->setSize(64.0f, 24.0f);
        editConnections.emplace_back(durationEdit->events().onSubmitted.connect([this](MRTextEdit&, const std::wstring& text) {
            try {
                const float value = std::stof(narrow(text));
                if (value > 0.05f && value <= 600.0f) {
                    workingClip.duration = value;
                    commitClip();
                }
            } catch (...) {
                m_shell.setStatus("Duration must be numeric");
            }
        }));
        panel->addChild(durationEdit);
        timeLabel = std::make_shared<MRLabel>();
        timeLabel->setText(L"0.000s", "default");
        timeLabel->setFontSize(13.0f);
        timeLabel->setFontColor(0.70f, 0.76f, 0.84f, 1.0f);
        timeLabel->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        timeLabel->getTransform()->setPosition(528.0f, 4.0f, 0.0f);
        timeLabel->getTransform()->setSize(120.0f, 24.0f);
        panel->addChild(timeLabel);
    }
    resolveActivePlayer();
    durationEdit->setText(workingValid ? wide(std::to_string(workingClip.duration)) : L"");
    loopButton->setText(workingValid && workingClip.loop ? L"Loop: On" : L"Loop: Off", "default");
    if (playPauseButton) {
        const auto* state = workingValid ? m_shell.m_animationService.find(activePlayerId) : nullptr;
        playPauseButton->setText(state && state->playing ? L"Pause" : L"Play", "default");
    }
    rebuildTracks();
    updatePlayheadVisual();
}

void AnimationPanel::rebuildTracks() {
    if (!panel)
        return;
    // 清掉轨道区旧控件（传输条 y<32 的不动）
    for (auto& diamonds : keyDiamonds) {
        for (const auto& diamond : diamonds)
            panel->removeChild(diamond);
    }
    for (const auto& label : trackLabels)
        panel->removeChild(label);
    if (trackBackground) {
        panel->removeChild(trackBackground);
        trackBackground.reset();
    }
    if (playhead) {
        panel->removeChild(playhead);
        playhead.reset();
    }
    keyDiamonds.clear();
    trackLabels.clear();

    if (!workingValid) {
        auto hint = std::make_shared<MRLabel>();
        hint->setText(L"No Animation Player in scene: select a node and press Key Selected to create one.", "default");
        hint->setFontSize(14.0f);
        hint->setFontColor(0.60f, 0.64f, 0.72f, 1.0f);
        hint->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        hint->getTransform()->setPosition(12.0f, 40.0f, 0.0f);
        hint->getTransform()->setSize(700.0f, 24.0f);
        panel->addChild(hint);
        trackLabels.push_back(hint);  // 复用清理通道
        return;
    }

    const float panelWidth = panel->getTransform()->getSize().x;
    trackBackground = MRColor::create();
    trackBackground->setColor(Vector4(0.085f, 0.095f, 0.12f, 1.0f));
    trackBackground->getTransform()->setPosition(timelineStartX() - 4.0f, 36.0f, 0.0f);
    trackBackground->getTransform()->setSize(timelineWidth() + 8.0f, std::max(4.0f, 32.0f + static_cast<float>(workingClip.tracks.size()) * 24.0f));
    panel->addChild(trackBackground);

    playhead = MRColor::create();
    playhead->setColor(Vector4(0.95f, 0.45f, 0.20f, 1.0f));
    playhead->getTransform()->setSize(2.0f, trackBackground->getTransform()->getSize().y);
    panel->addChild(playhead);

    for (size_t trackIndex = 0; trackIndex < workingClip.tracks.size(); ++trackIndex) {
        const auto& track = workingClip.tracks[trackIndex];
        const float rowY = trackStartY() + static_cast<float>(trackIndex) * 24.0f;
        const auto* targetNode = m_shell.m_session->document().findNode(track.targetId);
        auto label = std::make_shared<MRLabel>();
        label->setText(wide((targetNode ? targetNode->name : track.targetId) + "." + track.property), "default");
        label->setFontSize(12.0f);
        label->setFontColor(0.78f, 0.80f, 0.86f, 1.0f);
        label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        label->getTransform()->setPosition(8.0f, rowY, 0.0f);
        label->getTransform()->setSize(158.0f, 20.0f);
        panel->addChild(label);
        trackLabels.push_back(label);

        std::vector<std::shared_ptr<MRColor>> diamonds;
        for (size_t keyIndex = 0; keyIndex < track.keys.size(); ++keyIndex) {
            auto diamond = MRColor::create();
            const bool isSelected = static_cast<int>(trackIndex) == selectedTrack && static_cast<int>(keyIndex) == selectedKey;
            diamond->setColor(isSelected ? Vector4(1.0f, 0.55f, 0.15f, 1.0f) : Vector4(0.25f, 0.75f, 0.95f, 1.0f));
            diamond->setRounding(2.0f);
            diamond->getTransform()->setPosition(timeToX(track.keys[keyIndex].first) - 5.0f, rowY + 2.0f, 0.0f);
            diamond->getTransform()->setSize(10.0f, 14.0f);
            panel->addChild(diamond);
            diamonds.push_back(std::move(diamond));
        }
        keyDiamonds.push_back(std::move(diamonds));
    }
}

void AnimationPanel::updatePlayheadVisual() {
    if (!playhead || !workingValid)
        return;
    playhead->getTransform()->setPosition(timeToX(playheadTime), 36.0f, 0.0f);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3fs", playheadTime);
    timeLabel->setText(wide(buffer), "default");
}

void AnimationPanel::onTick() {
    if (!workingValid)
        return;
    const auto* state = m_shell.m_animationService.find(activePlayerId);
    if (state && state->playing) {
        playheadTime = state->time;
        updatePlayheadVisual();
    }
}

void AnimationPanel::keySelected() {
    const auto& selection = m_shell.m_session->model().selection().nodeIds;
    const auto& document = m_shell.m_session->document();
    // 目标节点：选中的非播放器节点（单个）
    const SceneNodeRecord* target = nullptr;
    for (auto id = selection.rbegin(); id != selection.rend(); ++id) {
        const auto* node = document.findNode(*id);
        if (node && node->type != "MRAnimationPlayer") {
            target = node;
            break;
        }
    }
    if (!target) {
        m_shell.setStatus("Select a node to key");
        return;
    }
    // 无播放器：一键创建（场景根下 MRAnimationPlayer + 空 Animation sub_resource）
    if (activePlayerId.empty()) {
        const auto* descriptor = m_shell.m_nodeTypeCatalog.find("MRAnimationPlayer");
        if (!descriptor) {
            m_shell.setStatus("Animation Player type is unavailable");
            return;
        }
        std::string rootId;
        for (const auto& node : document.nodes()) {
            if (node.parentId.empty()) {
                rootId = node.id;
                break;
            }
        }
        SceneNodeRecord playerNode = m_shell.m_nodeTypeCatalog.createNode(*descriptor, rootId, document);
        std::string animationId = "anim_auto";
        for (int suffix = 2; document.findSubResource(animationId) != nullptr; ++suffix)
            animationId = "anim_auto" + std::to_string(suffix);
        playerNode.properties["animation_id"] = animationId;
        const std::string playerId = playerNode.id;
        std::string error;
        if (!m_shell.m_session->addNode(std::move(playerNode), error)) {
            m_shell.setStatus("Failed to create Animation Player: " + error);
            return;
        }
        AnimationClip fresh;
        fresh.duration = 1.0f;
        fresh.loop = true;
        m_shell.m_session->document().upsertSubResource(fresh.toSubResource(animationId));
        m_shell.m_session->markDirty();
        m_shell.m_animationService.reload(m_shell.m_session->document());
        activePlayerId = playerId;
        activeAnimationId = animationId;
        workingClip = fresh;
        workingValid = true;
        playheadTime = 0.0f;
        m_shell.m_viewport->rebuildRuntime();
        m_shell.m_sceneTree->refresh();
    }
    if (!workingValid) {
        resolveActivePlayer();
        if (!workingValid) {
            m_shell.setStatus("Animation player has no clip");
            return;
        }
    }
    // 打 position/rotation/scale 键（取文档当前值）
    int keyed = 0;
    for (const char* property : {"position", "rotation", "scale"}) {
        const auto entry = target->properties.find(property);
        if (entry == target->properties.end())
            continue;
        const float time = std::min(playheadTime, workingClip.duration);
        workingClip.trackFor(target->id, property).setKey(time, entry->second);
        ++keyed;
    }
    if (keyed == 0) {
        m_shell.setStatus("Selected node has no position/rotation/scale");
        return;
    }
    commitClip();
    m_shell.setStatus("Keyed " + std::to_string(keyed) + " tracks at " + std::to_string(playheadTime) + "s");
}

void AnimationPanel::deleteSelectedKey() {
    if (!workingValid || selectedTrack < 0 || selectedTrack >= static_cast<int>(workingClip.tracks.size()))
        return;
    auto& track = workingClip.tracks[static_cast<size_t>(selectedTrack)];
    if (selectedKey < 0 || selectedKey >= static_cast<int>(track.keys.size()))
        return;
    track.keys.erase(track.keys.begin() + selectedKey);
    // 空轨道移除
    if (track.keys.empty())
        workingClip.tracks.erase(workingClip.tracks.begin() + selectedTrack);
    selectedTrack = -1;
    selectedKey = -1;
    commitClip();
    m_shell.setStatus("Deleted key");
}

void AnimationPanel::handlePointer(const TouchEvent& event) {
    if (!panel || !workingValid)
        return;
    if (event.eventType != TOUCH_EVENT_TYPE_TOUCH && event.eventType != TOUCH_EVENT_TYPE_MOVE && event.eventType != TOUCH_EVENT_TYPE_RELEASE)
        return;
    const bool gesture = draggingPlayhead || draggingKey;
    if (!gesture) {
        const Math::Rect bounds = panel->getWorldSpaceAABB();
        if (!bounds.Contains(event.positionX, event.positionY))
            return;
    }
    // panel 局部系（左上原点 Y 向下）
    const Math::Rect bounds = panel->getWorldSpaceAABB();
    const float localX = event.positionX - bounds.Min.x;
    const float localY = bounds.Max.y - event.positionY;

    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        // 命中菱形？
        for (size_t trackIndex = 0; trackIndex < keyDiamonds.size(); ++trackIndex) {
            for (size_t keyIndex = 0; keyIndex < keyDiamonds[trackIndex].size(); ++keyIndex) {
                const auto& diamond = keyDiamonds[trackIndex][keyIndex];
                if (diamond->getWorldSpaceAABB().Contains(event.positionX, event.positionY)) {
                    selectedTrack = static_cast<int>(trackIndex);
                    selectedKey = static_cast<int>(keyIndex);
                    draggingKey = true;
                    rebuildTracks();
                    return;
                }
            }
        }
        // 播放头区域（时间轴带）
        if (localX >= timelineStartX() - 8.0f && std::abs(localX - timeToX(playheadTime)) <= 10.0f && localY >= 30.0f) {
            draggingPlayhead = true;
            return;
        }
        // 时间轴空白：点按定位播放头
        if (localY >= 36.0f && localY < 56.0f && localX >= timelineStartX() && localX <= timelineStartX() + timelineWidth()) {
            draggingPlayhead = true;
            playheadTime = xToTime(localX);
            m_shell.m_animationService.previewAt(
                activePlayerId, playheadTime, [this](const std::string& nodeId, const std::string& property, const std::string& value) {
                    m_shell.applyAnimationSample(nodeId, property, value);
                });
            updatePlayheadVisual();
        }
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE && draggingPlayhead) {
        playheadTime = xToTime(localX);
        m_shell.m_animationService.previewAt(
            activePlayerId, playheadTime, [this](const std::string& nodeId, const std::string& property, const std::string& value) {
                m_shell.applyAnimationSample(nodeId, property, value);
            });
        updatePlayheadVisual();
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE && draggingKey) {
        if (selectedTrack >= 0 && selectedTrack < static_cast<int>(workingClip.tracks.size())) {
            auto& track = workingClip.tracks[static_cast<size_t>(selectedTrack)];
            if (selectedKey >= 0 && selectedKey < static_cast<int>(track.keys.size())) {
                const float newTime = xToTime(localX);
                // 实时挪动菱形视觉；时间在释放时提交
                if (auto& diamond = keyDiamonds[static_cast<size_t>(selectedTrack)][static_cast<size_t>(selectedKey)])
                    diamond->getTransform()->setPosition(timeToX(newTime) - 5.0f, diamond->getTransform()->getPosition().y, 0.0f);
            }
        }
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        if (draggingKey && selectedTrack >= 0 && selectedTrack < static_cast<int>(workingClip.tracks.size())) {
            auto& track = workingClip.tracks[static_cast<size_t>(selectedTrack)];
            if (selectedKey >= 0 && selectedKey < static_cast<int>(track.keys.size())) {
                const float newTime = xToTime(localX);
                const std::string value = track.keys[static_cast<size_t>(selectedKey)].second;
                track.keys.erase(track.keys.begin() + selectedKey);
                track.setKey(newTime, value);
                selectedKey = track.keyIndexAt(newTime);
            }
            commitClip();
        }
        draggingPlayhead = false;
        draggingKey = false;
    }
}

}  // namespace morrow::editor
