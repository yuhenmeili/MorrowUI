//
// MultiScreenDemo — 多屏渲染开关演示（QNX 双屏 / 两个 displayId）。
//
// Mirror 模式（默认）：主屏（display 3）UI 只渲染一次到离屏 RT，副屏
// （display 4，分辨率故意配置得不同以演示 letterbox）贴图上屏，内容一致。
// Independent 模式：把下方 screenMode 注释改为 Independent，两屏经
// getScene2D(index) 各自挂各自的 widget 子树、各自渲染。
//
// 仅 QNX/EGL 平台生效；其他平台 extraWindowInfos 被忽略（单窗口运行），
// 仍可用于开发主屏 UI 内容。
//

#include "morrow/Engine.h"

#include "morrow/base/Transform.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRLabel.h"
#include "morrow/utils/Log.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, float x, float y, float width, float height, float fontSize) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(fontSize);
    label->setFontColor(0.12f, 0.16f, 0.22f, 1.0f);
    label->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, height);
    return label;
}

} // namespace

int main() {
    EngineOptions engineOptions;

    // 主屏（窗口尺寸即镜像内容分辨率，建议取分辨率较大的一块屏）
    engineOptions.windowInfo.name = "PrimaryScreen";
    engineOptions.windowInfo.width = 1920;
    engineOptions.windowInfo.height = 1080;
    engineOptions.windowInfo.displayId = 3;

    // 副屏：分辨率与主屏不同，演示 Mirror 模式的等比缩放 + 黑边
    WindowInfo secondary = engineOptions.windowInfo;
    secondary.name = "SecondaryScreen";
    secondary.width = 1920;
    secondary.height = 720;
    secondary.displayId = 4;
    engineOptions.extraWindowInfos.push_back(secondary);

    // 开关：默认 Mirror；改 Independent 则两屏各自渲染各自的子树
    // engineOptions.screenMode = ScreenRenderMode::Independent;

    auto engine = std::make_shared<Engine>(engineOptions);
    engine->setClearColor(0.98f, 0.97f, 0.94f, 1.0f);

    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    // Mirror 模式下副屏只是主屏内容的缩放贴图，widget 仍全部挂在主窗口的
    // Scene2D；Independent 模式下副屏经 getScene2D(1) 挂自己的子树
    auto scene = engine->getScene2D();
    scene->addChild(createLabel(L"多屏镜像 Mirror：本窗口渲染一次，两块屏内容一致",
                                460.0f, 120.0f, 1000.0f, 60.0f, 34.0f));

    auto button = MRButton::create();
    button->setText(L"点我计数", "default");
    button->setTextFontSize(26.0f);
    button->getComponent<Transform>()->setPosition(760.0f, 260.0f, 0.0f);
    button->getComponent<Transform>()->setSize(200.0f, 90.0f);
    scene->addChild(button);

    auto counter = createLabel(L"点击次数：0", 660.0f, 400.0f, 600.0f, 60.0f, 28.0f);
    scene->addChild(counter);

    int clickCount = 0;
    auto clickConnection = button->events().onClicked.connect([&](BaseButton&) {
        ++clickCount;
        counter->setText(L"点击次数：" + std::to_wstring(clickCount), "default");
        LOG_I("Button clicked: {}", clickCount);
    });

    if (engineOptions.screenMode == ScreenRenderMode::Independent) {
        if (auto secondaryScene = engine->getScene2D(1)) {
            secondaryScene->addChild(createLabel(L"副屏独立内容 Independent",
                                                 460.0f, 240.0f, 1000.0f, 60.0f, 34.0f));
        }
    }

    LOG_I("MultiScreenDemo started: mode={}, windows={}",
          engineOptions.screenMode == ScreenRenderMode::Mirror ? "Mirror" : "Independent",
          engineOptions.extraWindowInfos.size() + 1);
    engine->render();
    return 0;
}
