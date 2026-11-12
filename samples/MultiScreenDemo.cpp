//
// MultiScreenDemo — 多屏渲染开关演示（QNX 双屏 / 两个 displayId）。
//
// Mirror 模式（默认）：主屏（display 3）UI 只渲染一次到离屏 RT，副屏
// （display 4，分辨率故意配置得不同以演示 letterbox）贴图上屏，内容一致。
// Independent 模式：把下方 screenMode 注释改为 Independent，两屏各自挂
// 各自的 widget 子树、各自渲染。
//
// 仅 QNX/EGL 平台生效；Windows 平台 extraWindowInfos 被忽略（单窗口运行），
// 仍可用于开发主屏 UI 内容。
//

#include "morrow/Engine.h"
#include "morrow/base/Transform.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRLabel.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createLabel(const std::wstring& text, const std::string& fontName, float x, float y, float width, float height, float fontSize) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, fontName);
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
    engineOptions.multithread = false;

    // 主屏（窗口尺寸即镜像内容分辨率，建议取分辨率较大的一块屏）
    engineOptions.windowInfo.name = "PrimaryScreen";
    engineOptions.windowInfo.width = 1920;
    engineOptions.windowInfo.height = 1080;
    engineOptions.windowInfo.displayId = 3;

    // 副屏：分辨率与主屏不同，演示 Mirror 模式的等比缩放 + 黑边
    // displayId 以 initDisplay 枚举到的实际 ID 为准（本板为 3/1/2/4）
    WindowInfo secondary = engineOptions.windowInfo;
    secondary.name = "SecondaryScreen";
    secondary.width = 1920;
    secondary.height = 720;
    secondary.displayId = 4;
    engineOptions.extraWindowInfos.push_back(secondary);

    // 开关：Mirror 两屏同内容；Independent 两屏各自渲染各自的子树
    engineOptions.screenMode = ScreenRenderMode::Independent;

    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    // 公共面：应用经 getWindow()（主窗口）/ getWindow(displayId)（按物理屏
    // WindowInfo.displayId 查找）持有各屏窗口门面，Scene2D 根与清屏色等
    // 窗口能力都在 Window 上。
    auto primaryWindow = engine->getWindow();
    auto primaryScene = primaryWindow->scene2D();
    // 透明背景（与 Cluster 的窗口透明模式一致：清屏 alpha=0，半透明内容
    // 由 Screen 合成器按 PRE_MULTIPLIED_ALPHA 与下层图层合成）
    primaryWindow->setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    FontInfo fontInfo = {"debug_morrow_20", "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});
    const std::string fontName = fontInfo.name;

    // Mirror 模式下副屏只是主屏内容的缩放贴图，widget 仍全部挂在主窗口
    const bool mirror = engineOptions.screenMode == ScreenRenderMode::Mirror;
    auto title = createLabel(mirror ? L"多屏镜像 Mirror：本窗口渲染一次，两块屏内容一致"
                                    : L"双屏独立 Independent：两块屏各自渲染各自的内容",
                             fontName, 460.0f, 120.0f, 1000.0f, 60.0f, 34.0f);
    primaryScene->addChild(title);

    auto button = MRButton::create();
    button->setText(L"隐藏副屏", fontName);
    button->setTextFontSize(26.0f);
    button->getComponent<Transform>()->setPosition(760.0f, 260.0f, 0.0f);
    button->getComponent<Transform>()->setSize(200.0f, 90.0f);
    primaryScene->addChild(button);

    auto statusLabel = createLabel(L"副屏状态：显示中", fontName, 660.0f, 400.0f, 600.0f, 60.0f, 28.0f);
    primaryScene->addChild(statusLabel);

    // Observable::connect 返回 RAII Connection，临时对象语句结束即析构断开：
    // 连接必须保活到 render() 退出，否则 onClicked 永远进不到应用回调
    // （表现为 MouseDown 有日志、点击无反应）
    std::vector<Observable<BaseButton&>::Connection> clickConnections;
    // 主屏按钮：切换副屏显隐（演示窗口门面 Window::setVisible，QNX 走 Screen
    // 合成器 SCREEN_PROPERTY_VISIBLE，隐藏期间副屏渲染与 present 一并跳过；
    // displayId 与副屏创建时的 WindowInfo.displayId 一致）
    bool secondaryVisible = true;
    clickConnections.push_back(button->events().onClicked.connect([&](BaseButton&) {
        secondaryVisible = !secondaryVisible;
        auto secondaryWin = engine->getWindow(secondary.displayId);
        const bool applied = secondaryWin != nullptr;
        if (secondaryWin) {
            secondaryWin->setVisible(secondaryVisible);
        }
        button->setText(secondaryVisible ? L"隐藏副屏" : L"显示副屏", fontName);
        statusLabel->setText(applied
                                 ? (secondaryVisible ? L"副屏状态：显示中" : L"副屏状态：已隐藏")
                                 : L"接口未生效：未找到副屏",
                             fontName);
        LOG_INFO("Toggle secondary visible={}, applied={}", secondaryVisible, applied);
    }));

    // Independent 模式：副屏挂自己的子树（widget 不能同时挂两个窗口，
    // addChild 会自动从旧父节点摘除）；副屏独立清屏色经窗口门面设置
    //（Windows 等忽略 extraWindowInfos 的平台按 displayId 查不到副屏，
    // 返回 nullptr，整块跳过）。
    auto secondaryWindow = engine->getWindow(secondary.displayId);
    if (engineOptions.screenMode == ScreenRenderMode::Independent && secondaryWindow) {
        secondaryWindow->setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        auto secondaryScene = secondaryWindow->scene2D();
        secondaryScene->addChild(createLabel(L"副屏独立内容 Independent", fontName,
                                             460.0f, 240.0f, 1000.0f, 60.0f, 34.0f));
        secondaryScene->addChild(createLabel(L"主屏的标题/按钮/计数不会出现在本屏", fontName,
                                             460.0f, 340.0f, 1000.0f, 50.0f, 28.0f));

        // 副屏自己的按钮 + 计数：验证 Independent 模式下副屏输入路由
        auto secondaryButton = MRButton::create();
        secondaryButton->setText(L"副屏点我", fontName);
        secondaryButton->setTextFontSize(26.0f);
        secondaryButton->getComponent<Transform>()->setPosition(760.0f, 420.0f, 0.0f);
        secondaryButton->getComponent<Transform>()->setSize(200.0f, 90.0f);
        secondaryScene->addChild(secondaryButton);

        auto secondaryCounter = createLabel(L"副屏点击次数：0", fontName,
                                            660.0f, 540.0f, 600.0f, 50.0f, 28.0f);
        secondaryScene->addChild(secondaryCounter);

        int secondaryClickCount = 0;
        clickConnections.push_back(secondaryButton->events().onClicked.connect([&](BaseButton&) {
            ++secondaryClickCount;
            secondaryCounter->setText(L"副屏点击次数：" + std::to_wstring(secondaryClickCount), fontName);
            LOG_INFO("Secondary button clicked: {}", secondaryClickCount);
        }));
    }

    LOG_INFO("MultiScreenDemo started: mode={}, windows={}",
             engineOptions.screenMode == ScreenRenderMode::Mirror ? "Mirror" : "Independent",
             1 + engineOptions.extraWindowInfos.size());
    engine->render();
    return 0;
}
