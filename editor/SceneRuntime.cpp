#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "Engine.h"
#include "FontManager.h"
#include "ProjectSettings.h"
#include "assets/AssetDatabase.h"
#include "scene/SceneDocument.h"
#include "scene/SceneInstantiator.h"

namespace {

struct RuntimeOptions {
    std::filesystem::path projectPath;
    std::filesystem::path scenePath;
    uint32_t maxFrames = 0;
    bool showHelp = false;
};

void printUsage(const char* executable) {
    std::cout << "Usage: " << executable << " --project <project.morrow>"
              << " [--scene <scene.scene>]"
              << " [--max-frames <count>]\n";
}

bool parseArguments(int argc, char** argv, RuntimeOptions& options) {
#ifdef MORROW_RUNTIME_TEST_PROJECT
    options.projectPath = MORROW_RUNTIME_TEST_PROJECT;
#endif
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            options.showHelp = true;
            printUsage(argv[0]);
            return false;
        }
        if (argument == "--project" || argument == "--scene" || argument == "--max-frames") {
            if (index + 1 >= argc) {
                std::cerr << "Missing value for " << argument << '\n';
                return false;
            }
            const std::string value = argv[++index];
            if (argument == "--project")
                options.projectPath = value;
            else if (argument == "--scene")
                options.scenePath = value;
            else
                options.maxFrames = static_cast<uint32_t>(std::stoul(value));
            continue;
        }
        std::cerr << "Unknown argument: " << argument << '\n';
        return false;
    }
    if (options.projectPath.empty()) {
        std::cerr << "A .morrow project path is required\n";
        return false;
    }
    return true;
}

int integerSetting(const morrow::editor::ProjectSettings& project, const std::string& name, int fallback) {
    try {
        return std::stoi(project.value(name, std::to_string(fallback)));
    } catch (...) {
        return fallback;
    }
}

}  // namespace

int main(int argc, char** argv) {
    RuntimeOptions options;
    if (!parseArguments(argc, argv, options))
        return options.showHelp ? 0 : 2;

    morrow::editor::ProjectSettings project;
    std::string error;
    if (!project.load(options.projectPath, error)) {
        std::cerr << "Failed to load project: " << error << '\n';
        return 3;
    }
    if (options.scenePath.empty()) {
        options.scenePath = project.pathValue("default_scene", "scenes/main.scene");
    } else if (options.scenePath.is_relative()) {
        options.scenePath = (project.projectRoot() / options.scenePath).lexically_normal();
    }

    morrow::editor::SceneDocument document;
    if (!morrow::editor::SceneDocument::loadFromFile(options.scenePath, document, error)) {
        std::cerr << "Failed to load scene: " << error << '\n';
        return 4;
    }

    morrow::editor::AssetDatabase assets;
    if (!assets.scan(project.projectRoot(), project.value("asset_root", "assets"), error)) {
        std::cerr << "Failed to scan assets: " << error << '\n';
        return 5;
    }

    morrow::EngineOptions engineOptions;
    engineOptions.multithread = false;
    engineOptions.enableRequestRender = false;
    engineOptions.maxFrames = options.maxFrames;
    engineOptions.windowInfo.name = project.value("name", "Morrow Scene");
    engineOptions.windowInfo.width = integerSetting(project, "runtime_width", 1280);
    engineOptions.windowInfo.height = integerSetting(project, "runtime_height", 720);

    auto engine = std::make_shared<morrow::Engine>(engineOptions);
    auto window = engine->getWindow();
    window->setClearColor(0.12f, 0.14f, 0.17f, 1.0f);
    engine->addFonts({
        {.name = "default", .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"},
    });

    if (!morrow::editor::SceneInstantiator::instantiate(document, window, &assets, error)) {
        std::cerr << "Failed to instantiate scene: " << error << '\n';
        return 6;
    }

    std::cout << "MorrowSceneRuntime\n"
              << "  project: " << project.projectPath() << '\n'
              << "  scene:   " << options.scenePath << '\n';
    engine->render();
    return 0;
}
