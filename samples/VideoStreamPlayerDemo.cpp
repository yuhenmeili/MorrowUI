#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRVideoStreamPlayer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace morrow;

namespace {

constexpr int kVideoWidth = 640;
constexpr int kVideoHeight = 360;
constexpr float kFramesPerSecond = 30.0f;
constexpr uint64_t kTotalFrames = 300;

std::shared_ptr<MRLabel> createLabel(
    const std::wstring& text, float x, float y, float width, float height, float fontSize) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.9f, 0.94f, 1.0f, 1.0f);
    label->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

std::shared_ptr<MRButton> createButton(const std::wstring& text, float x, float y, float width = 210.0f) {
    auto button = MRButton::create();
    button->setText(text, "default");
    button->setTextFontSize(20.0f);
    button->setTextColor(0.08f, 0.12f, 0.18f, 1.0f);
    button->setBackgroundColor(0.82f, 0.89f, 0.96f, 1.0f);
    button->setHoverColor(Vector4(0.7f, 0.82f, 0.93f, 1.0f));
    button->setPressedColor(Vector4(0.58f, 0.72f, 0.87f, 1.0f));
    button->setCornerRadius(7.0f);
    button->getComponent<Transform>()->setPosition(x, y, 0.0f);
    button->getComponent<Transform>()->setSize(width, 56.0f);
    return button;
}

bool generateFrame(uint64_t frameIndex, std::vector<unsigned char>& pixels) {
    if (pixels.size() < static_cast<size_t>(kVideoWidth * kVideoHeight * 4))
        return false;

    const float time = static_cast<float>(frameIndex) / kFramesPerSecond;
    const int circleX = static_cast<int>((std::sin(time * 1.7f) * 0.42f + 0.5f) * kVideoWidth);
    const int circleY = static_cast<int>((std::cos(time * 1.2f) * 0.32f + 0.5f) * kVideoHeight);

    for (int y = 0; y < kVideoHeight; ++y) {
        for (int x = 0; x < kVideoWidth; ++x) {
            const size_t index = static_cast<size_t>((y * kVideoWidth + x) * 4);
            const float nx = static_cast<float>(x) / kVideoWidth;
            const float ny = static_cast<float>(y) / kVideoHeight;
            unsigned char r = static_cast<unsigned char>(25.0f + 80.0f * nx);
            unsigned char g = static_cast<unsigned char>(45.0f + 100.0f * ny);
            unsigned char b = static_cast<unsigned char>(100.0f + 80.0f * (1.0f - nx));

            const int dx = x - circleX;
            const int dy = y - circleY;
            if (dx * dx + dy * dy < 46 * 46) {
                r = 250;
                g = 184;
                b = 52;
            }

            const int scanY = static_cast<int>(frameIndex * 3 % kVideoHeight);
            if (std::abs(y - scanY) <= 2) {
                r = 70;
                g = 240;
                b = 190;
            }

            pixels[index + 0] = r;
            pixels[index + 1] = g;
            pixels[index + 2] = b;
            pixels[index + 3] = 255;
        }
    }
    return true;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.04f, 0.07f, 0.12f, 1.0f);
    engine->addFonts({FontInfo{.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"}});

    window->addChild(createLabel(L"MRVideoStreamPlayer 视频播放", 100.0f, 42.0f, 1000.0f, 54.0f, 34.0f));
    window->addChild(createLabel(
        L"演示使用内存 RGBA 帧源；实际项目可接入软件解码器或平台硬解码输出。",
        100.0f, 96.0f, 1400.0f, 40.0f, 20.0f));

    auto videoBackground = MRColor::create();
    videoBackground->setColor(0.01f, 0.02f, 0.04f, 1.0f);
    videoBackground->setRounding(12.0f);
    videoBackground->getComponent<Transform>()->setPosition(100.0f, 170.0f, -0.1f);
    videoBackground->getComponent<Transform>()->setSize(1280.0f, 720.0f);
    window->addChild(videoBackground);

    auto player = MRVideoStreamPlayer::create(
        kVideoWidth, kVideoHeight, kFramesPerSecond, kTotalFrames);
    player->getComponent<Transform>()->setPosition(100.0f, 170.0f, 0.0f);
    player->getComponent<Transform>()->setSize(1280.0f, 720.0f);
    player->setFrameProvider(generateFrame);
    player->setLoop(true);
    window->addChild(player);

    auto status = createLabel(L"状态：已停止", 1430.0f, 178.0f, 420.0f, 46.0f, 22.0f);
    auto timeLabel = createLabel(L"时间：0.00 / 10.00 秒", 1430.0f, 226.0f, 420.0f, 46.0f, 22.0f);
    window->addChild(status);
    window->addChild(timeLabel);

    player->setOnStateChangedCallback([status](MRVideoStreamPlayer::PlaybackState state) {
        switch (state) {
            case MRVideoStreamPlayer::PlaybackState::PLAYING:
                status->setText(L"状态：播放中", "default");
                break;
            case MRVideoStreamPlayer::PlaybackState::PAUSED:
                status->setText(L"状态：已暂停", "default");
                break;
            case MRVideoStreamPlayer::PlaybackState::STOPPED:
                status->setText(L"状态：已停止", "default");
                break;
        }
    });
    player->setOnFrameChangedCallback([player, timeLabel](uint64_t frame) {
        wchar_t text[96];
        std::swprintf(
            text, 96, L"时间：%.2f / %.2f 秒（帧 %llu）",
            player->getCurrentTime(), player->getDuration(),
            static_cast<unsigned long long>(frame));
        timeLabel->setText(text, "default");
    });

    auto play = createButton(L"播放", 1430.0f, 310.0f);
    play->setOnClickCallback([player]() { player->play(); });
    window->addChild(play);

    auto pause = createButton(L"暂停", 1430.0f, 382.0f);
    pause->setOnClickCallback([player]() { player->pause(); });
    window->addChild(pause);

    auto stop = createButton(L"停止", 1430.0f, 454.0f);
    stop->setOnClickCallback([player]() { player->stop(); });
    window->addChild(stop);

    auto backward = createButton(L"后退 2 秒", 1430.0f, 550.0f);
    backward->setOnClickCallback([player]() {
        player->seekSeconds(std::max(0.0, player->getCurrentTime() - 2.0));
    });
    window->addChild(backward);

    auto forward = createButton(L"前进 2 秒", 1430.0f, 622.0f);
    forward->setOnClickCallback([player]() {
        player->seekSeconds(std::min(player->getDuration(), player->getCurrentTime() + 2.0));
    });
    window->addChild(forward);

    auto speed = createButton(L"切换 1x / 2x", 1430.0f, 694.0f);
    speed->setOnClickCallback([player]() {
        player->setPlaybackSpeed(player->getPlaybackSpeed() < 1.5f ? 2.0f : 1.0f);
    });
    window->addChild(speed);

    player->play();
    LOG_I("VideoStreamPlayerDemo started");
    engine->render();
    return 0;
}
