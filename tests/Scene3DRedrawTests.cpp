#include <iostream>
#include <memory>
#include <string>

#include "OrbitCamera.h"
#include "renderer/resource/Material.h"
#include "renderer/resource/Texture.h"
#include "ui/base/ComponentManager.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

class ContinuousComponent final : public Component {
public:
    bool requiresContinuousUpdate() const override {
        return true;
    }
};

void testOrbitCameraChangeCallback() {
    OrbitCamera camera;
    int callbackCount = 0;
    camera.setChangeCallback([&callbackCount]() {
        ++callbackCount;
    });

    camera.setPosition(1.0f, 2.0f, 3.0f);
    expect(callbackCount == 1, "camera position changes should invalidate the scene");

    camera.setPosition(1.0f, 2.0f, 3.0f);
    expect(callbackCount == 1, "reapplying the same camera position should not invalidate");

    camera.setAngles(0.5f, 0.25f);
    expect(callbackCount == 2, "camera orbit changes should invalidate the scene");

    camera.update(0.0f, 0.0f, 1280.0f, 720.0f);
    expect(callbackCount == 2, "camera matrix refresh should not recursively invalidate");
}

void testContinuousComponentDetection() {
    ComponentManager components;
    auto component = components.addComponent<ContinuousComponent>();

    expect(components.requiresContinuousUpdate(),
           "enabled continuous components should keep the scene pass active");

    component->setEnabled(false);
    expect(!components.requiresContinuousUpdate(),
           "disabled continuous components should allow the scene pass to become static");
}

void testMaterialTextureRevisionPropagation() {
    auto material = Material::create();
    auto texture = Texture::create();
    const uint64_t emptyMaterialHash = material->getRenderRevisionHash();
    material->setTexture("baseColorTexture", texture);
    const uint64_t initialHash = material->getRenderRevisionHash();

    expect(initialHash != emptyMaterialHash,
           "adding a lazily bound scene texture should change the material fingerprint");

    texture->setWidth(64);
    expect(material->getRenderRevisionHash() != initialHash,
           "texture content revisions should invalidate the material render fingerprint");
}

} // namespace

int main() {
    testOrbitCameraChangeCallback();
    testContinuousComponentDetection();
    testMaterialTextureRevisionPropagation();

    if (g_failures == 0) {
        std::cout << "Scene3D redraw tests passed\n";
    }
    return g_failures == 0 ? 0 : 1;
}
