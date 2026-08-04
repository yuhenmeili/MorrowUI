#include "Engine.h"
#include "Window.h"
#include "elements/MR3DSceneView.h"
#include "elements/MRButton.h"
#include "../src/ui/helpers/Scene3DAsyncLoader.h"
#include "base/Transform.h"

using namespace morrow;

int main(int argc, char** argv) {
    const std::string modelPath = (argc > 1) ? argv[1] : "assets/models/2018_bmw_m5/scene.gltf";
    // const std::string modelPath = (argc > 1) ? argv[1] : "assets/models/box/box.gltf";
    const std::string iblDirectory = (argc > 2) ? argv[2] : "assets/textures/ibl/symmetrical_garden_1k";

    WindowInfo windowInfo;
    windowInfo.name = "GLTFDemo";
    windowInfo.width = 1920;
    windowInfo.height = 1080;
    EngineOptions engineOptions;
    engineOptions.windowInfo = windowInfo;
    engineOptions.samples = 4;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);
    auto window = engine->getWindow();
    window->setClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    auto scene3DView = MR3DSceneView::create(windowInfo.width, windowInfo.height);
    scene3DView->setSceneClearColor(Vector4(0.22f, 0.23f, 0.31f, 1.0f));
    scene3DView->setSunLight(Vector3(-0.45f, -1.0f, -0.30f), Vector3(1.0f, 0.98f, 0.95f), 5.5f);
    scene3DView->setAmbientLight(Vector3(0.50f, 0.54f, 0.68f), 0.34f);
    scene3DView->setIBLFromDirectory(iblDirectory, 1.0f);

    if (auto orbitController = scene3DView->getOrbitController()) {
        orbitController->setRotateSpeed(0.0085f);
        orbitController->setZoomSpeed(0.14f);
        orbitController->setMinDistance(1.0f);
        orbitController->setMaxDistance(10000.0f);
    }

    scene3DView->getOrbitCamera()->setPosition(0.0f, 3.0f, 10.0f);
    scene3DView->getOrbitCamera()->lookAt(0.0f, 0.0f, 0.0f);

    auto sceneLoader = Scene3DAsyncLoader::create(engine, scene3DView);
    Scene3DAsyncLoader::GLTFLoadOptions loadOptions;
    loadOptions.sceneOptions.debugLabel = modelPath;
    loadOptions.sceneOptions.cameraFit.enabled = true;
    loadOptions.sceneOptions.cameraFit.paddingScale = 1.25f;
    loadOptions.sceneOptions.onError = [modelPath](const std::string& error) {
        LOG_E("Failed to load GLTF '{}': {}", modelPath, error);
    };
    loadOptions.onSceneBuilt = [modelPath, iblDirectory](const std::shared_ptr<GLTFScene>& gltfScene,
                                                         const std::shared_ptr<SceneNode>& sceneRoot) {
        if (!gltfScene || !sceneRoot) {
            return;
        }

        LOG_I("GLTF loaded successfully ({} nodes, {} meshes, {} animations)",
              gltfScene->nodes.size(), gltfScene->meshes.size(), gltfScene->animations.size());

        LOG_I("GLTF loaded: {}  nodes={}  meshes={}  anims={}", modelPath, gltfScene->nodes.size(), gltfScene->meshes.size(), gltfScene->animations.size());
        LOG_I("Using IBL directory: {}", iblDirectory);
        LOG_I("Orbit controls: left-drag rotate, mouse wheel zoom");
    };
    sceneLoader->loadGLTF(modelPath, loadOptions);

    window->addChild(scene3DView);


    auto button = MRButton::create();
    button->setText(L"Button", "default");
    auto transform = button->getComponent<Transform>();
    transform->setPosition(100.0f, 100.0f, 0.0f);
    transform->setSize(100.0f, 100.0f);
    window->addChild(button);

    button->setOnClickCallback([]() {
        LOG_I("Button clicked!");
    });


    // auto radius = 10.0f; // 相机到原点的距离
    // auto height = 3.0f; // Y 坐标（高度）
    // float angle = 0.0f; // 初始角度
    // engine->preRender().add([&]() {
    //     // 每帧更新角度，控制旋转速度
    //     angle += 0.01f; // 可根据需要调整旋转速度
    //     // radius -= 0.001f;
    //
    //     // 计算相机位置：绕 Y 轴旋转
    //     float x = radius * std::cos(angle);
    //     float z = radius * std::sin(angle);
    //
    //     scene3DView->getOrbitCamera()->setPosition(x, height, z);
    // });

    engine->render();

    return 0;
}
