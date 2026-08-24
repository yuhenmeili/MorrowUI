#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "filesystem/FileSystemWatcher.h"
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
    const std::string& contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << contents;
}

void testSelectionAndSorting() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "morrow_file_system_enhancement_model_test";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    writeFile(root / "z.txt", "123456");
    writeFile(root / "a.png", "1");
    writeFile(root / "m.txt", "12");

    ProjectFileSystemModel model;
    std::string message;
    expect(model.scan(root, nullptr, message),
           "enhanced model should scan the temporary project");
    const auto* z = model.findByPath("z.txt");
    const auto* a = model.findByPath("a.png");
    expect(z && a, "test files should be present");
    expect(model.select(z->id, false), "single selection should succeed");
    expect(model.select(a->id, true), "additive selection should succeed");
    expect(model.selectedEntries().size() == 2,
           "additive selection should retain two files");
    expect(model.isSelected("z.txt") && model.isSelected("a.png"),
           "selected paths should be queryable");

    model.setSortMode(FileSortMode::Size, true);
    expect(model.refresh(message), "model should refresh with sort mode");
    expect(model.selectedEntries().size() == 2,
           "selection should survive a refresh");
    std::filesystem::remove_all(root, error);
}

void testWatcherReportsChanges() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "morrow_file_system_enhancement_watcher_test";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    writeFile(root / "before.txt", "before");

    FileSystemWatcher watcher;
    std::string message;
    expect(watcher.start(root, message), "watcher should start");
    writeFile(root / "after.txt", "after");
    bool foundAdded = false;
    for (int attempt = 0; attempt < 8; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        const auto changes = watcher.poll();
        for (const auto& change : changes) {
            if (change.relativePath == "after.txt")
                foundAdded = change.kind == FileChangeKind::Added;
        }
        if (!changes.empty())
            break;
    }
    expect(foundAdded, "watcher should report a newly added file");
    watcher.stop();
    std::filesystem::remove_all(root, error);
}

}  // namespace

int main() {
    testSelectionAndSorting();
    testWatcherReportsChanges();
    if (g_failures != 0) {
        std::cerr << g_failures
                  << " file system enhancement test(s) failed\n";
        return 1;
    }
    std::cout << "All file system enhancement tests passed\n";
    return 0;
}
