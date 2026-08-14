#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "Engine.h"
#include "FontManager.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "scene/SceneDocument.h"
#include "scene/SceneInstantiator.h"

using namespace morrow;

namespace {
struct EditorOptions {
    std::filesystem::path projectPath;
    std::filesystem::path scenePath;
    bool sceneExplicit = false;
    bool showHelp = false;
};

void printUsage(const char* executable) {
    std::cout
        << "Usage: " << executable << " [--project <MorrowUI.morrow>]"
        << " [--scene <path/to/main.scene>]\n";
}

std::filesystem::path findDefaultProjectPath(const char* executable) {
    std::error_code error;
    const auto currentProject =
        std::filesystem::current_path(error) / "MorrowUI.morrow";
    if (!error && std::filesystem::exists(currentProject)) {
        return currentProject;
    }

    auto executablePath = std::filesystem::absolute(executable, error);
    if (!error) {
        for (auto directory = executablePath.parent_path();
             !directory.empty();
             directory = directory.parent_path()) {
            const auto projectPath = directory / "MorrowUI.morrow";
            if (std::filesystem::exists(projectPath)) {
                return projectPath;
            }
            if (directory == directory.parent_path()) {
                break;
            }
        }
    }

#ifdef MORROW_EDITOR_TEST_PROJECT
    const std::filesystem::path testProjectPath = MORROW_EDITOR_TEST_PROJECT;
    if (std::filesystem::exists(testProjectPath)) {
        return testProjectPath;
    }
#endif

    return currentProject;
}

bool parseArguments(int argc, char** argv, EditorOptions& options) {
    options.projectPath = findDefaultProjectPath(argv[0]);

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            printUsage(argv[0]);
            options.showHelp = true;
            return false;
        }
        if (argument == "--project" || argument == "--scene") {
            if (index + 1 >= argc) {
                std::cerr << "Missing value for " << argument << "\n";
                printUsage(argv[0]);
                return false;
            }
            const std::filesystem::path value = argv[++index];
            if (argument == "--project") {
                options.projectPath = value;
            } else {
                options.scenePath = value;
                options.sceneExplicit = true;
            }
            continue;
        }

        std::cerr << "Unknown argument: " << argument << "\n";
        printUsage(argv[0]);
        return false;
    }

    if (!options.sceneExplicit) {
        options.scenePath = options.projectPath.parent_path() / "scenes" / "main.scene";
    }
    return true;
}

bool validateEditorInputs(const EditorOptions& options) {
    bool valid = true;
    if (!std::filesystem::exists(options.projectPath)) {
        std::cerr << "Project file does not exist: " << options.projectPath << "\n";
        valid = false;
    } else if (options.projectPath.extension() != ".morrow") {
        std::cerr << "Project file must use the .morrow extension: "
                  << options.projectPath << "\n";
        valid = false;
    }

    if (std::filesystem::exists(options.scenePath) &&
        options.scenePath.extension() != ".scene") {
        std::cerr << "Scene file must use the .scene extension: "
                  << options.scenePath << "\n";
        valid = false;
    }
    return valid;
}

class EditorApplication {
public:
    explicit EditorApplication(EditorOptions options)
        : m_options(std::move(options)) {}

    int run() {
        std::cout << "MorrowEditor\n"
                  << "  project: " << m_options.projectPath << "\n"
                  << "  scene:   " << m_options.scenePath << "\n";

        if (!std::filesystem::exists(m_options.scenePath)) {
            std::cout << "  scene status: not found; using built-in bootstrap preview\n";
        } else {
            std::cout << "  scene status: found; scene parsing will be enabled in Phase 1\n";
        }

        EngineOptions engineOptions;
        engineOptions.multithread = false;
        engineOptions.enableRequestRender = false;
        engineOptions.windowInfo.name = "MorrowEditor Preview";
        engineOptions.windowInfo.width = 1280;
        engineOptions.windowInfo.height = 720;

        EngineSharedPtr engine = std::make_shared<morrow::Engine>(engineOptions);
        auto window = engine->getWindow();
        window->setClearColor(0.12f, 0.14f, 0.17f, 1.0f);

        FontInfo fontInfo = {
            .name = "default",
            .path = "assets/fonts/MorrowSansCN1.1-Regular.otf"
        };
        engine->addFonts({fontInfo});

        if (std::filesystem::exists(m_options.scenePath)) {
            editor::SceneDocument document;
            std::string error;
            if (!editor::SceneDocument::loadFromFile(m_options.scenePath, document, error)) {
                std::cerr << "Failed to load scene: " << error << "\n";
                return 3;
            }
            if (!editor::SceneInstantiator::instantiate(document, window, error)) {
                std::cerr << "Failed to instantiate scene: " << error << "\n";
                return 3;
            }
            std::cout << "  scene nodes: " << document.nodes().size() << "\n";
        } else {
            auto button = MRButton::create();
            button->setText(L"MorrowEditor Bootstrap", "default");

            auto transform = button->getComponent<Transform>();
            transform->setPosition(80.0f, 80.0f, 0.0f);
            transform->setSize(280.0f, 64.0f);

            window->addChild(button);
        }
        engine->render();
        return 0;
    }

private:
    EditorOptions m_options;
};
} // namespace

int main(int argc, char** argv) {
    EditorOptions options;
    if (!parseArguments(argc, argv, options)) {
        return options.showHelp ? 0 : 2;
    }
    if (!validateEditorInputs(options)) {
        return 2;
    }
    return EditorApplication(std::move(options)).run();
}
