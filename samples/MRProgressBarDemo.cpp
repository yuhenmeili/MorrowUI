//
// Created by lance on 2026/8/17.
//

#include "Engine.h"
#include "base/Transform.h"
#include "elements/MRProgressBar.h"
#include "ui/helpers/Tween.h"

using namespace morrow;

int main() {
    auto engine = std::make_shared<Engine>();
    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // 1. 横向默认（左→右），圆角，Tween 循环动画填充
    auto bar1 = MRProgressBar::create();
    bar1->getComponent<Transform>()->setPosition(100.0f, 100.0f, 0.0f);
    bar1->getComponent<Transform>()->setSize(300.0f, 24.0f);
    bar1->setTrackColor(0.85f, 0.85f, 0.85f, 1.0f);
    bar1->setFillColor(0.2f, 0.6f, 0.9f, 1.0f);
    bar1->setRounding(12.0f);
    window->addChild(bar1);

    auto tween = Tween::create(0.0f, 1.0f, 2.0f);
    tween->setEase(EaseType::Linear)
          .onUpdate([bar1](float value) { bar1->setProgress(value); })
          .onComplete([tween]() { tween->restart(); });
    tween->play();
    TweenManager::getInstance().addTween(tween);

    // 2. 横向反向（右→左）
    auto bar2 = MRProgressBar::create();
    bar2->getComponent<Transform>()->setPosition(100.0f, 160.0f, 0.0f);
    bar2->getComponent<Transform>()->setSize(300.0f, 24.0f);
    bar2->setDirection(ProgressDirection::RightToLeft);
    bar2->setTrackColor(0.9f, 0.9f, 0.9f, 1.0f);
    bar2->setFillColor(0.9f, 0.4f, 0.2f, 1.0f);
    bar2->setRounding(12.0f);
    bar2->setProgress(0.7f);
    window->addChild(bar2);

    // 3. 纵向（下→上）
    auto bar3 = MRProgressBar::create();
    bar3->getComponent<Transform>()->setPosition(460.0f, 100.0f, 0.0f);
    bar3->getComponent<Transform>()->setSize(24.0f, 200.0f);
    bar3->setDirection(ProgressDirection::BottomToTop);
    bar3->setTrackColor(0.9f, 0.9f, 0.9f, 1.0f);
    bar3->setFillColor(0.3f, 0.8f, 0.4f, 1.0f);
    bar3->setRounding(12.0f);
    bar3->setProgress(0.4f);
    window->addChild(bar3);

    LOG_I("start render progress bar demo");
    engine->render();
    return 0;
}
