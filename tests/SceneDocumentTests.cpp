#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "commands/CommandHistory.h"
#include "scene/SceneDocument.h"

namespace {

bool require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    namespace fs = std::filesystem;
    using morrow::editor::CommandHistory;
    using morrow::editor::ReparentNodeCommand;
    using morrow::editor::SceneDocument;
    using morrow::editor::SetNodePropertyCommand;

    const auto temporaryDirectory =
        fs::temp_directory_path() / "morrow-scene-document-tests";
    std::error_code filesystemError;
    fs::create_directories(temporaryDirectory, filesystemError);
    if (!require(!filesystemError, "create temporary directory")) return 1;

    const auto inputPath = temporaryDirectory / "input.scene";
    const auto outputPath = temporaryDirectory / "output.scene";
    {
        std::ofstream output(inputPath, std::ios::trunc);
        output
            << "[morrow_scene format=1]\n\n"
            << "[external_resource id=\"texture_001\" type=\"Texture\" "
               "path=\"assets/button.png\"]\n\n"
            << "[sub_resource type=\"Style\" id=\"style_001\"]\n"
            << "property corner_radius = 6.0\n\n"
            << "[node id=\"node_001\" type=\"SceneNode\" name=\"Main\"]\n\n"
            << "[node id=\"node_002\" type=\"MRButton\" "
               "parent=\"node_001\" name=\"StartButton\"]\n"
            << "property text = \"Start\"\n"
            << "property position = Vector3(80.0, 80.0, 0.0)\n"
            << "property size = Vector2(280.0, 64.0)\n";
    }

    SceneDocument document;
    std::string error;
    if (!require(SceneDocument::loadFromFile(inputPath, document, error),
                 "load scene: " + error)) {
        return 1;
    }
    if (!require(document.nodes().size() == 2, "scene node count")) return 1;
    if (!require(document.externalResources().size() == 1,
                 "external resource count")) {
        return 1;
    }
    if (!require(document.subResources().size() == 1,
                 "sub resource count")) {
        return 1;
    }

    CommandHistory history;
    if (!require(
            history.execute(
                std::make_unique<SetNodePropertyCommand>(
                    "node_002", "text", "Updated"),
                document, error),
            "execute property command: " + error)) {
        return 1;
    }
    if (!require(document.findNode("node_002")->properties.at("text") == "Updated",
                 "property command result")) {
        return 1;
    }
    if (!require(history.undo(document, error), "undo property command: " + error)) {
        return 1;
    }
    if (!require(document.findNode("node_002")->properties.at("text") == "Start",
                 "property undo result")) {
        return 1;
    }
    if (!require(history.redo(document, error), "redo property command: " + error)) {
        return 1;
    }

    if (!require(
            history.execute(
                std::make_unique<ReparentNodeCommand>("node_002", ""),
                document, error),
            "execute reparent command: " + error)) {
        return 1;
    }
    if (!require(document.findNode("node_002")->parentId.empty(),
                 "reparent command result")) {
        return 1;
    }
    if (!require(history.undo(document, error), "undo reparent command: " + error)) {
        return 1;
    }
    if (!require(document.findNode("node_002")->parentId == "node_001",
                 "reparent undo result")) {
        return 1;
    }

    error.clear();
    if (!require(!document.reparentNode("node_001", "node_002", error),
                 "reject cyclic reparent")) {
        return 1;
    }

    if (!require(document.saveToFile(outputPath, error),
                 "save scene: " + error)) {
        return 1;
    }
    SceneDocument reloaded;
    if (!require(SceneDocument::loadFromFile(outputPath, reloaded, error),
                 "reload saved scene: " + error)) {
        return 1;
    }
    if (!require(reloaded.findNode("node_002")->properties.at("text") == "Updated",
                 "saved property value")) {
        return 1;
    }

    fs::remove_all(temporaryDirectory, filesystemError);
    std::cout << "SceneDocumentTests passed\n";
    return 0;
}
