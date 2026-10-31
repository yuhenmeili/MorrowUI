#ifndef MORROW_EDITOR_ANIMATION_SERVICE_H
#define MORROW_EDITOR_ANIMATION_SERVICE_H

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "scene/SceneDocument.h"

namespace morrow::editor {

/// 一条动画轨道：目标节点 + 属性 + 按 time 升序的关键帧（序列化值）。
/// interpolation：linear / ease_in / ease_out / ease_in_out（作用于相邻键间的插值比例）。
struct AnimationTrack {
    std::string targetId;
    std::string property;
    std::string interpolation = "linear";
    std::vector<std::pair<float, std::string>> keys;

    /// 找到 time 的关键帧索引（容差 1e-4），找不到返回 -1。
    int keyIndexAt(float time) const;

    /// upsert 关键帧并保持升序。
    void setKey(float time, const std::string& value);
};

/// 一个动画片段：时长/循环 + 轨道集合，序列化为 [sub_resource type="Animation"]。
struct AnimationClip {
    float duration = 1.0f;
    bool loop = true;
    std::vector<AnimationTrack> tracks;

    /// 从场景 sub_resource 解析（duration/loop/track_count/track_N_* 键）。
    static bool fromSubResource(const SceneSubResourceRecord& record, AnimationClip& clip, std::string& error);

    /// 写回场景 sub_resource（keys 用 "t=v;t=v" 打包）。
    SceneSubResourceRecord toSubResource(const std::string& id) const;

    /// 采样轨道在 t 时刻的值（数值/Vector/Color 线性插值，字符串阶梯保持）。
    bool sample(const AnimationTrack& track, float time, std::string& value) const;

    /// 找（或创建）目标轨道。
    AnimationTrack& trackFor(const std::string& targetId, const std::string& property);
};

/// 运行中的播放器状态。
struct AnimationPlayerState {
    std::string nodeId;  // MRAnimationPlayer 节点 id
    AnimationClip clip;
    float time = 0.0f;
    float speed = 1.0f;
    bool playing = false;
};

/// 属性应用回调：编辑器预览写运行时 Widget，SceneRuntime 写自己的实例表。
using AnimationApplyValue = std::function<void(const std::string& nodeId, const std::string& property, const std::string& value)>;

/// 动画播放服务：编辑器与 SceneRuntime 共用；tick 由引擎 onFrameBegin 驱动。
class AnimationService {
public:
    /// 从文档重建播放器集合（MRAnimationPlayer 节点 + animation_id 引用的 Animation sub_resource）。
    void reload(const SceneDocument& document);

    /// 播放/暂停指定播放器；stop 回零并应用第 0 帧恢复原值由调用方决定。
    void play(const std::string& nodeId);
    void pause(const std::string& nodeId);
    void stop(const std::string& nodeId);

    /// 预览：立即应用播放器在 t 时刻的采样（拖动播放头用，不改播放状态）。
    void previewAt(const std::string& nodeId, float time, const AnimationApplyValue& apply);

    /// 帧推进：内部计时 dt，推进所有播放中的播放器并经回调应用采样。
    void tick(const AnimationApplyValue& apply);

    const AnimationPlayerState* find(const std::string& nodeId) const;
    AnimationPlayerState* find(const std::string& nodeId);

    /// 用文档中的动画 sub_resource 替换播放器片段（编辑器写入后调用）。
    void updateClip(const std::string& nodeId, const AnimationClip& clip);

private:
    void applyPlayerAt(AnimationPlayerState& player, float time, const AnimationApplyValue& apply);

    std::map<std::string, AnimationPlayerState> m_players;
    std::chrono::steady_clock::time_point m_lastTick;
    bool m_lastTickValid = false;
};

}  // namespace morrow::editor

#endif  // MORROW_EDITOR_ANIMATION_SERVICE_H
