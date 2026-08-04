#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Material.h"
#include "base/Mesh.h"

using namespace morrow;
using namespace morrow::Math;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void testMaterialBatchCompatibilityCache() {
    const auto material = Material::create();
    const uint64_t initialHash = material->getBatchCompatibilityHash();
    const uint64_t initialBatchRevision =
        material->getBatchCompatibilityRevision();
    const uint64_t initialUniformRevision = material->getUniformRevision();

    expect(material->getBatchCompatibilityHash() == initialHash,
           "unchanged material should reuse the same compatibility key");

    material->setFloat("opacity", 0.5f);
    expect(material->getBatchCompatibilityRevision() == initialBatchRevision,
           "uniform-only material changes should preserve compatibility revision");
    expect(material->getUniformRevision() > initialUniformRevision,
           "uniform-only material changes should advance uniform revision");
    expect(material->getBatchCompatibilityHash() == initialHash,
           "uniform-only material changes should preserve batch compatibility");

    material->setBlendEnabled(false);
    const uint64_t blendHash = material->getBatchCompatibilityHash();
    expect(material->getBatchCompatibilityRevision() > initialBatchRevision,
           "blend state changes should advance compatibility revision");
    expect(blendHash != initialHash,
           "blend state changes should invalidate material batch compatibility");
    expect(material->getBatchCompatibilityHash() == blendHash,
           "unchanged blend state should reuse the cached compatibility key");
}

void testMeshBatchCompatibilityCache() {
    Mesh mesh;
    const uint64_t initialHash = mesh.getBatchCompatibilityHash();

    mesh.setVertices({Vector3(0.0f, 0.0f, 0.0f)});
    expect(mesh.getBatchCompatibilityHash() == initialHash,
           "vertex content changes should preserve mesh batch compatibility");

    mesh.setUVs({Vector2(0.0f, 0.0f)});
    const uint64_t uvLayoutHash = mesh.getBatchCompatibilityHash();
    expect(uvLayoutHash != initialHash,
           "adding a vertex attribute should invalidate mesh batch compatibility");

    mesh.setUVs({Vector2(1.0f, 1.0f)});
    expect(mesh.getBatchCompatibilityHash() == uvLayoutHash,
           "vertex attribute content changes should reuse the cached layout key");

    mesh.setDrawMode(PrimitiveType::LINES);
    expect(mesh.getBatchCompatibilityHash() != uvLayoutHash,
           "primitive topology changes should invalidate mesh batch compatibility");
}

} // namespace

int main() {
    testMaterialBatchCompatibilityCache();
    testMeshBatchCompatibilityCache();

    if (g_failures != 0) {
        std::cerr << g_failures << " batch compatibility cache test(s) failed\n";
        return 1;
    }
    std::cout << "All batch compatibility cache tests passed\n";
    return 0;
}
