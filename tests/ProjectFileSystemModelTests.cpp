#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "filesystem/ProjectFileSystemModel.h"

using namespace morrow::editor;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

void writeFile(
    const std::filesystem::path& path,
    const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << content;
}

void testScanAndFilter() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "morrow_project_file_system_model_test";
    std::error_code errorCode;
    std::filesystem::remove_all(root, errorCode);

    writeFile(root / "MorrowUI.morrow", "[morrow_project format=1]\n");
    writeFile(root / "assets" / "textures" / "panel.png", "png");
    writeFile(
        root / "assets" / "textures" / "panel.png.import",
        "[import]\n");
    writeFile(root / "scenes" / "main.scene", "[scene]\n");
    writeFile(root / "notes.txt", "notes\n");
    writeFile(root / ".morrow" / "editor.layout", "internal\n");

    ProjectFileSystemModel model;
    std::string error;
    expect(model.scan(root, nullptr, error),
           "project file system should scan a readable project root");
    expect(model.findByPath("assets") &&
               model.findByPath("assets")->directory,
           "scan should include project directories");
    expect(model.findByPath("assets/textures/panel.png") != nullptr,
           "scan should include ordinary project files");
    expect(model.findByPath("notes.txt") != nullptr,
           "scan should include unknown file types");
    expect(model.findByPath("assets/textures/panel.png.import") == nullptr,
           "scan should hide asset import metadata");
    expect(model.findByPath(".morrow/editor.layout") == nullptr,
           "scan should hide editor internal state");

    const auto filtered = model.filteredEntries("panel");
    bool foundRoot = false;
    bool foundAssets = false;
    bool foundTextures = false;
    bool foundPanel = false;
    for (const auto* entry : filtered) {
        foundRoot |= entry->relativePath == ".";
        foundAssets |= entry->relativePath == "assets";
        foundTextures |= entry->relativePath == "assets/textures";
        foundPanel |= entry->relativePath == "assets/textures/panel.png";
    }
    expect(foundRoot && foundAssets && foundTextures && foundPanel,
           "filter should preserve matching entries and their ancestors");

    std::filesystem::remove_all(root, errorCode);
}

}  // namespace

int main() {
    testScanAndFilter();
    if (g_failures != 0) {
        std::cerr << g_failures
                  << " ProjectFileSystemModel test(s) failed\n";
        return 1;
    }
    std::cout << "All ProjectFileSystemModel tests passed\n";
    return 0;
}
