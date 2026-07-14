//
// Created by lance on 2023/2/7.
//

#include "Engine.h"
#include "elements/MRImage.h"
#include "Texture.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BasisTextureLoader.h"
#include "base/Transform.h"

using namespace morrow;
using namespace morrow::Math;
using namespace basist;

int main(int argc, char** argv)
{
    uint32_t maxFrames = 0;
    bool reportJson = false;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--report-json") == 0) {
            reportJson = true;
        } else if (std::strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
            maxFrames = static_cast<uint32_t>(std::strtoul(argv[++index], nullptr, 10));
        }
    }
    if (reportJson && maxFrames == 0) {
        maxFrames = 5;
    }

    EngineOptions engineOptions;
    engineOptions.multithread = false;
    engineOptions.maxFrames = maxFrames;
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
                  << "\"ssboSupported\":" << (frameState->isSSBOSupport ? "true" : "false")
                  << "}" << std::endl;

        if (frameState->isSSBOSupport &&
            (stats.renderItemCount != 12 ||
             stats.batchCount != 2 ||
             stats.ssboBatchCount != 2 ||
             frameState->drawCallCount != 2)) {
            std::cerr << "ImageDemo batch acceptance failed" << std::endl;
            return 2;
        }
    }
    return 0;
}
