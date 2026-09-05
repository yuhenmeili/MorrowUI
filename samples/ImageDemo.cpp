//
// ImageDemo.cpp — Image 组件能力展示
//
// 六个卡片集中展示 MRImage 的能力与渲染特性：
//   1. 圆角阶梯     — shader 圆角（setRounding），一个 uniform 切出任意圆角矩形
//   2. UV 裁剪      — setScissor 取源图子区域，实现"放大镜"效果
//   3. 图片墙合批   — 10 张图共享纹理做 alpha 呼吸动画，仍合并在 1 个 SSBO 批次
//   4. 阴影         — Shadow 组件为任意带 MeshRenderer 的组件加投影
//   5. 图集区域     — TextureAtlas (.basis 图集) 按名取子区域显示
//   6. 程序化纹理   — CPU 生成 RGBA 数据经 Texture::setTextureData 直接上屏
//
// 批渲染统计 / 对象快照 / 帧数限制等调试能力已拆分至 DebugDemo。
//

#include <cmath>

#include "Engine.h"
#include "FontManager.h"
#include "Texture.h"
#include "atlas/TextureAtlas.h"
#include "base/Shadow.h"
#include "base/Transform.h"
#include "elements/MRColor.h"
#include "elements/MRImage.h"
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

std::shared_ptr<MRColor> createCard(float x, float y, float width, float height) {
    auto card = MRColor::create();
    card->setColor(0.99f, 0.995f, 1.0f, 1.0f);
    card->setRounding(14.0f);
    card->getComponent<Transform>()->setPosition(x, y, -0.2f);
    card->getComponent<Transform>()->setSize(width, height);
    return card;
}

void addCardTitle(const WindowSharedPtr& window, const std::wstring& title, const std::wstring& description, float x, float y, float width) {
    window->addChild(createLabel(title, x + 24.0f, y + 14.0f, width - 48.0f, 36.0f, 23.0f));
    window->addChild(createLabel(description, x + 24.0f, y + 50.0f, width - 48.0f, 30.0f, 16.0f));
}

std::shared_ptr<MRImage> createImage(const TextureSharedPtr& texture, float x, float y, float width, float height) {
    auto image = MRImage::create();
    image->setTexture(texture);
    auto transform = image->getComponent<Transform>();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    return image;
}

// CPU 生成 256x256 程序化纹理：对角渐变 + 棋盘格 + 中心圆环
std::shared_ptr<Texture> createProceduralTexture() {
    constexpr int size = 256;
    std::vector<unsigned char> pixels(static_cast<size_t>(size * size * 4));
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const size_t index = (static_cast<size_t>(y) * size + x) * 4;
            const bool checker = ((x / 16) + (y / 16)) % 2 == 0;
            const float gradient = static_cast<float>(x + y) / (2 * size - 2);
            const int dx = x - size / 2;
            const int dy = y - size / 2;
            const float radius = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            const bool ring = radius > 78.0f && radius < 92.0f;

            unsigned char r = static_cast<unsigned char>(40.0f + 180.0f * gradient);
            unsigned char g = static_cast<unsigned char>(60.0f + 150.0f * (1.0f - gradient));
            unsigned char b = checker ? 220 : 90;
            if (ring) {
                r = 255;
                g = 140;
                b = 20;
            }
            pixels[index + 0] = r;
            pixels[index + 1] = g;
            pixels[index + 2] = b;
            pixels[index + 3] = 255;
        }
    }
    auto data = std::shared_ptr<std::vector<unsigned char>>(
        new std::vector<unsigned char>(std::move(pixels)));
    auto texture = Texture::create();
    texture->setTextureData(data, size, size);
    return texture;
}

}  // namespace

int main() {
    EngineOptions engineOptions;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(0.92f, 0.95f, 0.98f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    window->addChild(createLabel(L"Image Showcase", 60.0f, 26.0f, 1800.0f, 54.0f, 36.0f));
    window->addChild(createLabel(L"MRImage 能力展示：圆角 / 裁剪 / 合批动画 / 阴影 / 图集区域 / 程序化纹理", 60.0f, 80.0f, 1800.0f, 36.0f, 20.0f));

    constexpr float top = 140.0f;
    constexpr float bottom = 560.0f;
    constexpr float cardWidth = 560.0f;
    constexpr float cardHeight = 390.0f;
    constexpr float left = 60.0f;
    constexpr float middle = 660.0f;
    constexpr float right = 1260.0f;

    const auto brickTexture = Texture::create();
    brickTexture->setImageUrl("assets/textures/brickwall.jpg");
    const auto carTexture = Texture::create();
    carTexture->setImageUrl("assets/textures/car.png");
    auto atlas = std::make_shared<TextureAtlas>("assets/textures/frame_animation/atlas_speed.atlas", "assets/textures/frame_animation/", false);

    // ---------------------------------------------------------------------
    // 1. 圆角阶梯：同一张图，shader 圆角从 0 递增
    // ---------------------------------------------------------------------
    window->addChild(createCard(left, top, cardWidth, cardHeight));
    addCardTitle(window, L"圆角阶梯 setRounding", L"shader 圆角裁剪，无需预处理贴图", left, top, cardWidth);
    for (int index = 0; index < 5; ++index) {
        auto image = createImage(brickTexture, left + 42.0f + index * 100.0f, top + 130.0f, 84.0f, 84.0f);
        image->setRounding(static_cast<float>(index) * 14.0f);
        window->addChild(image);
    }
    window->addChild(createLabel(L"rounding = 0 / 14 / 28 / 42 / 56", left + 24.0f, top + 250.0f, cardWidth - 48.0f, 32.0f, 17.0f));

    // ---------------------------------------------------------------------
    // 2. UV 裁剪：setScissor 取源图子区域实现放大镜
    // ---------------------------------------------------------------------
    window->addChild(createCard(left, bottom, cardWidth, cardHeight));
    addCardTitle(window, L"UV 裁剪 setScissor", L"取源图子区域拉伸显示，实现放大镜效果", left, bottom, cardWidth);
    window->addChild(createImage(brickTexture, left + 40.0f, bottom + 120.0f, 220.0f, 150.0f));
    auto zoomImage = createImage(brickTexture, left + 300.0f, bottom + 100.0f, 220.0f, 220.0f);
    zoomImage->setRounding(16.0f);
    window->addChild(zoomImage);
    window->addChild(createLabel(L"原图", left + 40.0f, bottom + 280.0f, 220.0f, 32.0f, 17.0f, HorizontalAlignment::CENTER));
    window->addChild(createLabel(L"setScissor 4 倍放大", left + 300.0f, bottom + 330.0f, 220.0f, 32.0f, 17.0f, HorizontalAlignment::CENTER));
    {
        // setImageUrl 在首次渲染时才同步解码，加载完成前 getWidth()/getHeight() 为 0，
        // 直接计算 UV 会得到 NaN（采样变黑）。订阅 onLoaded，尺寸就绪后再应用裁剪。
        auto scissorConnection = brickTexture->onLoaded().connect([brickTexture, zoomImage]() {
            const float sourceWidth = static_cast<float>(brickTexture->getWidth());
            const float sourceHeight = static_cast<float>(brickTexture->getHeight());
            zoomImage->setScissor(sourceWidth * 0.30f, sourceHeight * 0.30f, sourceWidth * 0.25f, sourceHeight * 0.25f);
        });
        (void)scissorConnection;  // main 内存活到渲染结束，RAII 连接无需手动断开
    }

    // ---------------------------------------------------------------------
    // 3. 图片墙：10 张图共享纹理，alpha 呼吸动画仍合并为 1 个 SSBO 批次
    // ---------------------------------------------------------------------
    window->addChild(createCard(middle, top, cardWidth, cardHeight));
    addCardTitle(window, L"图片墙与合批", L"同纹理同材质 → 1 个 SSBO 批次 / 1 次 draw call", middle, top, cardWidth);
    std::vector<std::shared_ptr<MRImage>> wallImages;
    for (int index = 0; index < 10; ++index) {
        const int column = index % 5;
        const int row = index / 5;
        auto image = createImage(carTexture, middle + 42.0f + column * 100.0f, top + 130.0f + row * 110.0f, 90.0f, 90.0f);
        wallImages.push_back(image);
        window->addChild(image);
    }
    {
        // 呼吸动画：alpha 是 SSBO 每实例属性，动画不影响合批
        auto wallTween = Tween::create(0.0f, 1.0f, 2.0f);
        wallTween->setEase(EaseType::Linear).onUpdate([wallImages](float value) {
            for (size_t i = 0; i < wallImages.size(); ++i) {
                const float phase = value + static_cast<float>(i) * 0.1f;
                const float alpha = 0.55f + 0.45f * std::sin(phase * 3.14159265f * 2.0f);
                wallImages[i]->getComponent<MeshRenderer>()->getMaterial()->setFloat("alpha", alpha);
            }
        }).onComplete([wallTween]() { wallTween->restart(); });
        wallTween->play();
        TweenManager::getInstance().addTween(wallTween);
    }

    // ---------------------------------------------------------------------
    // 4. 阴影：Shadow 组件（underlay 通道）
    // ---------------------------------------------------------------------
    // 注意：阴影走 underlay 通道、先于一切普通内容绘制，因此不能垫在不透明
    // 卡片上（会被后画的面板盖住）。此区域直接放在窗口清屏背景上。
    addCardTitle(window, L"Shadow 投影", L"underlay 通道先绘制，勿垫在不透明面板上", middle, bottom, cardWidth);
    auto shadowColor = MRColor::create();
    shadowColor->setColor(Vector4(0.95f, 0.45f, 0.25f, 1.0f));
    shadowColor->setRounding(18.0f);
    shadowColor->getComponent<Transform>()->setPosition(middle + 60.0f, bottom + 150.0f, 0.0f);
    shadowColor->getComponent<Transform>()->setSize(160.0f, 160.0f);
    {
        auto shadow = shadowColor->addComponent<Shadow>();
        shadow->setShadowOffset(Vector2(28.0f, 28.0f));
        shadow->setShadowColor(Vector4(0.1f, 0.15f, 0.3f, 0.55f));
    }
    window->addChild(shadowColor);

    auto shadowImage = createImage(carTexture, middle + 300.0f, bottom + 150.0f, 170.0f, 170.0f);
    shadowImage->setRounding(20.0f);
    {
        auto shadow = shadowImage->addComponent<Shadow>();
        shadow->setShadowOffset(Vector2(40.0f, 40.0f));
        shadow->setShadowColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));
    }
    window->addChild(shadowImage);
    window->addChild(createLabel(L"offset 28 / 40，渐变宽度与偏移一致", middle + 24.0f, bottom + 350.0f, cardWidth - 48.0f, 32.0f, 17.0f));

    // ---------------------------------------------------------------------
    // 5. 图集区域：TextureAtlas 按名取子区域
    // ---------------------------------------------------------------------
    window->addChild(createCard(right, top, cardWidth, cardHeight));
    addCardTitle(window, L"图集区域 TextureAtlas", L".basis 图集按名取子区域，同图集天然合批", right, top, cardWidth);
    const char* regionNames[] = {"O_HundredDigit_1.png", "O_SingleDigit_3.png", "O_TensDigit_6.png"};
    for (int index = 0; index < 3; ++index) {
        auto image = MRImage::create();
        image->setTexture(atlas, regionNames[index]);
        auto transform = image->getComponent<Transform>();
        transform->setPosition(right + 42.0f + index * 170.0f, top + 130.0f, 0.0f);
        transform->setSize(120.0f, 190.0f);
        window->addChild(image);
    }
    window->addChild(createLabel(L"regions: O_*Digit_*.png，同图集天然合批", right + 24.0f, top + 340.0f, cardWidth - 48.0f, 32.0f, 17.0f));

    // ---------------------------------------------------------------------
    // 6. 程序化纹理：CPU 数据直接上屏
    // ---------------------------------------------------------------------
    window->addChild(createCard(right, bottom, cardWidth, cardHeight));
    addCardTitle(window, L"程序化纹理 setTextureData", L"CPU 生成的 RGBA 数据直接创建纹理", right, bottom, cardWidth);
    const auto proceduralTexture = createProceduralTexture();
    window->addChild(createImage(proceduralTexture, right + 60.0f, bottom + 120.0f, 220.0f, 220.0f));
    auto proceduralRounded = createImage(proceduralTexture, right + 320.0f, bottom + 120.0f, 200.0f, 200.0f);
    proceduralRounded->setRounding(100.0f);
    window->addChild(proceduralRounded);
    window->addChild(createLabel(L"渐变 + 棋盘 + 圆环，右侧切圆形", right + 24.0f, bottom + 350.0f, cardWidth - 48.0f, 32.0f, 17.0f));

    LOG_I("ImageDemo started");
    engine->render();
    return 0;
}
