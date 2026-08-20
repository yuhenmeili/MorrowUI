//
// Created by lance on 2023/2/7.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "BasisTextureLoader.h"
#include "Engine.h"
#include "Texture.h"
#include "base/Shadow.h"
#include "base/Transform.h"
#include "elements/MRColor.h"
#include "elements/MRImage.h"

using namespace morrow;
using namespace morrow::Math;
using namespace basist;

int main(int argc, char** argv) {
    uint32_t maxFrames = 0;
    bool reportJson = false;
    bool enableRequestRender = false;
    std::string objectSnapshotPath;
    std::string objectSnapshotCommandPath;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--report-json") == 0) {
            reportJson = true;
        } else if (std::strcmp(argv[index], "--request-render") == 0) {
            enableRequestRender = true;
        } else if (std::strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
            maxFrames = static_cast<uint32_t>(std::strtoul(argv[++index], nullptr, 10));
        } else if (std::strcmp(argv[index], "--object-snapshot") == 0 && index + 1 < argc) {
            objectSnapshotPath = argv[++index];
        } else if (std::strcmp(argv[index], "--object-snapshot-command") == 0 && index + 1 < argc) {
            objectSnapshotCommandPath = argv[++index];
        }
    }
    if (reportJson && maxFrames == 0) {
        maxFrames = 5;
    }

    EngineOptions engineOptions;
    engineOptions.multithread = false;
    engineOptions.enableRequestRender = enableRequestRender;
    engineOptions.maxFrames = maxFrames;
    engineOptions.objectSnapshotPath = objectSnapshotPath;
    engineOptions.objectSnapshotCommandPath = objectSnapshotCommandPath;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto textAtlas = Texture::create(ImageType::IMAGE);
    textAtlas->setImageUrl("assets/textures/img.bmp");

    for (int index = 0; index < 10; index++) {
        auto im1 = MRImage::create();
        auto transform = im1->getComponent<Transform>();
        transform->setPosition(index * 100, 100, 0);
        transform->setSize(100.0f, 100.0f + std::abs(sin(index) * 100.0f));

        auto meshRenderer = im1->getComponent<MeshRenderer>();
        auto material = meshRenderer->getMaterial();
        material->setTexture("texture", textAtlas);

        window->addChild(im1);
    }

    // shadow
    auto mr_color = MRColor::create();
    mr_color->setColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    auto transform = mr_color->getComponent<Transform>();
    transform->setPosition(300, 600, 0);
    transform->setSize(100, 100);
    // 添加阴影组件
    auto shadow = mr_color->addComponent<Shadow>();
    shadow->setShadowOffset(Vector2(5.0f, 5.0f));
    shadow->setShadowColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));

    window->addChild(mr_color);

    // shadow
    auto mr_image = MRImage::create();
    mr_image->setTexture(textAtlas);

    auto image_transform = mr_image->getComponent<Transform>();
    image_transform->setPosition(600, 600, 0);
    image_transform->setSize(100, 100);
    // 添加阴影组件
    auto image_shadow = mr_image->addComponent<Shadow>();
    image_shadow->setShadowOffset(Vector2(5.0f, 5.0f));
    image_shadow->setShadowColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));

    window->addChild(mr_image);

    LOG_I("start render");
    engine->render();

    if (reportJson) {
        const auto frameState = engine->getFrameState();
        const auto& stats = frameState->batchStatistics;
        std::cout << "{"
                  << "\"renderItems\":" << stats.renderItemCount << ","
                  << "\"batches\":" << stats.batchCount << ","
                  << "\"ssboBatches\":" << stats.ssboBatchCount << ","
                  << "\"standardBatches\":" << stats.standardBatchCount << ","
                  << "\"batchDrawCalls\":" << stats.batchDrawCallCount << ","
                  << "\"drawCalls\":" << frameState->drawCallCount << ","
                  << "\"cacheHits\":" << stats.cacheHitCount << ","
                  << "\"cacheMisses\":" << stats.cacheMissCount << ","
                  << "\"ssboSupported\":" << (frameState->isSSBOSupport ? "true" : "false") << "}" << std::endl;

        if (frameState->isSSBOSupport && (stats.renderItemCount != 12 || stats.batchCount != 2 || stats.ssboBatchCount != 2 || frameState->drawCallCount != 2)) {
            std::cerr << "ImageDemo batch acceptance failed" << std::endl;
            return 2;
        }
    }
    return 0;
}
