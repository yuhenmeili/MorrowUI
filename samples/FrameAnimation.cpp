//
// Created by lance on 24-7-5.
//
//
#include "Engine.h"
#include "atlas/TextureAtlas.h"
#include "base/Transform.h"
#include "elements/MRFrameAnimation.h"

using namespace morrow;
using namespace morrow::Math;

int main()
{
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    auto atlas = std::make_shared<TextureAtlas>("assets/textures/frame_animation/atlas_cube.atlas", "assets/textures/frame_animation/", false);
    auto animation = MRFrameAnimation::create();
    animation->setTextureAtlas(atlas);

    auto transform = animation->getComponent<Transform>();
    transform->setPosition(200.0f, 200.0f, 0.0f);
    transform->setSize(200.0f, 200.0f);

    window->addChild(animation);
    engine->render();
    return 0;
}
