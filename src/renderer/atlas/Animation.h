//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_ANIMATION_H_
#define MORROW_RENDERER_ANIMATION_H_

#include <vector>
#include <memory>
#include "MathUtils.h"
#include "Log.h"

namespace morrow
{
enum PlayMode
{
    NORMAL,
    REVERSED,
    LOOP,
    LOOP_REVERSED,
    LOOP_PINGPONG,
    LOOP_RANDOM
};

template<typename T>
class Animation
{
public:
    Animation(float frameDuration, const std::vector<T>& keyFrames)
    {
        this->m_playMode = PlayMode::NORMAL;
        this->m_frameDuration = frameDuration;
        this->setKeyFrames(keyFrames);
        LOG_I("keyFramesSize {}", keyFrames.size());
    }

    Animation(float frameDuration, const std::vector<T>& keyFrames, PlayMode playMode)
    {
        this->setPlayMode(playMode);
        this->m_frameDuration = frameDuration;
        this->setKeyFrames(keyFrames);
        LOG_I("keyFramesSize {}", keyFrames.size());
    }

    ~Animation()
    {
        this->m_keyFrames.clear();
    }

    T getKeyFrame(float stateTime, bool looping)
    {
        PlayMode oldPlayMode = this->m_playMode;
        if (looping && (this->m_playMode == PlayMode::NORMAL || this->m_playMode == PlayMode::REVERSED)) {
            if (this->m_playMode == PlayMode::NORMAL) {
                this->m_playMode = PlayMode::LOOP;
            } else {
                this->m_playMode = PlayMode::LOOP_REVERSED;
            }
        } else if (!looping && this->m_playMode != PlayMode::NORMAL && this->m_playMode != PlayMode::REVERSED) {
            if (this->m_playMode == PlayMode::LOOP_REVERSED) {
                this->m_playMode = PlayMode::REVERSED;
            } else {
                this->m_playMode = PlayMode::LOOP;
            }
        }

        T frame = this->getKeyFrame(stateTime);
        this->m_playMode = oldPlayMode;
        return frame;
    }

    T getKeyFrame(float stateTime)
    {
        int32_t frameNumber = this->getKeyFrameIndex(stateTime);
        return this->m_keyFrames[frameNumber];
    }

    int32_t getKeyFrameIndex(float stateTime)
    {
        int32_t keyFramesSize = (int32_t) this->m_keyFrames.size();
        if (keyFramesSize == 1) {
            return 0;
        } else {
            int32_t frameNumber = (int32_t) (stateTime / this->m_frameDuration);
            switch (this->m_playMode) {
                case NORMAL: {
                    frameNumber = std::min(keyFramesSize - 1, frameNumber);
                    break;
                }
                case LOOP: {
                    frameNumber %= keyFramesSize;
                    break;
                }
                case LOOP_PINGPONG: {
                    frameNumber %= keyFramesSize * 2 - 2;
                    if (frameNumber >= keyFramesSize) {
                        frameNumber = keyFramesSize - 2 - (frameNumber - keyFramesSize);
                    }
                    break;
                }
                case LOOP_RANDOM: {
                    int32_t lastFrameNumber = (int32_t) (this->m_lastStateTime / this->m_frameDuration);
                    if (lastFrameNumber != frameNumber) {
                        frameNumber = Math::random_int(0, keyFramesSize - 1);
                    } else {
                        frameNumber = this->m_lastFrameNumber;
                    }
                    break;
                }
                case REVERSED: {
                    frameNumber = std::max(keyFramesSize - frameNumber - 1, 0);
                    break;
                }
                case LOOP_REVERSED: {
                    frameNumber %= keyFramesSize;
                    frameNumber = keyFramesSize - frameNumber - 1;
                }
            }

            this->m_lastFrameNumber = frameNumber;
            this->m_lastStateTime = stateTime;
            return frameNumber;
        }
    }

    std::vector<T> getKeyFrames()
    {
        return this->m_keyFrames;
    }

    void setKeyFrames (const std::vector<T>& keyFrames) {
        this->m_keyFrames = keyFrames;
        this->m_animationDuration = keyFrames.size() * m_frameDuration;
    }

    PlayMode getPlayMode()
    {
        return this->m_playMode;
    }

    void setPlayMode(PlayMode playMode)
    {
        this->m_playMode = playMode;
    }

    bool isAnimationFinished(float stateTime)
    {
        int32_t frameNumber = (int32_t) (stateTime / this->m_frameDuration);
        return this->m_keyFrames.size() - 1 < frameNumber;
    }

    void setFrameDuration(float frameDuration)
    {
        this->m_frameDuration = frameDuration;
        this->m_animationDuration = (float) this->m_keyFrames.size() * frameDuration;
    }

    float getFrameDuration()
    {
        return this->m_frameDuration;
    }

    float getAnimationDuration()
    {
        return this->m_animationDuration;
    }

private:
    std::vector<T> m_keyFrames;
    float m_frameDuration;
    float m_animationDuration;
    int32_t m_lastFrameNumber;
    float m_lastStateTime;
    PlayMode m_playMode;
};

template<typename T>
using AnimationSharedPtr = std::shared_ptr<Animation<T>>;

} // MORROWGUI

#endif //MORROW_RENDERER_ANIMATION_H_
