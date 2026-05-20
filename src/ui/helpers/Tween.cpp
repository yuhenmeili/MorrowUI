#include "Tween.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace morrow {

std::shared_ptr<Tween> Tween::create(float from, float to, float duration) {
    return std::shared_ptr<Tween>(new Tween(from, to, duration));
}

Tween::Tween(float from, float to, float duration)
    : m_from(from)
    , m_to(to)
    , m_duration(duration)
    , m_currentTime(0.0f)
    , m_currentValue(from)
    , m_easeType(EaseType::Linear)
    , m_state(TweenState::Stopped) {
}

Tween& Tween::setEase(EaseType easeType) {
    m_easeType = easeType;
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
}

void Tween::pause() {
    m_state = TweenState::Paused;
}

void Tween::stop() {
    m_state = TweenState::Stopped;
    m_currentTime = 0.0f;
    m_currentValue = m_from;
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

    if (m_currentTime >= m_duration) {
        m_currentTime = m_duration;
        m_state = TweenState::Stopped;
        m_currentValue = m_to;

        if (m_updateCallback) {
            m_updateCallback(m_currentValue);
        }

        if (m_completeCallback) {
            m_completeCallback();
        }
    } else {
        float t = m_currentTime / m_duration;
        float easedT = ease(t);
        m_currentValue = m_from + (m_to - m_from) * easedT;

        if (m_updateCallback) {
            m_updateCallback(m_currentValue);
        }
    }
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
}

void TweenManager::update(FrameStateSharedPtr frameState) {
    // 更新所有Tween
    for (auto it = m_tweens.begin(); it != m_tweens.end();) {
        (*it)->update(frameState);
        
        // 移除已完成的Tween
        if ((*it)->getCurrentValue() == (*it)->getTargetValue()) {
            it = m_tweens.erase(it);
        } else {
            ++it;
        }
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