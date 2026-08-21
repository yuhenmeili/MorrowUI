//
// SafeStaticTextLayoutDemo.cpp — 安全静态文本排版器演示
//
// 演示内容（模拟 HMI 仪表盘强文字告警）：
//   1. 固定告警词句轮播 — "CHECK ENGINE" / "BRAKE FAILURE" 等
//   2. 字符间距/颜色调整
//   3. 动态切换文本内容
//
// 编译运行：
//   cmake --build build --target SafeStaticTextLayoutDemo --parallel 8
//   ./build/SafeStaticTextLayoutDemo.exe
//

#include <cstring>
#include <vector>

#include "Engine.h"
#include "StaticAtlasManager.h"
#include "base/Transform.h"
#include "hmi/SafeStaticTextLayout.h"

using namespace morrow;
using namespace morrow::Math;

// =========================================================================
// 简易"字体"图集 — 每字符 8×12 像素 + 2px 间距，RGBA8
// Atlas: 512×16 像素 = 37 字符 × 10px pitch（8+2gap），单行
// 字符集: A-Z (0..25), 0-9 (26..35), SPACE (36)
// =========================================================================

static constexpr int kFontAtlasW = 512;
static constexpr int kFontAtlasH = 16;
static constexpr int kCharW = 8;
static constexpr int kCharH = 12;
static constexpr int kCharPitch = 10;  // 8px char + 2px gap

static unsigned char kFontAtlasData[kFontAtlasW * kFontAtlasH * 4] = {};

// 填充图集：每个 8×12 字符区域含白色边框 + 位置标记线，间距 2px
static void initFontAtlas() {
    std::memset(kFontAtlasData, 0, sizeof(kFontAtlasData));

    for (int col = 0; col < 37; ++col) {
        int ox = col * kCharPitch;  // 10px 步进（含 2px 间距）
        int oy = 0;

        for (int y = 0; y < kCharH; ++y) {
            for (int x = 0; x < kCharW; ++x) {
                int idx = ((oy + y) * kFontAtlasW + (ox + x)) * 4;
                // 白色边框
                bool border = (x == 0 || x == kCharW - 1 || y == 0 || y == kCharH - 1);
                // 位置标记线（区分不同字符）
                bool marker = (y == 3 + (col % 7) && x >= 1 && x <= kCharW - 2);

                if (border || marker) {
                    kFontAtlasData[idx + 0] = 255;  // R
                    kFontAtlasData[idx + 1] = 255;  // G
                    kFontAtlasData[idx + 2] = 255;  // B
                    kFontAtlasData[idx + 3] = 255;  // A
                }
            }
        }
    }
}

// =========================================================================
// 字符 Sprite 定义表（编译期常量，.rodata，永不失效）
// =========================================================================

// 37 个字符的定义：A-Z (0-25), 0-9 (26-35), SPACE (36)
static SafeSpriteDef kCharSpriteTable[37];

static void buildCharSpriteTable() {
    const float atlasW = static_cast<float>(kFontAtlasW);
    const float atlasH = static_cast<float>(kFontAtlasH);

    // 字符名 → 字符串字面量映射（.rodata 段，生命周期永久）
    static const char* kCharNames[37] = {"char_A", "char_B", "char_C", "char_D", "char_E", "char_F", "char_G", "char_H", "char_I", "char_J", "char_K",    "char_L", "char_M",
                                         "char_N", "char_O", "char_P", "char_Q", "char_R", "char_S", "char_T", "char_U", "char_V", "char_W", "char_X",    "char_Y", "char_Z",
                                         "char_0", "char_1", "char_2", "char_3", "char_4", "char_5", "char_6", "char_7", "char_8", "char_9", "char_SPACE"};

    for (int i = 0; i < 37; ++i) {
        int col = i;
        float ox = static_cast<float>(col * kCharPitch);
        kCharSpriteTable[i].name = kCharNames[i];  // 指向 .rodata
        kCharSpriteTable[i].u = ox / atlasW;
        kCharSpriteTable[i].v = 0.0f;
        kCharSpriteTable[i].u2 = (ox + kCharW) / atlasW;
        kCharSpriteTable[i].v2 = static_cast<float>(kCharH) / atlasH;
        kCharSpriteTable[i].offsetX = ox;
        kCharSpriteTable[i].offsetY = 0.0f;
        kCharSpriteTable[i].spriteWidth = static_cast<float>(kCharW);
        kCharSpriteTable[i].spriteHeight = static_cast<float>(kCharH);
    }
}

// =========================================================================
// 辅助：ASCII 字符 → sprite 名称（直接查表，无动态分配）
// =========================================================================
static const char* charToSpriteName(char c) {
    if (c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';
    if (c >= 'A' && c <= 'Z')
        return kCharSpriteTable[c - 'A'].name;
    if (c >= '0' && c <= '9')
        return kCharSpriteTable[26 + (c - '0')].name;
    if (c == ' ')
        return kCharSpriteTable[36].name;
    return kCharSpriteTable[36].name;  // fallback to space
}

static std::vector<const char*> textToSprites(const char* text) {
    std::vector<const char*> result;
    for (const char* p = text; *p; ++p) {
        result.push_back(charToSpriteName(*p));
    }
    return result;
}

// =========================================================================
// main
// =========================================================================
int main() {
    // 初始化图集数据
    initFontAtlas();
    buildCharSpriteTable();

    // -----------------------------------------------------------------------
    // 创建引擎
    // -----------------------------------------------------------------------
    EngineOptions options;
    options.multithread = false;
    options.windowInfo.name = "SafeStaticTextLayout Demo - HMI Warnings";

    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    float winW = static_cast<float>(options.windowInfo.width);
    float winH = static_cast<float>(options.windowInfo.height);

    // -----------------------------------------------------------------------
    // 注册字体图集到 StaticAtlasManager
    // -----------------------------------------------------------------------
    StaticAtlasManager::getInstance().registerAtlas("font_hmi", kFontAtlasData, kFontAtlasW, kFontAtlasH, kCharSpriteTable, 37);

    // -----------------------------------------------------------------------
    // 创建 3 行文本布局
    // -----------------------------------------------------------------------
    auto line1 = SafeStaticTextLayout::create("font_hmi");
    auto line2 = SafeStaticTextLayout::create("font_hmi");
    auto line3 = SafeStaticTextLayout::create("font_hmi");

    auto t1 = line1->getComponent<Transform>();
    auto t2 = line2->getComponent<Transform>();
    auto t3 = line3->getComponent<Transform>();

    // 先添加到窗口树，再设置内容和外观（避免 Transform 初始化时序问题）
    window->addChild(line1);
    window->addChild(line2);
    window->addChild(line3);

    // 位置：左上角绝对像素坐标（Transform position = 控件左上角）
    t1->setPosition(60.0f, 100.0f, 0.0f);
    t2->setPosition(60.0f, 160.0f, 0.0f);
    t3->setPosition(60.0f, 220.0f, 0.0f);

    // 初始文本（先设颜色再设文本，确保 buildTextMesh 前颜色已生效）
    line1->setColor(1.0f, 0.2f, 0.2f, 0.95f);  // 红色
    line2->setColor(1.0f, 0.5f, 0.1f, 0.95f);  // 橙色
    line3->setColor(1.0f, 0.9f, 0.1f, 0.95f);  // 黄色

    auto sp1 = textToSprites("CHECK ENGINE");
    auto sp2 = textToSprites("BRAKE FAILURE");
    auto sp3 = textToSprites("SEATBELT OFF");

    line1->setText(sp1.data(), static_cast<int>(sp1.size()));
    line2->setText(sp2.data(), static_cast<int>(sp2.size()));
    line3->setText(sp3.data(), static_cast<int>(sp3.size()));

    // -----------------------------------------------------------------------
    // 逐帧更新 — 轮播告警词句
    // -----------------------------------------------------------------------
    const char* warningMessages[] = {
        "CHECK ENGINE", "BRAKE FAILURE", "SEATBELT OFF", "OIL PRESSURE", "BATTERY LOW", "TIRE WARNING", "ABS FAULT", "AIRBAG ERROR", "ENGINE HOT", "FUEL LOW",
    };
    constexpr int kMsgCount = sizeof(warningMessages) / sizeof(warningMessages[0]);
    int frameCount = 0;

    auto frameBeginConnection = engine->events().onFrameBegin.connect([&]() {
        frameCount++;
        // 每 2 秒切换一次
        int msgIdx = (frameCount / 120) % kMsgCount;

        auto sp = textToSprites(warningMessages[msgIdx]);
        line3->setText(sp.data(), static_cast<int>(sp.size()));
        line3->setCharSpacing(2.0f);

        // line1/line2 保持不变
    });

    // -----------------------------------------------------------------------
    // 启动渲染主循环
    // -----------------------------------------------------------------------
    LOG_I("SafeStaticTextLayout Demo started — HMI warning text display");
    engine->render();

    return 0;
}
