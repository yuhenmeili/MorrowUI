#ifndef MORROW_TWEEN_H
#define MORROW_TWEEN_H

#include <functional>
#include <memory>
#include <vector>
#include <chrono>
#include "FrameState.h"

namespace morrow {
// 缓动函数类型
enum class EaseType {
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic
};

// Tween状态
enum class TweenState {
    Playing,
    Paused,
    Stopped
};

class Tween {
public:
    using TweenCallback = std::function<void(float)>;
    using TweenCompleteCallback = std::function<void()>;

    static std::shared_ptr<Tween> create(float from, float to, float duration);

    // 设置缓动类型
    Tween& setEase(EaseType easeType);

    // 循环次数：<0 无限循环；1（默认）播放一次；N>1 共播放 N 轮。
    // 循环 tween 的 onComplete 只在全部轮次结束后触发一次；
    // 每轮边界不产生回调，相位由超出部分结转保持。
    Tween& setLoop(int loopCount);

    // 设置更新回调
    Tween& onUpdate(TweenCallback callback);

    // 设置完成回调
    Tween& onComplete(TweenCompleteCallback callback);

    // 控制方法
    void play();

    void pause();

    void stop();

    void restart();

    // 更新方法（每帧调用）
    void update(FrameStateSharedPtr frameState);

    // 获取当前值
    float getCurrentValue() const;

    float getTargetValue() const;

    bool isPlaying() const;

    /// 自然播放到时长终点后置位（play/restart 复位）。
    /// TweenManager 以此回收 tween——不能用 currentValue==target 判完成：
    /// 循环 tween 在每轮边界值等于 target 但必须继续播放（见 setLoop）。
    bool isFinished() const;

private:
    Tween(float from, float to, float duration);

    // 缓动函数
    float ease(float t);

    float m_from;
    float m_to;
    float m_duration;
    float m_currentTime;
    float m_currentValue;
    EaseType m_easeType;
    TweenState m_state;
    int m_loopCount = 1;
    int m_loopsDone = 0;
    bool m_finished = false;
    TweenCallback m_updateCallback;
    TweenCompleteCallback m_completeCallback;
};

using TweenSharedPtr = std::shared_ptr<Tween>;

class TweenManager {
public:
    static TweenManager& getInstance();

    void addTween(TweenSharedPtr tween);

    void update(FrameStateSharedPtr frameState);

    void removeAllTweens();

    void killTween(TweenSharedPtr tween);

private:
    TweenManager() = default;

    std::vector<TweenSharedPtr> m_tweens;
};
} // namespace morrow

#endif // MORROW_TWEEN_H