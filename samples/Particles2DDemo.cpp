#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRColor.h"
#include "elements/MRLabel.h"
#include "elements/MRParticles2D.h"

using namespace morrow;

namespace {

std::shared_ptr<MRLabel> createTitle(const std::wstring& text, float x, float y, float width) {
    auto label = std::make_shared<MRLabel>();
    label->setText(text, "default");
    label->setFontSize(28.0f);
    label->setFontColor(0.9f, 0.94f, 1.0f, 1.0f);
    label->setAlign(HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    label->getComponent<Transform>()->setPosition(x, y, 0.0f);
    label->getComponent<Transform>()->setSize(width, 48.0f);
    return label;
}

std::shared_ptr<MRColor> createPanel(float x, float y, float width, float height) {
    auto panel = MRColor::create();
    panel->setColor(0.035f, 0.055f, 0.1f, 1.0f);
    panel->setRounding(10.0f);
    panel->getComponent<Transform>()->setPosition(x, y, 0.0f);
    panel->getComponent<Transform>()->setSize(width, height);
    return panel;
}

}  // namespace

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(0.015f, 0.025f, 0.055f, 1.0f);

    FontInfo fontInfo = {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"};
    engine->addFonts({fontInfo});

    constexpr float panelY = 150.0f;
    constexpr float panelWidth = 700.0f;
    constexpr float panelHeight = 560.0f;
    constexpr float leftX = 180.0f;
    constexpr float rightX = 1040.0f;

    window->addChild(createPanel(leftX, panelY, panelWidth, panelHeight));
    window->addChild(createPanel(rightX, panelY, panelWidth, panelHeight));
    window->addChild(createTitle(L"MRCPUParticles2D：CPU 飘落粒子", leftX, 86.0f, panelWidth));
    window->addChild(createTitle(L"MRGPUParticles2D：GPU 发光氛围", rightX, 86.0f, panelWidth));

    auto cpuParticles = MRCPUParticles2D::create();
    cpuParticles->getComponent<Transform>()->setPosition(leftX, panelY, 0.0f);
    cpuParticles->getComponent<Transform>()->setSize(panelWidth, panelHeight);
    cpuParticles->setAmount(180);
    cpuParticles->setLifetime(5.0f);
    cpuParticles->setEmissionSize(Vector2(panelWidth - 80.0f, 40.0f));
    cpuParticles->setVelocityRange(Vector2(-18.0f, -75.0f), Vector2(18.0f, -35.0f));
    cpuParticles->setGravity(Vector2(0.0f, -12.0f));
    cpuParticles->setSizeRange(5.0f, 12.0f);
    cpuParticles->setColorGradient(Vector4(1.0f, 0.82f, 0.28f, 0.95f), Vector4(1.0f, 0.18f, 0.03f, 0.0f));
    cpuParticles->setAdditiveBlend(true);
    window->addChild(cpuParticles);

    auto gpuParticles = MRGPUParticles2D::create();
    gpuParticles->getComponent<Transform>()->setPosition(rightX, panelY, 0.0f);
    gpuParticles->getComponent<Transform>()->setSize(panelWidth, panelHeight);
    gpuParticles->setAmount(900);
    gpuParticles->setLifetime(6.0f);
    gpuParticles->setEmissionSize(Vector2(panelWidth - 100.0f, panelHeight - 100.0f));
    gpuParticles->setVelocityRange(Vector2(-8.0f, -24.0f), Vector2(8.0f, 24.0f));
    gpuParticles->setGravity(Vector2(0.0f, 0.0f));
    gpuParticles->setParticleSize(13.0f);
    gpuParticles->setColorGradient(Vector4(0.25f, 0.9f, 1.0f, 0.95f), Vector4(0.28f, 0.18f, 1.0f, 0.0f));
    gpuParticles->setAdditiveBlend(true);
    window->addChild(gpuParticles);

    LOG_I("Particles2D demo started");
    engine->render();
    return 0;
}
