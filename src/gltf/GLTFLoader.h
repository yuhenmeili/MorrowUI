//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_GLTFLOADER_H
#define MORROW_GUI_GLTFLOADER_H

#include <string>
#include <memory>
#include "GLTFTypes.h"

namespace morrow {

/**
 * GLTFLoader – two-phase loader:
 *
 *  Phase A (background thread): tinygltf parse → GLTFScene (pure CPU data).
 *         VBOData are packed here; texture pixel bytes are kept alive via
 *         ordinary CPU texture data copied by RenderDeviceProxy in threaded mode.
 *
 *  Phase B (main/callback thread): caller hands the GLTFScene to
 *         GLTFSceneBuilder which uploads GPU resources via the proxy.
 *
 * Usage:
 *   GLTFLoader::loadAsync("model.glb", [](auto scene, auto err){ ... });
 *   GLTFLoader::loadSync ("model.glb")  → returns GLTFScene or nullptr
 */
class GLTFLoader {
public:
    /**
     * Asynchronous load. The callback runs on an internal worker thread;
     * only CPU-side parsing should happen there. Forward scene/UI/GPU work to
     * the main thread before touching engine objects.
     */
    static void loadAsync(const std::string& path, GLTFLoadCallback callback);

    /**
     * Synchronous load (blocks caller thread).  Returns nullptr on failure
     * and fills outError.
     */
    static std::shared_ptr<GLTFScene> loadSync(const std::string& path,
                                               std::string&        outError);
};

} // namespace morrow

#endif //MORROW_GUI_GLTFLOADER_H

