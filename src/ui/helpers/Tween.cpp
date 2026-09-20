#include "Tween.h"
#include <algorithm>
#include <cmath>
#include <memory>

#include "GlobalObject.h"

namespace morrow {
std::shared_ptr<Tween> Tween::create(float from, float to, float duration) {
    return std::shared_ptr<Tween>(new Tween(from, to, duration));
}

Tween::Tween(float from, float to, float duration) :
    m_from(from), m_to(to), m_duration(duration), m_currentTime(0.0f), m_currentValue(from), m_easeType(EaseType::Linear), m_state(TweenState::Stopped) {
}

Tween& Tween::setEase(EaseType easeType) {
    m_easeType = easeType;
    return *this;
}

Tween& Tween::setLoop(int loopCount) {
    m_loopCount = loopCount;
    return *this;
}

Tween& Tween::onUpdate(TweenCallback callback) {
    m_updateCallback = callback;
    return *this;
}

Tween& Tween::onComplete(TweenCompleteCallback callback) {
    m_completeCallback = callback;
    return *this;
}

void Tween::play() {
    m_state = TweenState::Playing;
    m_finished = false;
    REQUESTRENDER;
}

void Tween::pause() {
    m_state = TweenState::Paused;
}

void Tween::stop() {
    m_state = TweenState::Stopped;
    m_currentTime = 0.0f;
    m_currentValue = m_from;
    m_loopsDone = 0;
}

void Tween::restart() {
    stop();
    play();
}

void Tween::update(FrameStateSharedPtr frameState) {
    if (m_state != TweenState::Playing) {
        return;
    }

    m_currentTime += frameState->deltaTime;

    // duration<=0 视为立即完成，避免除零与空转
    if (m_duration <= 0.0f) {
        m_currentTime = 0.0f;
        m_state = TweenState::Stopped;
        m_currentValue = m_to;
        m_finished = true;
        if (m_updateCallback) {
            m_updateCallback(m_currentValue);
        }
        if (m_completeCallback) {
            m_completeCallback();
        }
        return;
    }

    while (m_currentTime >= m_duration) {
        if (m_loopCount < 0 || m_loopsDone + 1 < m_loopCount) {
            // 还有下一轮：超出部分结转，保持相位与 Playing 状态；
            // 每轮边界不产生回调（onUpdate 由下方按新相位统一发出）
            ++m_loopsDone;
            m_currentTime -= m_duration;
        } else {
            // 最后一轮：终值、停止、标记完成
            m_currentTime = m_duration;
            m_state = TweenState::Stopped;
            m_currentValue = m_to;
            m_finished = true;

            if (m_updateCallback) {
                m_updateCallback(m_currentValue);
            }

            if (m_completeCallback) {
                m_completeCallback();
            }
            return;
        }
    }

    const float t = m_currentTime / m_duration;
    const float easedT = ease(t);
    m_currentValue = m_from + (m_to - m_from) * easedT;

    if (m_updateCallback) {
        m_updateCallback(m_currentValue);
    }
}

float Tween::getCurrentValue() const {
    return m_currentValue;
}

float Tween::getTargetValue() const {
    return m_to;
}

bool Tween::isPlaying() const {
    return m_state == TweenState::Playing;
}

bool Tween::isFinished() const {
    return m_finished;
}

float Tween::ease(float t) {
    switch (m_easeType) {
        case EaseType::Linear:
            return t;

        case EaseType::InQuad:
            return t * t;

        case EaseType::OutQuad:
            return t * (2 - t);

        case EaseType::InOutQuad:
            return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;

        case EaseType::InCubic:
            return t * t * t;

        case EaseType::OutCubic:
            t -= 1;
            return t * t * t + 1;

        case EaseType::InOutCubic:
            return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;

        default:
            return t;
    }
}

TweenManager& TweenManager::getInstance() {
    static TweenManager instance;
    return instance;
}

void TweenManager::addTween(TweenSharedPtr tween) {
    m_tweens.push_back(tween);
    REQUESTRENDER;
}

void TweenManager::update(FrameStateSharedPtr frameState) {
    bool hasPlayingTween = false;
    // 更新所有Tween
    for (auto it = m_tweens.begin(); it != m_tweens.end();) {
        (*it)->update(frameState);

        // 只按"自然完成"回收。不能用 currentValue==target 浮点相等判定：
        // 循环 tween 在每轮边界值等于 target 但必须继续播放，
        // 值相等会把循环动画回收掉。restart → play 会复位 m_finished，
        // 因此刚重启的 tween 也不会被误删。
        if ((*it)->isFinished()) {
            it = m_tweens.erase(it);
        } else {
            hasPlayingTween = hasPlayingTween || (*it)->isPlaying();
            ++it;
        }
    }
    if (hasPlayingTween) {
        REQUESTRENDER;
    }
}

void TweenManager::removeAllTweens() {
    m_tweens.clear();
}

void TweenManager::killTween(TweenSharedPtr tween) {
    auto it = std::find(m_tweens.begin(), m_tweens.end(), tween);
    if (it != m_tweens.end()) {
        m_tweens.erase(it);
    }
}
} // namespace morrow