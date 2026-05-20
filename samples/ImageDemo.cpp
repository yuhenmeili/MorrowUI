//
// Created by lance on 2023/2/7.
//

#include "Engine.h"
#include "elements/MRImage.h"
#include "Texture.h"
#include <cstdio>
#include "BasisTextureLoader.h"
#include "base/Transform.h"

using namespace morrow;
using namespace morrow::Math;
using namespace basist;

int main()
{
    EngineOptions engineOptions;
    engineOptions.multithread = false;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto textAtlas = Texture::create(ImageType::IMAGE);
    textAtlas->setImageUrl("../assets/textures/img.bmp");

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
    return 0;
}
