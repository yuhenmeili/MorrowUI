//
// DebugDemo.cpp — 引擎调试能力演示
//
// 集中演示 Engine 的调试 / 观测设施（原 ImageDemo 的验收功能独立成 demo）：
//   1. --report-json               批渲染统计 JSON 输出 + 合批验收断言
//   2. --frames N                  限制渲染帧数（自动化 / 截图测试）
//   3. --request-render            按需渲染模式（无脏事件不重绘）
//   4. --object-snapshot PATH      退出时输出对象注册表快照（按帧号标记文件）
//   5. --object-snapshot-command   命令文件触发的运行期快照（外部工具心跳）
//   6. F3                          运行期切换调试 overlay（FPS / 批次 / draw call）
//
// 场景本身刻意保持简单（10 张同纹理图片 + 2 个阴影组件），
// 使批渲染统计有确定预期值，可作为合批回归的验收基准。
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Engine.h"
#include "Texture.h"
#include "base/Shadow.h"
#include "base/Transform.h"
#include "elements/MRColor.h"
#include "elements/MRImage.h"

using namespace morrow;
using namespace morrow::Math;

int main(int argc, char** argv) {
    uint32_t maxFrames = 0;
    bool reportJson = false;
    bool enableRequestRender = false;
    bool showOverlay = false;
    std::string objectSnapshotPath;
    std::string objectSnapshotCommandPath;
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--report-json") == 0) {
            reportJson = true;
        } else if (std::strcmp(argv[index], "--request-render") == 0) {
            enableRequestRender = true;
        } else if (std::strcmp(argv[index], "--overlay") == 0) {
            showOverlay = true;
        } else if (std::strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
            maxFrames = static_cast<uint32_t>(std::strtoul(argv[++index], nullptr, 10));
        } else if (std::strcmp(argv[index], "--object-snapshot") == 0 && index + 1 < argc) {
            objectSnapshotPath = argv[++index];
        } else if (std::strcmp(argv[index], "--object-snapshot-command") == 0 && index + 1 < argc) {
            objectSnapshotCommandPath = argv[++index];
        } else if (std::strcmp(argv[index], "--help") == 0) {
            std::cout << "DebugDemo [options]\n"
                         "  --report-json                    print batch statistics JSON and assert batch acceptance\n"
                         "  --frames N                       stop after N frames\n"
                         "  --request-render                 on-demand rendering mode\n"
                         "  --overlay                        start with the debug overlay visible (F3 toggles)\n"
                         "  --object-snapshot PATH           write object registry snapshot on exit\n"
                         "  --object-snapshot-command PATH   heartbeat command file triggering runtime snapshots\n";
            return 0;
        }
    }
    if (reportJson && maxFrames == 0) {
        maxFrames = 5;
    }

    EngineOptions engineOptions;
    engineOptions.multithread = false;
    engineOptions.enableRequestRender = enableRequestRender;
    engineOptions.maxFrames = maxFrames;
    engineOptions.debugOverlayVisible = showOverlay;
    engineOptions.objectSnapshotPath = objectSnapshotPath;
    engineOptions.objectSnapshotCommandPath = objectSnapshotCommandPath;
    EngineSharedPtr engine = std::make_shared<Engine>(engineOptions);

    auto window = engine->getWindow();
    window->setClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // ---------------------------------------------------------------------
    // 验收基准场景：10 张共享同一纹理的图片 + 2 个带阴影的组件。
    // 预期（桌面 GL，SSBO 可用，实测校准）：renderItems=12, batches=4,
    // ssboBatches=1, standardBatches=3, drawCalls=5 ——
    // 10 张图片合并为 1 个 SSBO 批次，色块/两个阴影材质各自成批。
    // ---------------------------------------------------------------------
    const auto textAtlas = Texture::create(ImageType::IMAGE);
    textAtlas->setImageUrl("assets/textures/img.bmp");

    for (int index = 0; index < 10; index++) {
        auto image = MRImage::create();
        auto transform = image->getComponent<Transform>();
        transform->setPosition(index * 100, 100, 0);
        transform->setSize(100.0f, 100.0f + std::abs(std::sin(index) * 100.0f));
        image->getComponent<MeshRenderer>()->getMaterial()->setTexture("texture", textAtlas);
        window->addChild(image);
    }

    auto colorBlock = MRColor::create();
    colorBlock->setColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
    colorBlock->getComponent<Transform>()->setPosition(300, 600, 0);
    colorBlock->getComponent<Transform>()->setSize(100, 100);
    {
        auto shadow = colorBlock->addComponent<Shadow>();
        shadow->setShadowOffset(Vector2(5.0f, 5.0f));
        shadow->setShadowColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));
    }
    window->addChild(colorBlock);

    auto shadowedImage = MRImage::create();
    shadowedImage->setTexture(textAtlas);
    shadowedImage->getComponent<Transform>()->setPosition(600, 600, 0);
    shadowedImage->getComponent<Transform>()->setSize(100, 100);
    {
        auto shadow = shadowedImage->addComponent<Shadow>();
        shadow->setShadowOffset(Vector2(5.0f, 5.0f));
        shadow->setShadowColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));
    }
    window->addChild(shadowedImage);

    LOG_I("DebugDemo started — press F3 to toggle the debug overlay");
    if (enableRequestRender) {
        LOG_I("request-render mode is ON: frames render only when dirty events arrive");
    }
    if (!objectSnapshotCommandPath.empty()) {
        LOG_I("snapshot command file: {} (write 'snapshot path=<file>' to trigger)", objectSnapshotCommandPath);
    }
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

        if (frameState->isSSBOSupport &&
            (stats.renderItemCount != 12 || stats.batchCount != 4 || stats.ssboBatchCount != 1 || stats.standardBatchCount != 3 || frameState->drawCallCount != 5)) {
            std::cerr << "DebugDemo batch acceptance failed" << std::endl;
            return 2;
        }
    }
    return 0;
}
