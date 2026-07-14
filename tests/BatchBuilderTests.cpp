#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "core/BatchBuilder.h"

using namespace morrow;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

RenderItem makeItem(const std::string& shader,
                    uintptr_t texture,
                    uint64_t materialState,
                    int32_t displayLayer = 0) {
    RenderItem item;
    item.displayLayer = displayLayer;
    item.batchKey.shaderName = shader;
    item.batchKey.primaryTexture = reinterpret_cast<const void*>(texture);
    item.batchKey.materialStateHash = materialState;
    item.batchKey.blendEnabled = true;
    item.batchKey.srcBlendFactor = 1;
    item.batchKey.dstBlendFactor = 0;
    return item;
}

void testSameMaterialFormsOneBatch() {
    BatchBuilder builder;
    std::vector<RenderItem> items(10, makeItem("image", 0x1000, 1));
    const auto result = builder.build(items);
    expect(result.groups.size() == 1, "10 identical items should form one batch");
    expect(result.groups[0].itemIndices.size() == 10, "the batch should contain all 10 items");
    expect(result.breakReasons.empty(), "identical items should not record a break");
}

void testDifferentTextureBreaksBatch() {
    BatchBuilder builder;
    const auto result = builder.build({
        makeItem("image", 0x1000, 1),
        makeItem("image", 0x2000, 2)
    });
    expect(result.groups.size() == 2, "different textures should form two batches");
    expect(result.breakReasons.size() == 1 &&
           result.breakReasons[0] == BatchBreakReason::Texture,
           "different textures should report Texture");
}

void testPainterOrderIsPreserved() {
    BatchBuilder builder;
    const auto result = builder.build({
        makeItem("imageA", 0x1000, 1),
        makeItem("imageB", 0x2000, 2),
        makeItem("imageA", 0x1000, 1)
    });
    expect(result.groups.size() == 3, "A-B-A must remain three ordered batches");
    expect(result.groups[0].itemIndices[0] == 0 &&
           result.groups[1].itemIndices[0] == 1 &&
           result.groups[2].itemIndices[0] == 2,
           "A-B-A item order must remain unchanged");
}

void testBlendStateBreaksBatch() {
    BatchBuilder builder;
    auto first = makeItem("image", 0x1000, 1);
    auto second = makeItem("image", 0x1000, 2);
    second.batchKey.dstBlendFactor = 1;
    const auto result = builder.build({first, second});
    expect(result.groups.size() == 2, "different blend state should break the batch");
    expect(result.breakReasons[0] == BatchBreakReason::MaterialState,
           "different blend state should report MaterialState");
}

void testDisplayLayerBreaksBatch() {
    BatchBuilder builder;
    const auto result = builder.build({
        makeItem("image", 0x1000, 1, 0),
        makeItem("image", 0x1000, 1, 1)
    });
    expect(result.groups.size() == 2, "different display layer should break the batch");
    expect(result.breakReasons[0] == BatchBreakReason::DisplayLayer,
           "different display layer should report DisplayLayer");
}

} // namespace

int main() {
    testSameMaterialFormsOneBatch();
    testDifferentTextureBreaksBatch();
    testPainterOrderIsPreserved();
    testBlendStateBreaksBatch();
    testDisplayLayerBreaksBatch();

    if (g_failures != 0) {
        std::cerr << g_failures << " BatchBuilder test(s) failed\n";
        return 1;
    }
    std::cout << "All BatchBuilder tests passed\n";
    return 0;
}
