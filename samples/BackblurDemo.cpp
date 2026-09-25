//
// BackblurDemo — Kawase 共享背景模糊链演示
//（docs/road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md §6 S1 验收场景）。
//
//   1. 全屏卡片墙（displayLayer = -5，边界以下）：彩色圆角卡片 + 文字，
//      其中一行做 alpha 呼吸动画 → 模糊背景持续变化，验证链每帧更新；
//   2. 两块毛玻璃面板 + 一枚胶囊（displayLayer = 0）：BackdropBlur 组件，
//      不同 tint / 圆角，互相交叠验证共享链语义（模糊源只含边界以下内容）；
//   3. 前景清晰条（displayLayer = 3，边界以上）：压在玻璃上保持锐利；
//   4. 运行中开/关切换：按钮翻转 BackdropBlurManager::setEnabled，
//      关闭后毛玻璃降级为 tint 半透明面板（"关闭 ≠ 删组件"）。
//
// 验收断言（--report-json）：
//   开启：backdropQuads=3、backdropDraws=5（downsample + kawase×2 + 合成 + 面片合并绘制）
//   --no-blur：backdropQuads=0、backdropDraws=0（帧结构退化、模糊子系统零成本）
//
// F3 切换调试 overlay 可看到 "Blur:3q/5d" 统计。
//

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Engine.h"
#include "FontManager.h"
#include "FrameState.h"
#include "base/Transform.h"
#include "ui/effects/BackdropBlur.h"
#include "ui/effects/BackdropBlurManager.h"
#include "elements/MRButton.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "ui/helpers/Tween.h"

using namespace morrow;
using namespace morrow::Math;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float height, float fontSize = 20.0f,
                                     HorizontalAlignment horizontal = HorizontalAlignment::LEFT) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.13f, 0.17f, 0.23f, 1.0f);
    label->setAlign(horizontal, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

// 卡片墙底色：色相环绕的柔和色板
Vector4 cardColor(int index) {
    const float hues[6][3] = {
        {0.91f, 0.36f, 0.32f}, {0.95f, 0.60f, 0.28f}, {0.94f, 0.80f, 0.32f},
        {0.36f, 0.68f, 0.52f}, {0.32f, 0.52f, 0.82f}, {0.62f, 0.40f, 0.76f},
    };
    const auto& hue = hues[index % 6];
    return Vector4(hue[0], hue[1], hue[2], 1.0f);
}

// 毛玻璃面板：透明 MRColor 作属主（提供 MeshFilter/圆角供 tint 降级复用），
// BackdropBlur 组件提交共享链采样面片
std::shared_ptr<MRColor> createGlassPanel(float x, float y, float width, float height, float rounding, const Vector4& tint) {
    auto panel = MRColor::create();
    panel->setColor(0.0f, 0.0f, 0.0f, 0.0f); // 面板自身不绘制，模糊面片即背景
    panel->setRounding(rounding);
    panel->getComponent<Transform>()->setPosition(x, y, 0.0f);
    panel->getComponent<Transform>()->setSize(width, height);

    auto blur = panel->addComponent<BackdropBlur>();
    blur->setBlurRadius(24.0f);
    blur->setTintColor(tint);
    // 圆角跟随属主（MRColor::setRounding 已写入材质）
    return panel;
}

}  // namespace

int main(int argc, char** argv) {
    uint32_t maxFrames = 0;
    bool reportJson = false;
    bool startDisabled = false;
    bool showOverlay = false;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--report-json") == 0) {
            reportJson = true;
        } else if (std::strcmp(argv[index], "--no-blur") == 0) {
            startDisabled = true;
        } else if (std::strcmp(argv[index], "--overlay") == 0) {
            showOverlay = true;
        } else if (std::strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
            maxFrames = static_cast<uint32_t>(std::strtoul(argv[++index], nullptr, 10));
        } else if (std::strcmp(argv[index], "--help") == 0) {
            std::cout << "BackblurDemo [options]\n"
                         "  --report-json   print batch statistics JSON and assert S1 acceptance\n"
                         "  --no-blur       start with backdrop blur disabled (tint degraded panels)\n"
                         "  --overlay       start with the debug overlay visible (F3 toggles)\n"
                         "  --frames N      stop after N frames\n";
            return 0;
        }
    }
    if (reportJson && maxFrames == 0) {
        maxFrames = 5;
    }

    EngineOptions engineOptions;
    engineOptions.multithread = false;
    engineOptions.maxFrames = maxFrames;
    engineOptions.debugOverlayVisible = showOverlay;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(0.90f, 0.93f, 0.96f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    // ---------------------------------------------------------------------
    // 1. 背景卡片墙（displayLayer = -5 → 分段边界以下，进入模糊源）
    // ---------------------------------------------------------------------
    constexpr int kColumns = 6;
    constexpr int kRows = 3;
    constexpr float cardW = 280.0f;
    constexpr float cardH = 200.0f;
    constexpr float gap = 18.0f;
    constexpr float wallLeft = 42.0f;
    constexpr float wallTop = 130.0f;
    std::vector<std::shared_ptr<MRColor>> wallCards;
    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            const int index = row * kColumns + column;
            const float x = wallLeft + column * (cardW + gap);
            const float y = wallTop + row * (cardH + gap);
            auto card = MRColor::create();
            card->setColor(cardColor(index));
            card->setRounding(16.0f);
            card->setDisplayLayer(-5);
            card->getComponent<Transform>()->setPosition(x, y, 0.0f);
            card->getComponent<Transform>()->setSize(cardW, cardH);
            window->addChild(card);
            wallCards.push_back(card);

            auto label = createLabel(L"卡片 " + std::to_wstring(index + 1), x + 20.0f, y + 18.0f, cardW - 40.0f, 34.0f, 22.0f);
            label->setFontColor(1.0f, 1.0f, 1.0f, 0.95f);
            label->setDisplayLayer(-5);
            window->addChild(label);
        }
    }
    // 中间一行做 alpha 呼吸：模糊背景持续变化，验证链跟随更新
    //（呼吸只改每实例 alpha，不破坏卡片合批）
    {
        auto wallTween = Tween::create(0.0f, 1.0f, 2.4f);
        wallTween->setLoop(-1);
        wallTween->setEase(EaseType::Linear).onUpdate([wallCards](float value) {
            const float alpha = 0.55f + 0.45f * std::sin(value * 3.14159265f * 2.0f);
            for (int column = 0; column < kColumns; ++column) {
                const auto& card = wallCards[kColumns + column];
                card->getComponent<MeshRenderer>()->getMaterial()->setFloat("alpha", alpha);
            }
        });
        wallTween->play();
        TweenManager::getInstance().addTween(wallTween);
    }

    window->addChild(createLabel(L"BackblurDemo — Kawase 共享背景模糊（S1）", 42.0f, 24.0f, 900.0f, 48.0f, 30.0f));
    window->addChild(createLabel(L"卡片墙在边界以下（会被模糊）；前景条在边界以上（保持清晰）；按钮切换全局开关", 42.0f, 76.0f, 1400.0f, 32.0f, 17.0f));

    // ---------------------------------------------------------------------
    // 2. 毛玻璃面板（displayLayer = 0，默认层即边界以上）
    // ---------------------------------------------------------------------
    auto panelA = createGlassPanel(260.0f, 320.0f, 560.0f, 420.0f, 28.0f, Vector4(0.97f, 0.98f, 1.0f, 0.45f));
    window->addChild(panelA);
    {
        auto title = createLabel(L"毛玻璃面板 A", 32.0f, 26.0f, 480.0f, 42.0f, 26.0f);
        panelA->addChild(title);
        auto desc = createLabel(L"白色 tint · 圆角 28 · 半径 24（S1 固定档）\n背后卡片与呼吸动画被实时模糊", 32.0f, 84.0f, 480.0f, 84.0f, 18.0f);
        panelA->addChild(desc);
    }

    auto panelB = createGlassPanel(760.0f, 200.0f, 480.0f, 340.0f, 44.0f, Vector4(0.30f, 0.46f, 0.85f, 0.50f));
    window->addChild(panelB);
    {
        auto title = createLabel(L"面板 B（与 A 交叠）", 32.0f, 26.0f, 400.0f, 42.0f, 24.0f);
        title->setFontColor(1.0f, 1.0f, 1.0f, 0.98f);
        panelB->addChild(title);
        auto desc = createLabel(L"共享模糊链语义：模糊源只含\n边界以下内容，面板之间\n不互相出现在对方背景里", 32.0f, 84.0f, 400.0f, 120.0f, 18.0f);
        desc->setFontColor(1.0f, 1.0f, 1.0f, 0.95f);
        panelB->addChild(desc);
    }

    auto pill = createGlassPanel(1330.0f, 620.0f, 500.0f, 120.0f, 60.0f, Vector4(0.16f, 0.18f, 0.24f, 0.42f));
    window->addChild(pill);
    {
        auto text = createLabel(L"胶囊形态 · 圆角 60", 36.0f, 38.0f, 420.0f, 44.0f, 22.0f);
        text->setFontColor(1.0f, 1.0f, 1.0f, 0.98f);
        pill->addChild(text);
    }

    // ---------------------------------------------------------------------
    // 3. 前景清晰条（displayLayer = 3，边界以上）：压在面板 A 上沿保持锐利
    // ---------------------------------------------------------------------
    auto sharpBar = MRColor::create();
    sharpBar->setColor(0.10f, 0.12f, 0.16f, 0.88f);
    sharpBar->setRounding(12.0f);
    sharpBar->setDisplayLayer(3);
    sharpBar->getComponent<Transform>()->setPosition(180.0f, 250.0f, 0.0f);
    sharpBar->getComponent<Transform>()->setSize(760.0f, 60.0f);
    window->addChild(sharpBar);
    {
        auto text = createLabel(L"边界以上的普通 UI：清晰压在玻璃之上（displayLayer = 3）", 24.0f, 14.0f, 700.0f, 32.0f, 18.0f);
        text->setFontColor(1.0f, 1.0f, 1.0f, 1.0f);
        sharpBar->addChild(text);
    }

    // ---------------------------------------------------------------------
    // 4. 运行中开/关切换：翻转全局开关，毛玻璃降级为 tint 面板
    // ---------------------------------------------------------------------
    auto statusLabel = std::make_shared<MRLabel>();
    statusLabel->setText(L"[模糊：开启]", "default");
    statusLabel->setFontSize(22.0f);
    statusLabel->setFontColor(0.10f, 0.40f, 0.16f, 1.0f);
    statusLabel->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    statusLabel->getComponent<Transform>()->setPosition(0.0f, 30.0f, 0.0f);
    statusLabel->getComponent<Transform>()->setSize(240.0f, 40.0f);
    statusLabel->setDisplayLayer(3);

    auto toggleButton = MRButton::create();
    toggleButton->setText(L"切换背景模糊", "default");
    toggleButton->setTextFontSize(22.0f);
    toggleButton->setTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    toggleButton->setBackgroundColor(Vector4(0.16f, 0.48f, 0.32f, 1.0f));
    toggleButton->setCornerRadius(14.0f);
    toggleButton->setDisplayLayer(3);
    toggleButton->getComponent<Transform>()->setPosition(40.0f, 60.0f, 0.0f);
    toggleButton->getComponent<Transform>()->setSize(240.0f, 90.0f);
    toggleButton->addChild(statusLabel);
    window->addChild(toggleButton);

    BackdropBlurManager::getInstance().setEnabled(!startDisabled);
    if (startDisabled) {
        statusLabel->setText(L"[模糊：关闭（tint 降级）]", "default");
    }

    auto toggleConnection = toggleButton->events().onClicked.connect([&statusLabel](BaseButton&) {
        auto& manager = BackdropBlurManager::getInstance();
        const bool enabled = !manager.isEnabled();
        manager.setEnabled(enabled);
        statusLabel->setText(enabled ? L"[模糊：开启]" : L"[模糊：关闭（tint 降级）]", "default");
    });
    (void)toggleConnection;

    LOG_I("BackblurDemo started — press F3 to toggle the debug overlay (Blur:3q/5d)");
    engine->render();

    if (reportJson) {
        const auto frameState = engine->getFrameState();
        const auto& stats = frameState->batchStatistics;
        std::cout << "{"
                  << "\"renderItems\":" << stats.renderItemCount << ","
                  << "\"batches\":" << stats.batchCount << ","
                  << "\"ssboBatches\":" << stats.ssboBatchCount << ","
                  << "\"standardBatches\":" << stats.standardBatchCount << ","
                  << "\"batchDrawCalls\":" << stats.batchDrawCallCount << ","
                  << "\"drawCalls\":" << frameState->drawCallCount << ","
                  << "\"backdropQuads\":" << stats.backdropQuadCount << ","
                  << "\"backdropDrawCalls\":" << stats.backdropDrawCallCount << "}" << std::endl;

        if (startDisabled) {
            // 降级路径：模糊子系统零成本，全部 draw 均来自普通批次
            if (stats.backdropQuadCount != 0 || stats.backdropDrawCallCount != 0 ||
                frameState->drawCallCount != stats.batchDrawCallCount) {
                std::cerr << "BackblurDemo --no-blur acceptance failed" << std::endl;
                return 2;
            }
        } else {
            // 3 个模糊面片合并为 1 个 draw；链 = downsample + kawase×2、合成 1、面片 1
            if (stats.backdropQuadCount != 3 || stats.backdropDrawCallCount != 5) {
                std::cerr << "BackblurDemo S1 acceptance failed" << std::endl;
                return 2;
            }
        }
    }
    return 0;
}
