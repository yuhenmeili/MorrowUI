#ifndef MORROW_GUI_MRVIDEOSTREAMPLAYER_H
#define MORROW_GUI_MRVIDEOSTREAMPLAYER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "Texture.h"
#include "base/UIWidget.h"
#include "MRImage.h"

namespace morrow {

/// RGBA 视频帧播放器：负责播放时钟和状态控制，帧解码由外部提供器完成。
class MRVideoStreamPlayer : public UIWidget {
public:
    /// 播放器状态。
    enum class PlaybackState {
        /// 已停止，并停留在第 0 帧。
        STOPPED,
        /// 正在播放。
        PLAYING,
        /// 已暂停，并保留当前帧。
        PAUSED,
    };

    /// 帧提供器。根据帧号写入 width × height × 4 字节 RGBA8 数据，成功返回 true。
    using FrameProvider = std::function<bool(uint64_t, std::vector<unsigned char>&)>;

    struct Events {
        Observable<MRVideoStreamPlayer&, PlaybackState> onStateChanged;
        Observable<MRVideoStreamPlayer&, uint64_t> onFrameChanged;
        Observable<MRVideoStreamPlayer&> onFinished;
    };

    /// 创建播放器。
    /// @param width 视频帧宽度
    /// @param height 视频帧高度
    /// @param framesPerSecond 视频帧率
    /// @param totalFrames 视频总帧数
    static std::shared_ptr<MRVideoStreamPlayer> create(
        int width, int height, float framesPerSecond, uint64_t totalFrames);

    /// 设置视频帧提供器。
    void setFrameProvider(FrameProvider provider);
    /// 开始或继续播放。
    void play();
    /// 暂停播放并保留当前帧。
    void pause();
    /// 停止播放并回到第 0 帧。
    void stop();
    /// 跳转到指定帧。
    void seekFrame(uint64_t frameIndex);
    /// 跳转到指定秒数。
    void seekSeconds(double seconds);
    /// 设置播放结束后是否循环。
    void setLoop(bool loop);
    /// 查询是否循环播放。
    bool isLoop() const;
    /// 设置播放速度；1.0 表示正常速度。
    void setPlaybackSpeed(float speed);
    /// 获取播放速度。
    float getPlaybackSpeed() const;
    /// 获取当前播放状态。
    PlaybackState getPlaybackState() const;
    /// 获取当前帧号。
    uint64_t getCurrentFrame() const;
    /// 获取视频总帧数。
    uint64_t getTotalFrames() const;
    /// 获取当前播放时间，单位为秒。
    double getCurrentTime() const;
    /// 获取视频总时长，单位为秒。
    double getDuration() const;

    Events& events();
    /// 每帧推进播放时钟并按需提交新的视频帧。
    void update(FrameStateSharedPtr frameState) override;

private:
    MRVideoStreamPlayer(int width, int height, float framesPerSecond, uint64_t totalFrames);

    bool presentFrame(uint64_t frameIndex);

    void setState(PlaybackState state);

    int m_width = 0;
    int m_height = 0;
    float m_framesPerSecond = 30.0f;
    uint64_t m_totalFrames = 0;
    uint64_t m_currentFrame = 0;
    double m_frameAccumulator = 0.0;
    float m_playbackSpeed = 1.0f;
    bool m_loop = false;
    PlaybackState m_state = PlaybackState::STOPPED;
    FrameProvider m_frameProvider;
    Events m_events;
    MRImageSharedPtr m_surface;
    TextureSharedPtr m_frameTexture;
};

using MRVideoStreamPlayerSharedPtr = std::shared_ptr<MRVideoStreamPlayer>;
using VideoStreamPlayer = MRVideoStreamPlayer;

}  // namespace morrow

#endif  // MORROW_GUI_MRVIDEOSTREAMPLAYER_H
