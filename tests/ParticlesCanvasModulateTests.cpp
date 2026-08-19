#include <cmath>
#include <iostream>
#include <string>

#include "ui/base/Mesh.h"
#include "ui/base/MeshFilter.h"
#include "ui/base/Transform.h"
#include "ui/elements/MRCanvasModulate.h"
#include "ui/elements/MRParticles2D.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

bool near(float left, float right) {
    return std::abs(left - right) < 0.00001f;
}

void testCpuParticleMesh() {
    auto particles = MRCPUParticles2D::create();
    particles->setAmount(32);
    particles->getComponent<Transform>()->setSize(300.0f, 180.0f);

    const auto mesh = particles->getComponent<MeshFilter>()->getMesh();
    expect(particles->getAmount() == 32, "CPU particles should keep the configured amount");
    expect(mesh->getVertexCount() == 32 * 4, "CPU particles should generate one quad per active particle");
    expect(mesh->getIndexCount() == 32 * 6, "CPU particle quads should generate six indices each");

    particles->setEmitting(false);
    expect(!particles->isEmitting(), "CPU particles should expose their emission state");
}

void testGpuParticleSeedMesh() {
    auto particles = MRGPUParticles2D::create();
    particles->setAmount(128);
    particles->getComponent<Transform>()->setSize(400.0f, 240.0f);

    const auto mesh = particles->getComponent<MeshFilter>()->getMesh();
    expect(particles->getAmount() == 128, "GPU particles should keep the configured amount");
    expect(mesh->getVertexCount() == 128 * 4, "GPU particles should create one static seed quad per particle");
    expect(mesh->getColors().size() == 128 * 4, "GPU particles should encode seeds in vertex colors");
}

void testCanvasModulateState() {
    auto modulate = MRCanvasModulate::create();
    modulate->setModulateColor(Vector4(0.3f, 0.4f, 0.6f, 1.0f));
    modulate->setStrength(0.65f);
    expect(near(modulate->getStrength(), 0.65f), "canvas modulation should store the configured strength");

    modulate->setNightMode(true);
    expect(modulate->isNightMode() && near(modulate->getStrength(), 1.0f), "night mode should enable full modulation");
    modulate->setNightMode(false);
    expect(!modulate->isNightMode() && near(modulate->getStrength(), 0.0f), "disabling night mode should restore the original canvas");
}

}  // namespace

int main() {
    testCpuParticleMesh();
    testGpuParticleSeedMesh();
    testCanvasModulateState();

    if (g_failures != 0) {
        std::cerr << g_failures << " particle/canvas modulation test(s) failed\n";
        return 1;
    }
    std::cout << "All particle/canvas modulation tests passed\n";
    return 0;
}
