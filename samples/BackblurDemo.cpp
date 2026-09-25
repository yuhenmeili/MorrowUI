//
// BackblurDemo — Kawase 共享背景模糊链演示
//（docs/road_map/KAWASE_BACKDROP_BLUR_PROPOSAL.md §6 S1/S2 验收场景）。
//
//   1. 全屏卡片墙（displayLayer = -5，边界以下）：彩色圆角卡片 + 文字，
//      其中一行做 alpha 呼吸动画 → 模糊背景持续变化，验证链每帧更新；
//   2. 三级半径毛玻璃面板（displayLayer = 0）：BackdropBlur 组件，
//      轻半径 8（L0'）/ 中半径 24（L1' ×2，验证层级内合批）/ 重半径 56（L2'），
//      互相交叠验证共享链语义（模糊源只含边界以下内容）；
//   3. 前景清晰条（displayLayer = 3，边界以上）：压在玻璃上保持锐利；
//   4. 运行中开/关切换与档位循环：按钮翻转 BackdropBlurManager::setEnabled /
//      setQuality（Off / Standard / LowCost），关闭后毛玻璃降级为 tint 半透明
//      面板（"关闭 ≠ 删组件"）。
//
// 验收断言（--report-json，统计取末帧）：
//   默认（呼吸动画 → 每帧脏）：backdropQuads=4、backdropDraws=10
//     （链 3 级 × 2 pass + 合成 1 + 面片 3 层级各 1 draw）
//   --static（背景静止 → 缓存命中）：backdropQuads=4、backdropDraws=4
//     （链与 backdrop 段整段跳过，仅合成 1 + 面片 3）
//   --quality off：backdropQuads=0、backdropDraws=0（退化路径，零成本）
//
// F3 切换调试 overlay 可看到 "Blur:4q/10d"（--static 下为 4q/4d）统计。
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
// BackdropBlur 组件按半径提交到对应采样层级
std::shared_ptr<MRColor> createGlassPanel(float x, float y, float width, float height, float rounding, const Vector4& tint, float blurRadius) {
    auto panel = MRColor::create();
    panel->setColor(0.0f, 0.0f, 0.0f, 0.0f); // 面板自身不绘制，模糊面片即背景
    panel->setRounding(rounding);
    panel->getComponent<Transform>()->setPosition(x, y, 0.0f);
    panel->getComponent<Transform>()->setSize(width, height);

    auto blur = panel->addComponent<BackdropBlur>();
    blur->setBlurRadius(blurRadius);
    blur->setTintColor(tint);
    // 圆角跟随属主（MRColor::setRounding 已写入材质）
    return panel;
}

}  // namespace

int main(int argc, char** argv) {
    uint32_t maxFrames = 0;
    bool reportJson = false;
    bool staticBackground = false;
    BackdropBlurQuality quality = BackdropBlurQuality::Standard;
    bool showOverlay = false;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--report-json") == 0) {
            reportJson = true;
        } else if (std::strcmp(argv[index], "--static") == 0) {
            staticBackground = true;
        } else if (std::strcmp(argv[index], "--no-blur") == 0) {
            quality = BackdropBlurQuality::Off; // 兼容别名：等价 --quality off
        } else if (std::strcmp(argv[index], "--quality") == 0 && index + 1 < argc) {
            const std::string value(argv[++index]);
            if (value == "off") {
                quality = BackdropBlurQuality::Off;
            } else if (value == "lowcost") {
                quality = BackdropBlurQuality::LowCost;
            } else {
                quality = BackdropBlurQuality::Standard;
            }
        } else if (std::strcmp(argv[index], "--overlay") == 0) {
            showOverlay = true;
        } else if (std::strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
            maxFrames = static_cast<uint32_t>(std::strtoul(argv[++index], nullptr, 10));
        } else if (std::strcmp(argv[index], "--help") == 0) {
            std::cout << "BackblurDemo [options]\n"
                         "  --report-json          print batch statistics JSON and assert S2 acceptance\n"
                         "  --static               no background animation (backdrop cache stays clean)\n"
                         "  --quality off|standard|lowcost   startup quality tier (default standard)\n"
                         "  --no-blur              alias of --quality off\n"
                         "  --overlay              start with the debug overlay visible (F3 toggles)\n"
                         "  --frames N             stop after N frames\n";
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
    engineOptions.backdropBlur = quality;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(0.90f, 0.93f, 0.96f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    // 帧时间统计（S2 性能验收观测）
    double totalFrameSeconds = 0.0;
    uint32_t timedFrameCount = 0;
    auto frameEndConnection = engine->events().onFrameEnd.connect([&totalFrameSeconds, &timedFrameCount, engine]() {
        totalFrameSeconds += engine->getFrameState()->deltaTime;
        ++timedFrameCount;
    });
    (void)frameEndConnection;

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
    if (!staticBackground) {
        // 中间一行做 alpha 呼吸：模糊背景持续变化，验证链跟随更新
        //（呼吸只改每实例 alpha，不破坏卡片合批）
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

    window->addChild(createLabel(L"BackblurDemo — Kawase 共享背景模糊（S1/S2）", 42.0f, 24.0f, 980.0f, 48.0f, 30.0f));
    window->addChild(createLabel(L"卡片墙在边界以下；玻璃分三级半径（8/24/56 → L0/L1/L2）；按钮切换开关与档位", 42.0f, 76.0f, 1500.0f, 32.0f, 17.0f));

    // ---------------------------------------------------------------------
    // 2. 三级半径毛玻璃面板（displayLayer = 0，默认层即边界以上）
    // ---------------------------------------------------------------------
    auto panelA = createGlassPanel(260.0f, 320.0f, 560.0f, 420.0f, 28.0f, Vector4(0.97f, 0.98f, 1.0f, 0.45f), 8.0f);
    window->addChild(panelA);
    {
        auto title = createLabel(L"轻模糊面板 A（半径 8 → L0'）", 32.0f, 26.0f, 500.0f, 42.0f, 24.0f);
        panelA->addChild(title);
        auto desc = createLabel(L"白色 tint · 圆角 28\n背后卡片与呼吸动画被实时模糊\n（与中/重面板对比模糊强度）", 32.0f, 84.0f, 480.0f, 120.0f, 18.0f);
        panelA->addChild(desc);
    }

    auto panelB = createGlassPanel(760.0f, 200.0f, 480.0f, 340.0f, 44.0f, Vector4(0.30f, 0.46f, 0.85f, 0.50f), 24.0f);
    window->addChild(panelB);
    {
        auto title = createLabel(L"面板 B（半径 24 → L1'）", 32.0f, 26.0f, 400.0f, 42.0f, 24.0f);
        title->setFontColor(1.0f, 1.0f, 1.0f, 0.98f);
        panelB->addChild(title);
        auto desc = createLabel(L"中模糊（面板常用档）\n与下方 B2 同层级 → 同层\n全部面片合并 1 个 draw", 32.0f, 84.0f, 400.0f, 120.0f, 18.0f);
        desc->setFontColor(1.0f, 1.0f, 1.0f, 0.95f);
        panelB->addChild(desc);
    }

    auto panelB2 = createGlassPanel(820.0f, 560.0f, 400.0f, 240.0f, 20.0f, Vector4(0.42f, 0.62f, 0.42f, 0.42f), 24.0f);
    window->addChild(panelB2);
    {
        auto desc = createLabel(L"B2（同 L1'）", 28.0f, 20.0f, 320.0f, 40.0f, 20.0f);
        panelB2->addChild(desc);
    }

    auto pill = createGlassPanel(1330.0f, 620.0f, 500.0f, 120.0f, 60.0f, Vector4(0.16f, 0.18f, 0.24f, 0.42f), 56.0f);
    window->addChild(pill);
    {
        auto text = createLabel(L"重模糊胶囊（半径 56 → L2'）", 36.0f, 38.0f, 420.0f, 44.0f, 22.0f);
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
    // 4. 运行中开/关切换与档位循环
    // ---------------------------------------------------------------------
    auto statusLabel = std::make_shared<MRLabel>();
    statusLabel->setText(L"[模糊：开启 · Standard]", "default");
    statusLabel->setFontSize(20.0f);
    statusLabel->setFontColor(0.10f, 0.40f, 0.16f, 1.0f);
    statusLabel->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    statusLabel->getComponent<Transform>()->setPosition(0.0f, 26.0f, 0.0f);
    statusLabel->getComponent<Transform>()->setSize(240.0f, 40.0f);
    statusLabel->setDisplayLayer(3);

    auto toggleButton = MRButton::create();
    toggleButton->setText(L"切换背景模糊", "default");
    toggleButton->setTextFontSize(20.0f);
    toggleButton->setTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    toggleButton->setBackgroundColor(Vector4(0.16f, 0.48f, 0.32f, 1.0f));
    toggleButton->setCornerRadius(14.0f);
    toggleButton->setDisplayLayer(3);
    toggleButton->getComponent<Transform>()->setPosition(40.0f, 880.0f, 0.0f);
    toggleButton->getComponent<Transform>()->setSize(240.0f, 90.0f);
    toggleButton->addChild(statusLabel);
    window->addChild(toggleButton);

    const auto refreshStatus = [&statusLabel]() {
        auto& manager = BackdropBlurManager::getInstance();
        std::wstring text = manager.isEnabled() ? L"[模糊：开启 · " : L"[模糊：关闭（tint 降级）· ";
        switch (manager.getQuality()) {
            case BackdropBlurQuality::Off:
                text += L"Off]";
                break;
            case BackdropBlurQuality::LowCost:
                text += L"LowCost]";
                break;
            default:
                text += L"Standard]";
                break;
        }
        statusLabel->setText(text, "default");
    };

    auto toggleConnection = toggleButton->events().onClicked.connect([&statusLabel](BaseButton&) {
        auto& manager = BackdropBlurManager::getInstance();
        manager.setEnabled(!manager.isEnabled());
        statusLabel->setText(manager.isEnabled() ? L"[模糊：开启 · 保留档位]" : L"[模糊：关闭（tint 降级）]", "default");
    });
    (void)toggleConnection;

    auto qualityLabel = std::make_shared<MRLabel>();
    qualityLabel->setText(L"档位：Standard", "default");
    qualityLabel->setFontSize(20.0f);
    qualityLabel->setFontColor(0.10f, 0.30f, 0.50f, 1.0f);
    qualityLabel->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    qualityLabel->getComponent<Transform>()->setPosition(0.0f, 26.0f, 0.0f);
    qualityLabel->getComponent<Transform>()->setSize(240.0f, 40.0f);
    qualityLabel->setDisplayLayer(3);

    auto qualityButton = MRButton::create();
    qualityButton->setText(L"循环切换档位", "default");
    qualityButton->setTextFontSize(20.0f);
    qualityButton->setTextColor(1.0f, 1.0f, 1.0f, 1.0f);
    qualityButton->setBackgroundColor(Vector4(0.20f, 0.36f, 0.62f, 1.0f));
    qualityButton->setCornerRadius(14.0f);
    qualityButton->setDisplayLayer(3);
    qualityButton->getComponent<Transform>()->setPosition(310.0f, 880.0f, 0.0f);
    qualityButton->getComponent<Transform>()->setSize(240.0f, 90.0f);
    qualityButton->addChild(qualityLabel);
    window->addChild(qualityButton);

    auto qualityConnection = qualityButton->events().onClicked.connect([&refreshStatus, &qualityLabel](BaseButton&) {
        auto& manager = BackdropBlurManager::getInstance();
        switch (manager.getQuality()) {
            case BackdropBlurQuality::Off:
                manager.setQuality(BackdropBlurQuality::Standard);
                break;
            case BackdropBlurQuality::Standard:
                manager.setQuality(BackdropBlurQuality::LowCost);
                break;
            case BackdropBlurQuality::LowCost:
                manager.setQuality(BackdropBlurQuality::Off);
                break;
        }
        refreshStatus();
        const std::wstring names[3] = {L"Off", L"Standard", L"LowCost"};
        qualityLabel->setText(L"档位：" + names[static_cast<int>(manager.getQuality())], "default");
    });
    (void)qualityConnection;

    refreshStatus();

    LOG_I("BackblurDemo started (quality={}, static={}) — press F3 to toggle the debug overlay",
          static_cast<int>(quality), staticBackground);
    engine->render();

    if (reportJson) {
        const auto frameState = engine->getFrameState();
        const auto& stats = frameState->batchStatistics;
        const double avgFrameMs = timedFrameCount > 0 ? (totalFrameSeconds / timedFrameCount * 1000.0) : 0.0;
        std::cout << "{"
                  << "\"renderItems\":" << stats.renderItemCount << ","
                  << "\"batches\":" << stats.batchCount << ","
                  << "\"ssboBatches\":" << stats.ssboBatchCount << ","
                  << "\"standardBatches\":" << stats.standardBatchCount << ","
                  << "\"batchDrawCalls\":" << stats.batchDrawCallCount << ","
                  << "\"drawCalls\":" << frameState->drawCallCount << ","
                  << "\"backdropQuads\":" << stats.backdropQuadCount << ","
                  << "\"backdropDrawCalls\":" << stats.backdropDrawCallCount << ","
                  << "\"avgFrameMs\":" << avgFrameMs << "}" << std::endl;

        if (quality == BackdropBlurQuality::Off) {
            // 降级路径：模糊子系统零成本，全部 draw 均来自普通批次
            if (stats.backdropQuadCount != 0 || stats.backdropDrawCallCount != 0 ||
                frameState->drawCallCount != stats.batchDrawCallCount) {
                std::cerr << "BackblurDemo --quality off acceptance failed" << std::endl;
                return 2;
            }
        } else if (staticBackground) {
            // 静止背景：backdrop 段与链整段跳过（缓存命中），仅剩合成 + 面片
            if (stats.backdropQuadCount != 4 || stats.backdropDrawCallCount != 4) {
                std::cerr << "BackblurDemo --static cache acceptance failed" << std::endl;
                return 2;
            }
        } else {
            // 呼吸动画：每帧脏。链跑满 3 级（存在 L2' 面片）= 6 pass，
            // 合成 1，面片 3 层级各 1 draw
            if (stats.backdropQuadCount != 4 || stats.backdropDrawCallCount != 10) {
                std::cerr << "BackblurDemo S2 acceptance failed" << std::endl;
                return 2;
            }
        }
    }
    return 0;
}
