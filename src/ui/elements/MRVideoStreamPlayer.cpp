#include "MRVideoStreamPlayer.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRVideoStreamPlayer> MRVideoStreamPlayer::create(int width, int height, float framesPerSecond, uint64_t totalFrames) {
    auto player = std::shared_ptr<MRVideoStreamPlayer>(new MRVideoStreamPlayer(width, height, framesPerSecond, totalFrames));
    player->addChild(player->m_surface);
    return player;
}

MRVideoStreamPlayer::MRVideoStreamPlayer(int width, int height, float framesPerSecond, uint64_t totalFrames) :
    UIWidget(false), m_width(std::max(1, width)), m_height(std::max(1, height)), m_framesPerSecond(std::max(0.01f, framesPerSecond)), m_totalFrames(totalFrames),
    m_surface(MRImage::create()), m_frameTexture(Texture::create(ImageType::IMAGE)) {
    setWidgetType("MRVideoStreamPlayer");
    auto initialPixels = std::make_shared<std::vector<unsigned char>>(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4, 0);
    m_frameTexture->setTextureData(initialPixels, m_width, m_height, PixelDataFormat::RGBA, false);
    m_frameTexture->setMinFilterType(SamplerMinFilter::LINEAR);
    m_frameTexture->setMagFilterType(SamplerMagFilter::LINEAR);
    m_surface->setTexture(m_frameTexture);
    getComponent<Transform>()->setSize(static_cast<float>(m_width), static_cast<float>(m_height));
    getComponent<Transform>()->addSizeChangeListener([this]() {
        const Vector3 size = getComponent<Transform>()->getSize();
        m_surface->getComponent<Transform>()->setSize(size.x, size.y);
    });
    m_surface->getComponent<Transform>()->setPosition(0.0f, 0.0f, 0.0f);
    m_surface->getComponent<Transform>()->setSize(static_cast<float>(m_width), static_cast<float>(m_height));
}

void MRVideoStreamPlayer::setFrameProvider(FrameProvider provider) {
    m_frameProvider = std::move(provider);
    presentFrame(m_currentFrame);
}

void MRVideoStreamPlayer::play() {
    if (!m_frameProvider || m_totalFrames == 0)
        return;
    if (m_currentFrame >= m_totalFrames - 1 && !m_loop)
        seekFrame(0);
    setState(PlaybackState::PLAYING);
}

void MRVideoStreamPlayer::pause() {
    if (m_state == PlaybackState::PLAYING)
        setState(PlaybackState::PAUSED);
}

void MRVideoStreamPlayer::stop() {
    m_frameAccumulator = 0.0;
    setState(PlaybackState::STOPPED);
    seekFrame(0);
}

void MRVideoStreamPlayer::seekFrame(uint64_t frameIndex) {
    if (m_totalFrames == 0)
        return;
    m_frameAccumulator = 0.0;
    presentFrame(std::min(frameIndex, m_totalFrames - 1));
}

void MRVideoStreamPlayer::seekSeconds(double seconds) {
    const double clamped = std::clamp(seconds, 0.0, getDuration());
    seekFrame(static_cast<uint64_t>(std::floor(clamped * m_framesPerSecond)));
}

void MRVideoStreamPlayer::setLoop(bool loop) {
    m_loop = loop;
}

bool MRVideoStreamPlayer::isLoop() const {
    return m_loop;
}

void MRVideoStreamPlayer::setPlaybackSpeed(float speed) {
    m_playbackSpeed = std::max(0.01f, speed);
}

float MRVideoStreamPlayer::getPlaybackSpeed() const {
    return m_playbackSpeed;
}

MRVideoStreamPlayer::PlaybackState MRVideoStreamPlayer::getPlaybackState() const {
    return m_state;
}

uint64_t MRVideoStreamPlayer::getCurrentFrame() const {
    return m_currentFrame;
}

uint64_t MRVideoStreamPlayer::getTotalFrames() const {
    return m_totalFrames;
}

double MRVideoStreamPlayer::getCurrentTime() const {
    return static_cast<double>(m_currentFrame) / m_framesPerSecond;
}

double MRVideoStreamPlayer::getDuration() const {
    return static_cast<double>(m_totalFrames) / m_framesPerSecond;
}

MRVideoStreamPlayer::Events& MRVideoStreamPlayer::events() {
    return m_events;
}

void MRVideoStreamPlayer::update(FrameStateSharedPtr frameState) {
    if (m_state == PlaybackState::PLAYING && frameState && m_totalFrames > 0) {
        const double frameDuration = 1.0 / static_cast<double>(m_framesPerSecond);
        m_frameAccumulator += frameState->deltaTime * static_cast<double>(m_playbackSpeed);
        while (m_frameAccumulator >= frameDuration && m_state == PlaybackState::PLAYING) {
            m_frameAccumulator -= frameDuration;
            if (m_currentFrame + 1 < m_totalFrames) {
                presentFrame(m_currentFrame + 1);
            } else if (m_loop) {
                presentFrame(0);
            } else {
                setState(PlaybackState::STOPPED);
                m_events.onFinished.notify(*this);
            }
        }
    }
    UIWidget::update(frameState);
}

bool MRVideoStreamPlayer::presentFrame(uint64_t frameIndex) {
    if (!m_frameProvider || frameIndex >= m_totalFrames)
        return false;
    auto pixels = std::make_shared<std::vector<unsigned char>>(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4);
    if (!m_frameProvider(frameIndex, *pixels))
        return false;
    m_frameTexture->setTextureData(pixels, m_width, m_height, PixelDataFormat::RGBA, false);
    m_currentFrame = frameIndex;
    m_events.onFrameChanged.notify(*this, m_currentFrame);
    return true;
}

void MRVideoStreamPlayer::setState(PlaybackState state) {
    if (m_state == state)
        return;
    m_state = state;
    m_events.onStateChanged.notify(*this, m_state);
}

}  // namespace morrow
