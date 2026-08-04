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

RenderItem makeItem(uint64_t materialState, int32_t displayLayer = 0) {
    RenderItem item;
    item.displayLayer = displayLayer;
    item.batchKey.materialStateHash = materialState;
    return item;
}

void testSameMaterialFormsOneBatch() {
    BatchBuilder builder;
    std::vector<RenderItem> items(10, makeItem(1));
    const auto result = builder.build(items);
    expect(result.groups.size() == 1, "10 identical items should form one batch");
    expect(result.groups[0].itemIndices.size() == 10, "the batch should contain all 10 items");
    expect(result.breakReasons.empty(), "identical items should not record a break");
}

void testDifferentTextureBreaksBatch() {
    BatchBuilder builder;
    const auto result = builder.build({
        makeItem(1),
        makeItem(2)
    });
    expect(result.groups.size() == 2, "different textures should form two batches");
    expect(result.breakReasons.size() == 1 &&
           result.breakReasons[0] == BatchBreakReason::MaterialState,
           "different material state should report MaterialState");
}

void testPainterOrderIsPreserved() {
    BatchBuilder builder;
    const auto result = builder.build({
        makeItem(1),
        makeItem(2),
        makeItem(1)
    });
    expect(result.groups.size() == 3, "A-B-A must remain three ordered batches");
    expect(result.groups[0].itemIndices[0] == 0 &&
           result.groups[1].itemIndices[0] == 1 &&
           result.groups[2].itemIndices[0] == 2,
           "A-B-A item order must remain unchanged");
}

void testBlendStateBreaksBatch() {
    BatchBuilder builder;
    auto first = makeItem(1);
    auto second = makeItem(2);
    const auto result = builder.build({first, second});
    expect(result.groups.size() == 2, "different blend state should break the batch");
    expect(result.breakReasons[0] == BatchBreakReason::MaterialState,
           "different blend state should report MaterialState");
}

void testDisplayLayerBreaksBatch() {
    BatchBuilder builder;
    const auto result = builder.build({
        makeItem(1, 0),
        makeItem(1, 1)
    });
    expect(result.groups.size() == 2, "different display layer should break the batch");
    expect(result.breakReasons[0] == BatchBreakReason::DisplayLayer,
           "different display layer should report DisplayLayer");
}

void testGeometryBreaksBatch() {
    BatchBuilder builder;
    auto first = makeItem(1);
    auto second = makeItem(1);
    first.batchKey.meshStateHash = 1;
    second.batchKey.meshStateHash = 2;
    const auto result = builder.build({first, second});
    expect(result.groups.size() == 2, "different mesh state should break the batch");
    expect(result.breakReasons[0] == BatchBreakReason::Geometry,
           "different mesh state should report Geometry");
}

void testResourceRevisionsDoNotInvalidateBatchStructure() {
    auto current = std::vector<RenderItem>{makeItem(1)};
    auto previous = current;
    expect(BatchBuilder::areRenderItemListsEquivalent(current, previous),
           "identical render item lists should be cache-compatible");

    // Resource contents are consumed by their own upload/apply paths. They do
    // not change batch grouping as long as the compatibility key is unchanged.
    current[0] = previous[0];
    expect(BatchBuilder::areRenderItemListsEquivalent(current, previous),
           "resource data changes should not invalidate batch structure");

    current[0].batchKey.materialStateHash++;
    expect(!BatchBuilder::areRenderItemListsEquivalent(current, previous),
           "batch compatibility changes should invalidate the cached list");
}

} // namespace

int main() {
    testSameMaterialFormsOneBatch();
    testDifferentTextureBreaksBatch();
    testPainterOrderIsPreserved();
    testBlendStateBreaksBatch();
    testDisplayLayerBreaksBatch();
    testGeometryBreaksBatch();
    testResourceRevisionsDoNotInvalidateBatchStructure();

    if (g_failures != 0) {
        std::cerr << g_failures << " BatchBuilder test(s) failed\n";
        return 1;
    }
    std::cout << "All BatchBuilder tests passed\n";
    return 0;
}
