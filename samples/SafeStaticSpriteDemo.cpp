//
// SafeStaticSpriteDemo.cpp — SafeStaticSprite 安全图片组件演示
//
// 演示内容：
//   1. 数字时速表 — 自动循环切换 0-9
//   2. 档位显示   — P/R/N/D 依次切换
//   3. 报警灯     — 随机点亮/熄灭
//
// 编译运行：
//   cmake --build build --target SafeStaticSpriteDemo --parallel 8
//   ./build/SafeStaticSpriteDemo.exe
//

#include <cmath>
#include <cstdlib>
#include <ctime>

#include "Engine.h"
#include "StaticAtlasManager.h"
#include "base/Transform.h"
#include "hmi/SafeStaticSprite.h"
#include "hmi/SafeStaticSpriteAtlas.h"

using namespace morrow;
using namespace morrow::Math;

// =========================================================================
// 硬编码图集像素数据 — 由外部工具链从 PNG 图集自动生成
// （参见 tools/generate_atlas_c_array.py）
// 图集规格：64×32 像素，RGBA8，共 8 KB
// =========================================================================

static constexpr int kAtlasWidth = 64;
static constexpr int kAtlasHeight = 32;

// 编译期静态断言：确保图集数据大小正确
static_assert(sizeof(kAtlasPixelData) == kAtlasWidth * kAtlasHeight * 4, "Atlas pixel data size mismatch!");

// =========================================================================
// 硬编码精灵 UV 定义表
// UV 计算公式：u = offsetX / atlasWidth, v = offsetY / atlasHeight
// =========================================================================

static const SafeSpriteDef kSpriteTable[] = {
    // ---- 数字时速表（0-9），每数字 6×8 像素 ----
    {"speed_0", 0.00000f, 0.00000f, 0.09375f, 0.25000f, 0, 0, 6, 8},
    {"speed_1", 0.09375f, 0.00000f, 0.18750f, 0.25000f, 6, 0, 6, 8},
    {"speed_2", 0.18750f, 0.00000f, 0.28125f, 0.25000f, 12, 0, 6, 8},
    {"speed_3", 0.28125f, 0.00000f, 0.37500f, 0.25000f, 18, 0, 6, 8},
    {"speed_4", 0.37500f, 0.00000f, 0.46875f, 0.25000f, 24, 0, 6, 8},
    {"speed_5", 0.46875f, 0.00000f, 0.56250f, 0.25000f, 30, 0, 6, 8},
    {"speed_6", 0.56250f, 0.00000f, 0.65625f, 0.25000f, 36, 0, 6, 8},
    {"speed_7", 0.65625f, 0.00000f, 0.75000f, 0.25000f, 42, 0, 6, 8},
    {"speed_8", 0.75000f, 0.00000f, 0.84375f, 0.25000f, 48, 0, 6, 8},
    {"speed_9", 0.84375f, 0.00000f, 0.93750f, 0.25000f, 54, 0, 6, 8},

    // ---- 档位显示（P/R/N/D），每档位 16×8 像素 ----
    {"gear_P", 0.00000f, 0.25000f, 0.25000f, 0.50000f, 0, 8, 16, 8},
    {"gear_R", 0.25000f, 0.25000f, 0.50000f, 0.50000f, 16, 8, 16, 8},
    {"gear_N", 0.50000f, 0.25000f, 0.75000f, 0.50000f, 32, 8, 16, 8},
    {"gear_D", 0.75000f, 0.25000f, 1.00000f, 0.50000f, 48, 8, 16, 8},

    // ---- 报警灯图标，每灯 8×8 像素 ----
    {"warning_engine", 0.00000f, 0.50000f, 0.12500f, 0.75000f, 0, 16, 8, 8},
    {"warning_oil", 0.12500f, 0.50000f, 0.25000f, 0.75000f, 8, 16, 8, 8},
    {"warning_battery", 0.25000f, 0.50000f, 0.37500f, 0.75000f, 16, 16, 8, 8},
    {"warning_brake", 0.37500f, 0.50000f, 0.50000f, 0.75000f, 24, 16, 8, 8},
    {"warning_seatbelt", 0.50000f, 0.50000f, 0.62500f, 0.75000f, 32, 16, 8, 8},
    {"warning_abs", 0.62500f, 0.50000f, 0.75000f, 0.75000f, 40, 16, 8, 8},
    {"warning_airbag", 0.75000f, 0.50000f, 0.87500f, 0.75000f, 48, 16, 8, 8},
    {"warning_tire", 0.87500f, 0.50000f, 1.00000f, 0.75000f, 56, 16, 8, 8},
};

static constexpr int kSpriteTableSize = sizeof(kSpriteTable) / sizeof(kSpriteTable[0]);

int main() {
    // 初始化随机种子
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // -----------------------------------------------------------------------
    // 创建引擎（单线程模式，便于调试）
    // -----------------------------------------------------------------------
    EngineOptions options;
    options.multithread = false;  // 单线程渲染，调试更方便
    options.windowInfo.name = "SafeStaticSprite Demo - HMI Safety Component";

    auto engine = std::make_shared<Engine>(options);
    auto window = engine->getWindow();
    window->setClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // 深色仪表盘背景

    // -----------------------------------------------------------------------
    // 0. 向 StaticAtlasManager 注册图集数据（必须在创建任何 SafeStaticSprite 之前调用）
    // -----------------------------------------------------------------------
    StaticAtlasManager::getInstance().registerAtlas("hmi_main", kAtlasPixelData, kAtlasWidth, kAtlasHeight, kSpriteTable, kSpriteTableSize);

    // -----------------------------------------------------------------------
    // 1. 数字时速表 — 3 位数字（百位、十位、个位）
    // -----------------------------------------------------------------------
    SafeStaticSpriteSharedPtr speedDigits[3];

    for (int i = 0; i < 3; ++i) {
        speedDigits[i] = SafeStaticSprite::create("hmi_main");
        auto t = speedDigits[i]->getComponent<Transform>();
        t->setPosition(200.0f + i * 80.0f, 100.0f, 0.0f);
        t->setSize(60.0f, 80.0f);  // 原始 6×8 → 放大到 60×80
        speedDigits[i]->setSprite("speed_0");
        window->addChild(speedDigits[i]);
    }

    // -----------------------------------------------------------------------
    // 2. 档位显示 — 单个大字
    // -----------------------------------------------------------------------
    auto gearSprite = SafeStaticSprite::create("hmi_main");
    auto gearTransform = gearSprite->getComponent<Transform>();
    gearTransform->setPosition(200.0f, 220.0f, 0.0f);
    gearTransform->setSize(128.0f, 64.0f);  // 原始 16×8 → 放大到 128×64
    gearSprite->setSprite("gear_P");
    window->addChild(gearSprite);

    // -----------------------------------------------------------------------
    // 3. 报警灯 — 8 个图标并排
    // -----------------------------------------------------------------------
    const char* warningNames[] = {"warning_engine", "warning_oil", "warning_battery", "warning_brake", "warning_seatbelt", "warning_abs", "warning_airbag", "warning_tire"};
    constexpr int kWarningCount = sizeof(warningNames) / sizeof(warningNames[0]);
    SafeStaticSpriteSharedPtr warningSprites[kWarningCount];

    for (int i = 0; i < kWarningCount; ++i) {
        warningSprites[i] = SafeStaticSprite::create("hmi_main");
        auto t = warningSprites[i]->getComponent<Transform>();
        t->setPosition(100.0f + i * 90.0f, 350.0f, 0.0f);
        t->setSize(64.0f, 64.0f);  // 原始 8×8 → 放大到 64×64
        warningSprites[i]->setSprite(warningNames[i]);
        window->addChild(warningSprites[i]);
    }

    // -----------------------------------------------------------------------
    // 注册逐帧更新 — 模拟仪表盘数据变化
    // -----------------------------------------------------------------------
    int frameCount = 0;
    int lastHundreds = -1, lastTens = -1, lastOnes = -1, lastGear = -1;

    engine->preRender().add([&]() {
        frameCount++;

        // ---- 模拟时速变化（百位、十位、个位）----
        int speed = (frameCount / 10) % 1000;  // 0~999 循环
        int hundreds = speed / 100;
        int tens = (speed / 10) % 10;
        int ones = speed % 10;

        // 仅在数值变化时才更新精灵（setSprite 内部也有变更检测，此处双重保险）
        if (hundreds != lastHundreds) {
            lastHundreds = hundreds;
            if (speedDigits[0])
                speedDigits[0]->setSprite("speed_" + std::to_string(hundreds));
        }
        if (tens != lastTens) {
            lastTens = tens;
            if (speedDigits[1])
                speedDigits[1]->setSprite("speed_" + std::to_string(tens));
        }
        if (ones != lastOnes) {
            lastOnes = ones;
            if (speedDigits[2])
                speedDigits[2]->setSprite("speed_" + std::to_string(ones));
        }

        // ---- 模拟档位切换（P→R→N→D 循环）----
        const char* gears[] = {"gear_P", "gear_R", "gear_N", "gear_D"};
        int gearIndex = (frameCount / 30) % 4;
        if (gearIndex != lastGear) {
            lastGear = gearIndex;
            if (gearSprite)
                gearSprite->setSprite(gears[gearIndex]);
        }

        // ---- 模拟报警灯随机闪烁 ----
        if (frameCount % 15 == 0) {
            for (int i = 0; i < kWarningCount; ++i) {
                bool lit = (std::rand() % 100) < 60;  // 60% 概率点亮
                // setAlpha 内部有变更检测，重复设置相同值无开销
                warningSprites[i]->setAlpha(lit ? 1.0f : 0.15f);
            }
        }
    });

    // -----------------------------------------------------------------------
    // 启动渲染主循环
    // -----------------------------------------------------------------------
    LOG_I("SafeStaticSprite Demo started — displaying speed, gear, and warning lights");
    engine->render();

    return 0;
}
