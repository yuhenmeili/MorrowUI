//
// SafeDynamicVectorCanvasDemo.cpp — 安全动态矢量画布演示
//
// 演示内容（模拟 ADAS 智驾覆盖层）：
//   1. 动态障碍物红色检测框 — 模拟前方车辆/行人
//   2. 车道线（白色虚线）      — 行车引导线
//   3. 倒车轨迹曲线（绿色）    — 模拟倒车动态轨迹
//   4. 速度指示圆环            — 模拟 HUD 速度圈
//
// 编译运行：
//   cmake --build build --target SafeDynamicVectorCanvasDemo --parallel 8
//   ./build/SafeDynamicVectorCanvasDemo.exe
//

#include <cmath>
#include <cstdlib>
#include <ctime>

#include "Engine.h"
#include "base/Transform.h"
#include "hmi/SafeDynamicVectorCanvas.h"

using namespace morrow;
using namespace morrow::Math;

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // -----------------------------------------------------------------------
    // 创建引擎
    // -----------------------------------------------------------------------
    EngineOptions options;
    options.multithread = false;
    options.windowInfo.name = "SafeDynamicVectorCanvas Demo - ADAS Overlay";

    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.05f, 0.05f, 0.1f, 1.0f);  // 深色背景模拟夜间驾驶

    // -----------------------------------------------------------------------
    // 创建矢量画布（预分配 8192 顶点，覆盖全窗口）
    // -----------------------------------------------------------------------
    auto canvas = SafeDynamicVectorCanvas::create(8192);
    auto canvasTransform = canvas->getComponent<Transform>();
    float winW = static_cast<float>(options.windowInfo.width);
    float winH = static_cast<float>(options.windowInfo.height);
    canvasTransform->setPosition(0.0f, 0.0f, 0.0f);  // 画布中心对齐窗口中心
    canvasTransform->setSize(winW, winH);
    window->addChild(canvas);

    // -----------------------------------------------------------------------
    // 逐帧更新 — 模拟 ADAS 传感器数据
    // -----------------------------------------------------------------------
    int frameCount = 0;

    engine->preRender().add([&, winW, winH]() {
        frameCount++;
        canvas->clear();

        // 以画布半尺寸为参考，所有坐标按比例缩放
        const float sc = std::min(winW / 1280.0f, winH / 720.0f);  // 等比缩放

        // ===== 1. 车道线（白色虚线）=====
        const float laneLeftX = -200.0f * sc;
        const float laneRightX = 200.0f * sc;
        const float laneTop = 350.0f * sc;
        const float laneBottom = -350.0f * sc;
        const float dashLen = 60.0f * sc;
        const float gapLen = 30.0f * sc;
        const float dashOffset = std::fmod(frameCount * 4.0f, dashLen + gapLen);  // 滚动效果

        auto drawDashedLine = [&](float x) {
            for (float y = laneBottom + dashOffset; y < laneTop; y += dashLen + gapLen) {
                float y1 = y;
                float y2 = std::min(y + dashLen, laneTop);
                if (y2 > laneBottom) {
                    canvas->drawLine(x, y1, x, y2, 1.0f, 1.0f, 1.0f, 0.7f);
                }
            }
        };
        drawDashedLine(laneLeftX);
        drawDashedLine(laneRightX);

        // ===== 2. 道路边缘实线 =====
        canvas->drawLine(-350.0f * sc, laneBottom, -350.0f * sc, laneTop, 0.5f, 0.5f, 0.5f, 1.0f);
        canvas->drawLine(350.0f * sc, laneBottom, 350.0f * sc, laneTop, 0.5f, 0.5f, 0.5f, 1.0f);

        // ===== 3. 动态障碍物检测框（模拟前方车辆）=====
        float obstacleBaseY = (50.0f + std::sin(frameCount * 0.02f) * 20.0f) * sc;
        float obstacleX = (-30.0f + std::sin(frameCount * 0.03f) * 80.0f) * sc;

        // 主障碍物（红色，模拟前车）
        canvas->drawRect((obstacleX - 50.0f * sc), (obstacleBaseY - 40.0f * sc), 100.0f * sc, 80.0f * sc, 1.0f, 0.2f, 0.2f, 0.9f);

        // 侧方障碍物（橙色，模拟并排车辆）
        canvas->drawRect((obstacleX + 180.0f * sc), (obstacleBaseY + 20.0f * sc), 90.0f * sc, 70.0f * sc, 1.0f, 0.5f, 0.1f, 0.8f);

        // 远处小型障碍物（黄色，模拟行人/自行车）
        float farObstacleX = (-200.0f + std::cos(frameCount * 0.04f) * 150.0f) * sc;
        canvas->drawRect(farObstacleX - 15.0f * sc, 250.0f * sc, 30.0f * sc, 60.0f * sc, 1.0f, 0.9f, 0.1f, 0.85f);
        canvas->drawRect(farObstacleX + 120.0f * sc, 230.0f * sc, 25.0f * sc, 50.0f * sc, 1.0f, 0.9f, 0.1f, 0.85f);

        // 障碍物距离连线（雷达探测示意）
        float radarOriginX = 0.0f;
        float radarOriginY = -300.0f * sc;
        canvas->drawLine(radarOriginX, radarOriginY, obstacleX, obstacleBaseY, 0.2f, 1.0f, 0.2f, 0.3f);
        canvas->drawLine(radarOriginX, radarOriginY, obstacleX + 180.0f * sc, obstacleBaseY + 20.0f * sc, 0.2f, 1.0f, 0.2f, 0.3f);

        // ===== 4. 倒车轨迹曲线（绿色）=====
        const int trajSegments = 40;
        Vector2 trajPoints[trajSegments];
        float trajBaseAngle = std::sin(frameCount * 0.015f) * 0.4f;

        for (int i = 0; i < trajSegments; ++i) {
            float t = static_cast<float>(i) / (trajSegments - 1);
            float y = (-300.0f + t * 500.0f) * sc;
            float x = std::sin(t * 2.0f + trajBaseAngle) * 60.0f * sc * t;
            trajPoints[i] = Vector2(x, y);
        }
        canvas->drawPolyline(trajPoints, trajSegments, 0.2f, 1.0f, 0.2f, 0.8f);

        // ===== 5. 速度指示圆环（模拟 HUD）=====
        float speedAngle = std::fmod(frameCount * 0.5f, 360.0f);

        // 外圈背景
        canvas->drawCircle(0.0f, -320.0f * sc, 90.0f * sc, 48, 0.2f, 0.2f, 0.3f, 0.6f);
        // 速度弧线（动态）
        const int arcSegs = 32;
        float arcR = 80.0f * sc;
        float arcCY = -320.0f * sc;
        for (int i = 0; i < arcSegs; ++i) {
            float a1 = -1.57f + (3.14159265f * static_cast<float>(i) / arcSegs);
            float a2 = -1.57f + (3.14159265f * static_cast<float>(i + 1) / arcSegs);
            float alpha = (i < static_cast<int>(speedAngle / 360.0f * arcSegs)) ? 1.0f : 0.3f;
            canvas->drawLine(std::cos(a1) * arcR, arcCY + std::sin(a1) * arcR, std::cos(a2) * arcR, arcCY + std::sin(a2) * arcR, 0.3f, 1.0f, 0.8f, alpha);
        }

        // ===== 6. 十字准星（画面中心）=====
        canvas->drawLine(-15.0f * sc, 0.0f, -5.0f * sc, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f);
        canvas->drawLine(5.0f * sc, 0.0f, 15.0f * sc, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f);
        canvas->drawLine(0.0f, -15.0f * sc, 0.0f, -5.0f * sc, 0.0f, 1.0f, 0.0f, 0.5f);
        canvas->drawLine(0.0f, 5.0f * sc, 0.0f, 15.0f * sc, 0.0f, 1.0f, 0.0f, 0.5f);

        // ===== 7. 帧率文字区域边框（示意）=====
        canvas->drawRect(-630.0f * sc, -340.0f * sc, 100.0f * sc, 24.0f * sc, 0.4f, 0.4f, 0.6f, 0.5f);

        // ===== 提交 GPU =====
        canvas->commit();
    });

    // -----------------------------------------------------------------------
    // 启动渲染主循环
    // -----------------------------------------------------------------------
    LOG_I("SafeDynamicVectorCanvas Demo started — ADAS overlay simulation");
    engine->render();

    return 0;
}
