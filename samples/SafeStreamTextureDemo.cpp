//
// SafeStreamTextureDemo.cpp — 安全流媒体纹理组件演示
//
// 演示内容（模拟 RVC 倒车影像画面）：
//   1. 动态彩条测试图 — 模拟摄像头原始画面
//   2. 移动引导线叠加   — 模拟倒车轨迹
//   3. 每帧更新纹理数据 — 模拟 30fps 视频流
//
// 编译运行：
//   cmake --build build --target SafeStreamTextureDemo --parallel 8
//   ./build/SafeStreamTextureDemo.exe
//

#include <array>
#include <atomic>
#include <cmath>
#include <cstring>

#include "Engine.h"
#include "base/Transform.h"
#include "hmi/SafeStreamTexture.h"

using namespace morrow;
using namespace morrow::Math;

// =========================================================================
// 模拟摄像头画面生成（仿 SMPTE 彩条 + 移动辅助线）
// =========================================================================

static constexpr int kStreamW = 640;
static constexpr int kStreamH = 480;
static unsigned char kFrameBuffer[kStreamW * kStreamH * 4];

static void generateTestPattern(int frameCount, unsigned char* frameBuffer = kFrameBuffer) {
    const int barW = kStreamW / 7;  // 7 色彩条

    for (int y = 0; y < kStreamH; ++y) {
        for (int x = 0; x < kStreamW; ++x) {
            int idx = (y * kStreamW + x) * 4;
            int bar = x / barW;

            // 7 色彩条：白、黄、青、绿、品、红、蓝
            switch (bar) {
                case 0:  // 白
                    frameBuffer[idx + 0] = 235;
                    frameBuffer[idx + 1] = 235;
                    frameBuffer[idx + 2] = 235;
                    break;
                case 1:  // 黄
                    frameBuffer[idx + 0] = 235;
                    frameBuffer[idx + 1] = 235;
                    frameBuffer[idx + 2] = 0;
                    break;
                case 2:  // 青
                    frameBuffer[idx + 0] = 0;
                    frameBuffer[idx + 1] = 235;
                    frameBuffer[idx + 2] = 235;
                    break;
                case 3:  // 绿
                    frameBuffer[idx + 0] = 0;
                    frameBuffer[idx + 1] = 235;
                    frameBuffer[idx + 2] = 0;
                    break;
                case 4:  // 品
                    frameBuffer[idx + 0] = 235;
                    frameBuffer[idx + 1] = 0;
                    frameBuffer[idx + 2] = 235;
                    break;
                case 5:  // 红
                    frameBuffer[idx + 0] = 235;
                    frameBuffer[idx + 1] = 0;
                    frameBuffer[idx + 2] = 0;
                    break;
                default:  // 蓝
                    frameBuffer[idx + 0] = 0;
                    frameBuffer[idx + 1] = 0;
                    frameBuffer[idx + 2] = 235;
                    break;
            }
            frameBuffer[idx + 3] = 255;  // alpha
        }
    }

    // 移动水平辅助线（绿色扫描线）
    int scanY = (frameCount * 3) % kStreamH;
    for (int x = 0; x < kStreamW; ++x) {
        int idx = (scanY * kStreamW + x) * 4;
        frameBuffer[idx + 0] = 0;
        frameBuffer[idx + 1] = 255;
        frameBuffer[idx + 2] = 0;
    }

    // 倒车轨迹线（黄色虚线，模拟方向盘转角）
    float angle = std::sin(frameCount * 0.03f) * 0.3f;
    int centerX = kStreamW / 2;
    int centerY = kStreamH - 50;

    for (int t = 0; t < 200; ++t) {
        int y = centerY - t;
        if (y < 0 || y >= kStreamH)
            continue;
        int x = centerX + static_cast<int>(std::sin(t * 0.05f + angle) * t * 0.3f);
        if (x < 0 || x >= kStreamW)
            continue;

        int idx = (y * kStreamW + x) * 4;
        frameBuffer[idx + 0] = 255;
        frameBuffer[idx + 1] = 255;
        frameBuffer[idx + 2] = 0;

        // 虚线效果
        if (t % 12 < 6) {
            // 加粗
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = x + dx;
                if (nx >= 0 && nx < kStreamW) {
                    int nidx = (y * kStreamW + nx) * 4;
                    frameBuffer[nidx + 0] = 255;
                    frameBuffer[nidx + 1] = 255;
                    frameBuffer[nidx + 2] = 0;
                }
            }
        }
    }

    // 帧号水印
    char fpsText[32];
    std::snprintf(fpsText, sizeof(fpsText), "FRAME:%04d", frameCount);
    // 简易白色像素文字在左上角（8px 高）
    for (int ci = 0; fpsText[ci] && ci < 20; ++ci) {
        int cx = 10 + ci * 10;
        for (int dy = 0; dy < 8; ++dy) {
            for (int dx = 0; dx < 8; ++dx) {
                int px = cx + dx;
                int py = 10 + dy;
                if (px < kStreamW && py < kStreamH) {
                    int idx = (py * kStreamW + px) * 4;
                    frameBuffer[idx + 0] = frameBuffer[idx + 1] = frameBuffer[idx + 2] = 255;
                }
            }
        }
    }
}

// =========================================================================
// main
// =========================================================================
int main() {
    // -----------------------------------------------------------------------
    // 创建引擎
    // -----------------------------------------------------------------------
    EngineOptions options;
    options.multithread = false;
    options.windowInfo.name = "SafeStreamTexture Demo - RVC Camera Sim";

    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    float winW = static_cast<float>(options.windowInfo.width);
    float winH = static_cast<float>(options.windowInfo.height);

    // -----------------------------------------------------------------------
    // 创建流媒体纹理（640×480 模拟摄像头分辨率）
    // -----------------------------------------------------------------------
    auto streamTex = SafeStreamTexture::create(kStreamW, kStreamH);
    auto tx = streamTex->getComponent<Transform>();

    // 居中显示，缩放到窗口 80% 大小（保持宽高比）
    float scale = std::min(winW / kStreamW, winH / kStreamH) * 0.8f;
    tx->setSize(kStreamW * scale, kStreamH * scale);
    tx->setPosition(100.0f, 100.0f, 0.0f);

    window->addChild(streamTex);

    // -----------------------------------------------------------------------
    // 逐帧更新 — 生成模拟视频帧
    // -----------------------------------------------------------------------
    int frameCount = 0;

    auto frameBeginConnection = engine->events().onFrameBegin.connect([&]() {
#ifdef OPENGL_EGL
        // OES/EGLImage directly references the submitted buffer. Keep three
        // buffers in rotation and only reuse one after the GPU fence callback.
        static std::array<std::array<unsigned char, kStreamW * kStreamH * 4>, 3> buffers;
        static std::array<std::atomic_bool, 3> available = {
            std::atomic_bool{true},
            std::atomic_bool{true},
            std::atomic_bool{true},
        };
        static size_t nextBuffer = 0;

        size_t bufferIndex = buffers.size();
        for (size_t i = 0; i < buffers.size(); ++i) {
            const size_t candidate = (nextBuffer + i) % buffers.size();
            bool expected = true;
            if (available[candidate].compare_exchange_strong(expected, false)) {
                bufferIndex = candidate;
                break;
            }
        }

        if (bufferIndex == buffers.size()) {
            LOG_W("SafeStreamTexture demo: all OES buffers are busy");
            return;
        }

        nextBuffer = (bufferIndex + 1) % buffers.size();
        auto* buffer = buffers[bufferIndex].data();
        generateTestPattern(frameCount, buffer);
        streamTex->updateFromEGLImage(buffer, [bufferIndex]() { available[bufferIndex].store(true); });
#else
        generateTestPattern(frameCount);
        streamTex->updateFrame(kFrameBuffer);
#endif
        frameCount++;
    });

    // -----------------------------------------------------------------------
    // 启动渲染主循环
    // -----------------------------------------------------------------------
    LOG_I("SafeStreamTexture Demo started — simulated RVC camera stream");
    engine->render();

    return 0;
}
