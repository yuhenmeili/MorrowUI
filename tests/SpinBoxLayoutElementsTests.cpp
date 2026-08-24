#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "ui/base/Transform.h"
#include "ui/base/UIWidget.h"
#include "ui/elements/MRButton.h"
#include "ui/elements/MRSeparator.h"
#include "ui/elements/MRSpacer.h"
#include "ui/elements/MRSpinBox.h"
#include "ui/layout/HBoxContainer.h"
#include "ui/layout/MRSplitContainer.h"
#include "ui/layout/VBoxContainer.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

bool near(double left, double right) {
    return std::abs(left - right) < 0.000001;
}

void click(const std::shared_ptr<BaseButton>& button) {
    button->onMouseDown();
    button->onMouseUp();
}

void testSpinBoxSteppingAndClamping() {
    auto spinBox = MRSpinBox::create();
    spinBox->setRange(16.0, 30.0);
    spinBox->setStep(0.5);
    spinBox->setValue(22.0);

    double callbackValue = 0.0;
    auto valueConnection = spinBox->events().onValueChanged.connect(
        [&callbackValue](MRSpinBox&, double value) { callbackValue = value; });

    auto increase = std::dynamic_pointer_cast<MRButton>(spinBox->m_children[3]);
    click(increase);
    expect(near(spinBox->getValue(), 22.5), "increase button should add the configured step");
    expect(near(callbackValue, 22.5), "spin box should report the stepped value");

    spinBox->setValue(100.0);
    expect(near(spinBox->getValue(), 30.0), "spin box value should clamp to its maximum");
}

void testHorizontalSpacerDistribution() {
    auto row = std::make_shared<HBoxContainer>();
    row->setSpacing(10.0f);
    row->getComponent<Transform>()->setSize(300.0f, 40.0f);

    auto left = std::make_shared<UIWidget>(false);
    left->getComponent<Transform>()->setSize(50.0f, 40.0f);
    auto spacer = MRSpacer::create();
    spacer->setMinimumSize(Vector2(20.0f, 1.0f));
    auto right = std::make_shared<UIWidget>(false);
    right->getComponent<Transform>()->setSize(50.0f, 40.0f);

    row->addChild(left);
    row->addChild(spacer);
    row->addChild(right);
    row->update(std::make_shared<FrameState>());

    expect(near(spacer->getComponent<Transform>()->getSize().x, 180.0), "horizontal spacer should consume the remaining width");
    expect(near(right->getComponent<Transform>()->getPosition().x, 250.0), "horizontal spacer should push the trailing item to the far edge");
}

void testVerticalSpacerDistribution() {
    auto column = std::make_shared<VBoxContainer>();
    column->setSpacing(5.0f);
    column->getComponent<Transform>()->setSize(40.0f, 200.0f);

    auto top = std::make_shared<UIWidget>(false);
    top->getComponent<Transform>()->setSize(40.0f, 30.0f);
    auto spacer = MRSpacer::create();
    spacer->setMinimumSize(Vector2(1.0f, 10.0f));
    auto bottom = std::make_shared<UIWidget>(false);
    bottom->getComponent<Transform>()->setSize(40.0f, 30.0f);

    column->addChild(top);
    column->addChild(spacer);
    column->addChild(bottom);
    column->update(std::make_shared<FrameState>());

    expect(near(spacer->getComponent<Transform>()->getSize().y, 130.0), "vertical spacer should consume the remaining height");
    expect(near(bottom->getComponent<Transform>()->getPosition().y, 170.0), "vertical spacer should push the trailing item to the bottom edge");
}

void testSeparatorDefaults() {
    auto horizontal = MRHSeparator::create();
    auto vertical = MRVSeparator::create();
    expect(horizontal->getComponent<Transform>()->getSize().x > horizontal->getComponent<Transform>()->getSize().y, "horizontal separator should default to a wide, thin shape");
    expect(vertical->getComponent<Transform>()->getSize().y > vertical->getComponent<Transform>()->getSize().x, "vertical separator should default to a tall, thin shape");
}

void testSplitContainerLayoutAndMinimums() {
    auto split = MRSplitContainer::create();
    split->setOrientation(SplitOrientation::Horizontal);
    split->setHandleWidth(10.0f);
    split->setFirstMinSize(100.0f);
    split->setSecondMinSize(120.0f);
    split->getTransform()->setSize(500.0f, 200.0f);

    auto first = std::make_shared<UIWidget>(false);
    auto second = std::make_shared<UIWidget>(false);
    split->setFirst(first);
    split->setSecond(second);
    split->setSplitRatio(0.25f);

    expect(near(first->getTransform()->getSize().x, 122.5),
           "horizontal split should size the first panel from the ratio");
    expect(near(second->getTransform()->getPosition().x, 132.5),
           "horizontal split should place the second panel after the handle");
    expect(near(second->getTransform()->getSize().x, 367.5),
           "horizontal split should give the remaining width to the second panel");

    split->setSplitRatio(0.01f);
    expect(first->getTransform()->getSize().x >= 100.0f,
           "split ratio should respect the first panel minimum size");

    split->setSplitRatio(0.99f);
    expect(second->getTransform()->getSize().x >= 120.0f,
           "split ratio should respect the second panel minimum size");
}

}  // namespace

int main() {
    testSpinBoxSteppingAndClamping();
    testHorizontalSpacerDistribution();
    testVerticalSpacerDistribution();
    testSeparatorDefaults();
    testSplitContainerLayoutAndMinimums();

    if (g_failures != 0) {
        std::cerr << g_failures << " spin box/layout element test(s) failed\n";
        return 1;
    }
    std::cout << "All spin box/layout element tests passed\n";
    return 0;
}
