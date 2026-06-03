//
// Created by lance on 24-8-13.
//
#include "Engine.h"
#include "base/Transform.h"
#include "elements/MRImage.h"
#include "GlobalTools.h"

using namespace morrow;

#define TEST_BUFFER_WIDTH 48
#define TEST_BUFFER_HEIGHT 48

int main() {
#ifdef QNX
    EngineSharedPtr engine = std::make_shared<Engine>();

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    void* pmem_hdl;

    bool loadData = global_tools::loadPmemData(pmem_hdl, "assets/textures/d_p.rgb");
    auto texture = Texture::create(ImageType::OES);
    if (loadData) {
        texture->setTextureData(pmem_hdl, TEST_BUFFER_WIDTH, TEST_BUFFER_HEIGHT, PixelDataFormat::RGB, 0);
    }

    auto image = MRImage::create();
    auto transform = image->getComponent<Transform>();
    transform->setSize(100, 100);

    // 将按钮添加到窗口
    window->addChild(image);

    engine->render();
#endif
    return 0;
}
