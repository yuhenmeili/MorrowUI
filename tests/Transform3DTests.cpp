#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "ui/base/Transform3D.h"
#include "ui/base/Widget.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void expectNear(float actual, float expected, const std::string& message) {
    constexpr float epsilon = 0.0001f;
    expect(std::fabs(actual - expected) <= epsilon, message);
}

struct TransformNode {
    std::shared_ptr<Widget> widget;
    std::shared_ptr<Transform3D> transform;
};

TransformNode makeTransformNode() {
    auto widget = std::make_shared<Widget>();
    return {widget, widget->addComponent<Transform3D>()};
}

void testStaticWorldMatrixIsReused() {
    auto node = makeTransformNode();
    node.transform->setLocalPosition(1.0f, 2.0f, 3.0f);

    const Matrix4& first = node.transform->getWorldTransformMatrix();
    const uint64_t warmedVersion = node.transform->getWorldVersion();
    const Matrix4& second = node.transform->getWorldTransformMatrix();

    expectNear(first.elements[12], 1.0f,
               "the initial world matrix should contain the local translation");
    expect(first == second, "a static world matrix should keep the same value");
    expect(node.transform->getWorldVersion() == warmedVersion,
           "repeated static reads should not rebuild the world matrix");
}

void testLocalChangeInvalidatesWorldMatrix() {
    auto node = makeTransformNode();
    node.transform->getWorldTransformMatrix();
    const uint64_t initialVersion = node.transform->getWorldVersion();

    node.transform->setLocalPosition(4.0f, 5.0f, 6.0f);
    expect(node.transform->getWorldVersion() == initialVersion,
           "a local change should invalidate lazily");

    const Matrix4& world = node.transform->getWorldTransformMatrix();
    expectNear(world.elements[12], 4.0f,
               "the rebuilt world matrix should contain the new local translation");
    expect(node.transform->getWorldVersion() == initialVersion + 1,
           "the next world read should rebuild exactly once");
}

void testParentChangeInvalidatesChild() {
    auto parent = makeTransformNode();
    auto child = makeTransformNode();
    parent.widget->addChild(child.widget);
    parent.transform->setLocalPosition(10.0f, 0.0f, 0.0f);
    child.transform->setLocalPosition(2.0f, 0.0f, 0.0f);

    expectNear(child.transform->getWorldTransformMatrix().elements[12], 12.0f,
               "the child world matrix should include its parent translation");
    const uint64_t childVersion = child.transform->getWorldVersion();

    parent.transform->setLocalPosition(20.0f, 0.0f, 0.0f);
    expectNear(child.transform->getWorldTransformMatrix().elements[12], 22.0f,
               "a parent change should update the child world matrix");
    expect(child.transform->getWorldVersion() == childVersion + 1,
           "a parent world revision change should rebuild the child once");

    parent.widget->removeChild(child.widget);
}

void testAncestorChangePropagatesThroughUnqueriedParent() {
    auto grandparent = makeTransformNode();
    auto parent = makeTransformNode();
    auto child = makeTransformNode();
    grandparent.widget->addChild(parent.widget);
    parent.widget->addChild(child.widget);
    grandparent.transform->setLocalPosition(1.0f, 0.0f, 0.0f);
    parent.transform->setLocalPosition(2.0f, 0.0f, 0.0f);
    child.transform->setLocalPosition(3.0f, 0.0f, 0.0f);

    expectNear(child.transform->getWorldTransformMatrix().elements[12], 6.0f,
               "the initial child matrix should include the full ancestor chain");
    const uint64_t parentVersion = parent.transform->getWorldVersion();
    const uint64_t childVersion = child.transform->getWorldVersion();

    grandparent.transform->setLocalPosition(10.0f, 0.0f, 0.0f);
    expectNear(child.transform->getWorldTransformMatrix().elements[12], 15.0f,
               "querying only the child should refresh an invalid ancestor chain");
    expect(parent.transform->getWorldVersion() == parentVersion + 1,
           "the unqueried intermediate parent should rebuild during child refresh");
    expect(child.transform->getWorldVersion() == childVersion + 1,
           "the ancestor revision should propagate to the child");

    parent.widget->removeChild(child.widget);
    grandparent.widget->removeChild(parent.widget);
}

void testReparentingInvalidatesWorldMatrix() {
    auto firstParent = makeTransformNode();
    auto secondParent = makeTransformNode();
    auto child = makeTransformNode();
    firstParent.transform->setLocalPosition(10.0f, 0.0f, 0.0f);
    secondParent.transform->setLocalPosition(30.0f, 0.0f, 0.0f);
    child.transform->setLocalPosition(2.0f, 0.0f, 0.0f);
    firstParent.widget->addChild(child.widget);

    expectNear(child.transform->getWorldTransformMatrix().elements[12], 12.0f,
               "the child should initially use the first parent");
    const uint64_t childVersion = child.transform->getWorldVersion();

    secondParent.widget->addChild(child.widget);
    expectNear(child.transform->getWorldTransformMatrix().elements[12], 32.0f,
               "reparenting should update the child world matrix");
    expect(child.transform->getWorldVersion() == childVersion + 1,
           "a parent identity change should rebuild the child once");

    secondParent.widget->removeChild(child.widget);
}

void testExplicitLocalMatrixUsesRevisionTracking() {
    auto node = makeTransformNode();
    Matrix4 firstMatrix;
    firstMatrix.makeTranslation(7.0f, 8.0f, 9.0f);
    node.transform->setLocalTransformMatrix(firstMatrix);

    expectNear(node.transform->getWorldTransformMatrix().elements[12], 7.0f,
               "an explicit local matrix should be used without TRS recomposition");
    const uint64_t firstVersion = node.transform->getWorldVersion();
    node.transform->getWorldTransformMatrix();
    expect(node.transform->getWorldVersion() == firstVersion,
           "an unchanged explicit matrix should reuse the world cache");

    Matrix4 secondMatrix;
    secondMatrix.makeTranslation(11.0f, 12.0f, 13.0f);
    node.transform->setLocalTransformMatrix(secondMatrix);
    expectNear(node.transform->getWorldTransformMatrix().elements[12], 11.0f,
               "replacing an explicit matrix should invalidate the world cache");
    expect(node.transform->getWorldVersion() == firstVersion + 1,
           "an explicit matrix replacement should rebuild exactly once");
}

} // namespace

int main() {
    testStaticWorldMatrixIsReused();
    testLocalChangeInvalidatesWorldMatrix();
    testParentChangeInvalidatesChild();
    testAncestorChangePropagatesThroughUnqueriedParent();
    testReparentingInvalidatesWorldMatrix();
    testExplicitLocalMatrixUsesRevisionTracking();

    if (g_failures != 0) {
        std::cerr << g_failures << " Transform3D test(s) failed\n";
        return 1;
    }
    std::cout << "All Transform3D tests passed\n";
    return 0;
}
