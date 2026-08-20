#include "Engine.h"
#include "FontManager.h"
#include "Texture.h"
#include "atlas/TextureAtlas.h"
#include "base/Transform.h"
#include "elements/MRAnchorPointScale.h"
#include "elements/MRBounce.h"
#include "elements/MRBrakePedal.h"
#include "elements/MRColor.h"
#include "elements/MRFlowingLight.h"
#include "elements/MRFrameAnimation.h"
#include "elements/MRLabel.h"
#include "ui/helpers/Tween.h"

using namespace morrow;

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

void addCard(const WindowSharedPtr& window, const std::wstring& title, const std::wstring& description, float x, float y, float width, float height) {
    auto card = MRColor::create();
    card->setColor(0.99f, 0.995f, 1.0f, 1.0f);
    card->setRounding(14.0f);
    card->getComponent<Transform>()->setPosition(x, y, -0.2f);
    card->getComponent<Transform>()->setSize(width, height);
    window->addChild(card);
    window->addChild(createLabel(title, x + 24.0f, y + 16.0f, width - 48.0f, 38.0f, 24.0f));
    window->addChild(createLabel(description, x + 24.0f, y + 54.0f, width - 48.0f, 34.0f, 17.0f));
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.92f, 0.95f, 0.98f, 1.0f);
    engine->addFonts({FontInfo{
        .name = "default",
        .path = "assets/fonts/MorrowSansCN1.1-Regular.otf",
    }});

    window->addChild(createLabel(L"Animation Effects Showcase", 60.0f, 26.0f, 1800.0f, 54.0f, 36.0f));
    window->addChild(createLabel(L"集中展示 Transform/Tween、Shader 动画、流光和图集帧动画。", 60.0f, 80.0f, 1800.0f, 36.0f, 20.0f));

    constexpr float top = 140.0f;
    constexpr float cardWidth = 560.0f;
    constexpr float cardHeight = 390.0f;
    constexpr float left = 60.0f;
    constexpr float middle = 680.0f;
    constexpr float right = 1300.0f;

    // MRAnchorPointScale
    addCard(window, L"MRAnchorPointScale", L"左下 Pivot 驱动 Transform 缩放", left, top, cardWidth, cardHeight);
    auto anchorTexture = Texture::create(ImageType::IMAGE);
    anchorTexture->setImageUrl("assets/textures/img.jpg");
    auto anchorScale = MRAnchorPointScale::create();
    auto anchorTransform = anchorScale->getComponent<Transform>();
    anchorTransform->setPosition(left + 190.0f, top + 130.0f, 0.0f);
    anchorTransform->setSize(180.0f, 180.0f);
    anchorTransform->setPivot(0.0f, 1.0f, 0.0f);
    anchorScale->getComponent<MeshRenderer>()->getMaterial()->setTexture("texture", anchorTexture);
    window->addChild(anchorScale);

    auto scaleTween = Tween::create(0.15f, 1.0f, 1.5f);
    scaleTween->setEase(EaseType::Linear).onUpdate([anchorTransform](float value) {
        anchorTransform->setScale(value, value, 1.0f);
    }).onComplete([scaleTween]() {
        scaleTween->restart();
    });
    scaleTween->play();
    TweenManager::getInstance().addTween(scaleTween);

    // MRBounce
    addCard(window, L"MRBounce", L"Shader 参数与 Tween 联动的循环弹跳", middle, top, cardWidth, cardHeight);
    auto bounceTexture = Texture::create(ImageType::IMAGE);
    bounceTexture->setImageUrl("assets/textures/bounce/aeb_r.png");
    auto bounce = MRBounce::create();
    auto bounceTransform = bounce->getComponent<Transform>();
    bounceTransform->setPosition(middle + 190.0f, top + 130.0f, 0.0f);
    bounceTransform->setSize(180.0f, 180.0f);
    auto bounceMaterial = bounce->getComponent<MeshRenderer>()->getMaterial();
    bounceMaterial->setTexture("texture", bounceTexture);
    bounceMaterial->setFloat("bounceTimes", 3.0f);
    bounceMaterial->setFloat("scaleRange", 0.5f);
    bounceMaterial->setFloat("duration", 1.0f);
    bounceMaterial->setVector("meshCenter", bounceTransform->getCenter());
    window->addChild(bounce);

    auto bounceTween = Tween::create(0.0f, 1.0f, 1.0f);
    bounceTween->setEase(EaseType::Linear).onUpdate([bounceMaterial](float value) {
        bounceMaterial->setFloat("timeDelta", value);
    }).onComplete([bounceTween]() {
        bounceTween->restart();
    });
    bounceTween->play();
    TweenManager::getInstance().addTween(bounceTween);

    // MRBrakePedal
    addCard(window, L"MRBrakePedal", L"多纹理合成的制动踏板提示动画", right, top, cardWidth, cardHeight);
    auto brakePedalTexture = Texture::create(ImageType::IMAGE);
    brakePedalTexture->setImageUrl("assets/textures/brake_pedal/brakePedal.png");
    auto whiteTexture = Texture::create(ImageType::IMAGE);
    whiteTexture->setImageUrl("assets/textures/brake_pedal/whiteCircle.png");
    auto grayTexture = Texture::create(ImageType::IMAGE);
    grayTexture->setImageUrl("assets/textures/brake_pedal/grayCircle.png");
    auto brakePedal = MRBrakePedal::create();
    brakePedal->getComponent<Transform>()->setPosition(right + 172.0f, top + 112.0f, 0.0f);
    brakePedal->getComponent<Transform>()->setSize(216.0f, 216.0f);
    auto brakeMaterial = brakePedal->getComponent<MeshRenderer>()->getMaterial();
    brakeMaterial->setTexture("brakePedalTexture", brakePedalTexture);
    brakeMaterial->setTexture("grayTexture", grayTexture);
    brakeMaterial->setTexture("whiteTexture", whiteTexture);
    brakeMaterial->setFloat("duration", 1.0f);
    window->addChild(brakePedal);

    auto brakeTween = Tween::create(0.0f, 1.0f, 1.0f);
    brakeTween->setEase(EaseType::Linear).onUpdate([brakeMaterial](float value) {
        brakeMaterial->setFloat("timeDelta", value);
    }).onComplete([brakeTween]() {
        brakeTween->restart();
    });
    brakeTween->play();
    TweenManager::getInstance().addTween(brakeTween);

    constexpr float bottom = 580.0f;
    constexpr float wideCardWidth = 870.0f;

    // MRFlowingLight
    addCard(window, L"MRFlowingLight", L"沿组件边缘循环移动的流光效果", left, bottom, wideCardWidth, 400.0f);
    auto flowingLight = MRFlowingLight::create();
    flowingLight->getComponent<Transform>()->setPosition(left + 105.0f, bottom + 145.0f, 0.0f);
    flowingLight->getComponent<Transform>()->setSize(660.0f, 120.0f);
    flowingLight->initialize();
    window->addChild(flowingLight);

    // MRFrameAnimation
    addCard(window, L"MRFrameAnimation", L"TextureAtlas 图集区域按 30 FPS 循环播放", 990.0f, bottom, wideCardWidth, 400.0f);
    auto atlas = std::make_shared<TextureAtlas>("assets/textures/frame_animation/atlas_cube.atlas", "assets/textures/frame_animation/", false);
    auto frameAnimation = MRFrameAnimation::create();
    frameAnimation->setTextureAtlas(atlas);
    frameAnimation->getComponent<Transform>()->setPosition(1325.0f, bottom + 115.0f, 0.0f);
    frameAnimation->getComponent<Transform>()->setSize(200.0f, 200.0f);
    window->addChild(frameAnimation);

    LOG_I("AnimationEffectsDemo started");
    engine->render();
    return 0;
}
