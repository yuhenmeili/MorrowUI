#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "EditorInputRouter.h"
#include "assets/AssetDatabase.h"
#include "assets/ImportQueue.h"
#include "assets/MaterialAsset.h"
#include "ProjectSettings.h"
#include "ui/DockLayout.h"
#include "commands/CommandHistory.h"
#include "scene/EditorSession.h"
#include "scene/NodeTypeCatalog.h"
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

    const auto assetRoot = temporaryDirectory / "assets";
    fs::create_directories(assetRoot / "textures", filesystemError);
    const auto assetPath = assetRoot / "textures" / "button.png";
    std::ofstream(assetPath).put('\0');
    {
        std::ofstream import(assetPath.string() + ".import");
        import << "[import]\n"
               << "format = 1\n"
               << "asset_id = \"asset_test_texture\"\n"
               << "importer = \"morrow.texture\"\n"
               << "source_hash = \"test\"\n"
               << "importer_version = 1\n\n"
               << "[options]\n"
               << "srgb = true\n\n"
               << "[platform.windows]\n"
               << "artifact = \".morrow/imported/asset_test_texture/texture.bin\"\n";
    }
    morrow::editor::AssetDatabase assets;
    if (!require(assets.scan(temporaryDirectory, "assets", error),
                 "asset scan: " + error)) {
        return 1;
    }
    if (!require(assets.findById("asset_test_texture") != nullptr,
                 "asset id lookup")) {
        return 1;
    }
    if (!require(assets.findById("asset_test_texture")->type == "Texture",
                 "asset type detection")) {
        return 1;
    }
    if (!require(assets.validateSourcePath("assets/textures/button.png", error),
                 "valid asset path: " + error)) {
        return 1;
    }
    if (!require(!assets.validateSourcePath("../outside.png", error),
                 "reject asset path outside project")) {
        return 1;
    }
    if (!require(assets.validateAssetReference("asset_test_texture", error),
                 "valid asset reference: " + error)) {
        return 1;
    }
    if (!require(!assets.validateAssetReference("missing_asset", error),
                 "reject missing asset reference")) {
        return 1;
    }
    const auto materialPath = temporaryDirectory / "sample.mat";
    {
        std::ofstream material(materialPath);
        material << "[material]\n"
                    "format = 1\n"
                    "shader = \"asset_test_shader\"\n\n"
                    "[properties]\n"
                    "color = Color(1.0, 0.5, 0.25, 1.0)\n"
                    "alpha = 0.75\n";
    }
    morrow::editor::MaterialAsset parsedMaterial;
    if (!require(morrow::editor::loadMaterialAsset(materialPath, parsedMaterial, error),
                 "load material asset: " + error)) {
        return 1;
    }
    if (!require(parsedMaterial.shader == "asset_test_shader" &&
                     parsedMaterial.properties.at("alpha") == "0.75",
                 "material asset properties")) {
        return 1;
    }

    const auto projectFile = temporaryDirectory / "MorrowUI.morrow";
    {
        std::ofstream project(projectFile);
        project << "[morrow_project format=1]\n"
                << "property name = \"Test\"\n"
                << "property asset_root = \"assets\"\n"
                << "property build_root = \"build\"\n"
                << "property preview_target = \"Preview\"\n"
                << "property platform = \"windows\"\n";
    }
    morrow::editor::ProjectSettings settings;
    if (!require(settings.load(projectFile, error), "project settings load: " + error)) return 1;
    if (!require(settings.value("preview_target") == "Preview", "project target")) return 1;
    if (!require(settings.pathValue("build_root") == temporaryDirectory / "build", "project build path")) return 1;
    if (!require(assets.findById("asset_test_texture")->needsImport,
                 "changed source hash requires import")) {
        return 1;
    }
    morrow::editor::ImportQueue importQueue;
    std::vector<morrow::editor::ImportTaskResult> importResults;
    if (!require(importQueue.importAll(assets, temporaryDirectory, "windows", importResults, error),
                 "incremental import: " + error)) {
        return 1;
    }
    if (!require(importResults.size() == 1 && importResults[0].success && importResults[0].attempts == 1,
                 "import result")) {
        return 1;
    }
    morrow::editor::AssetDatabase rescannedAssets;
    if (!require(rescannedAssets.scan(temporaryDirectory, "assets", error),
                 "rescan imported asset: " + error)) {
        return 1;
    }
    importResults.clear();
    if (!require(importQueue.importAll(rescannedAssets, temporaryDirectory, "windows", importResults, error),
                 "skip unchanged import: " + error)) {
        return 1;
    }
    if (!require(importResults[0].skipped, "unchanged asset is skipped")) return 1;

    auto dock = morrow::editor::DockLayout::defaultLayout(1280.0f, 720.0f);
    const auto dockPath = temporaryDirectory / ".morrow" / "editor.layout";
    if (!require(dock.save(dockPath, error), "save dock layout: " + error)) return 1;
    morrow::editor::DockLayout loadedDock;
    if (!require(loadedDock.load(dockPath, error), "load dock layout: " + error)) return 1;
    if (!require(
            loadedDock.findSplit("left") != nullptr &&
                loadedDock.findSplit("center") != nullptr &&
                loadedDock.findSplit("workspace") != nullptr &&
                loadedDock.findTabs("center_dock") != nullptr &&
                loadedDock.findTabs("center_dock")->active == "viewport",
            "dock layout persistence")) {
        return 1;
    }

    const auto sessionScene = temporaryDirectory / "session.scene";
    {
        std::ofstream output(sessionScene);
        output << "[morrow_scene format=1]\n\n"
               << "[node id=\"root\" type=\"SceneNode\" name=\"Root\"]\n\n"
               << "[node id=\"button\" type=\"MRButton\" parent=\"root\" "
                  "name=\"Button\"]\n"
               << "property text = \"Button\"\n"
               << "property position = Vector3(10.0, 20.0, 0.0)\n"
               << "property size = Vector2(100.0, 40.0)\n";
    }
    morrow::editor::EditorSession session(sessionScene);
    if (!require(session.load(error), "session load: " + error)) return 1;
    if (!require(session.model().buildSceneTree().size() == 2,
                 "scene tree item count")) {
        return 1;
    }
    if (!require(session.selectNode("button", false, error),
                 "scene tree selection: " + error)) {
        return 1;
    }
    if (!require(session.selectNode("root", true, error),
                 "multi selection: " + error)) {
        return 1;
    }
    if (!require(session.model().inspectSelected().size() >= 3,
                 "inspector property count")) {
        return 1;
    }
    if (!require(session.model().inspectSelected().front().mixed,
                 "multi selection reports mixed property")) {
        return 1;
    }
    if (!require(session.selectAt(20.0f, 30.0f, error),
                 "2D viewport selection: " + error)) {
        return 1;
    }
    if (!require(session.moveGizmo("button", 30.0f, 40.0f, 0.0f,
                                   true, error),
                 "gizmo move: " + error)) {
        return 1;
    }
    if (!require(session.moveGizmo("button", 40.0f, 50.0f, 0.0f,
                                   true, error),
                 "gizmo move merge: " + error)) {
        return 1;
    }
    if (!require(session.undo(error), "merged gizmo undo: " + error)) {
        return 1;
    }
    if (!require(session.document().findNode("button")->properties.at("position") ==
                     "Vector3(10.0, 20.0, 0.0)",
                 "merged gizmo restores original position")) {
        return 1;
    }
    if (!require(session.duplicateNode("button", "button_copy", error),
                 "duplicate node: " + error)) {
        return 1;
    }
    if (!require(session.document().findNode("button_copy") != nullptr,
                 "duplicate exists")) {
        return 1;
    }
    if (!require(session.deleteNode("button_copy", error),
                 "delete node: " + error)) {
        return 1;
    }
    if (!require(session.document().findNode("button_copy") == nullptr,
                 "deleted node absent")) {
        return 1;
    }
    if (!require(session.addNode({
                     "button_added", "MRButton", "root", "Added", {},
                     0
                 }, error),
                 "add node: " + error)) {
        return 1;
    }
    morrow::editor::NodeTypeCatalog nodeTypes;
    const auto filteredTypes = nodeTypes.filter("button");
    const bool foundButtonType = std::any_of(
        filteredTypes.begin(), filteredTypes.end(),
        [](const morrow::editor::NodeTypeDescriptor* descriptor) {
            return descriptor && descriptor->type == "MRButton";
        });
    if (!require(foundButtonType &&
                     nodeTypes.types().size() > 40,
                 "node type catalog search")) {
        return 1;
    }
    const auto* buttonType = nodeTypes.find("MRButton");
    if (!require(buttonType != nullptr, "button node type exists"))
        return 1;
    auto catalogButton = nodeTypes.createNode(
        *buttonType, "root", session.document());
    const auto catalogButtonId = catalogButton.id;
    if (!require(
            catalogButton.parentId == "root" &&
                catalogButton.properties.at("text") == "Button" &&
                catalogButton.properties.at("size") ==
                    "Vector2(180.0, 48.0)",
            "button node default properties")) {
        return 1;
    }
    if (!require(session.addNode(catalogButton, error),
                 "add catalog button: " + error)) {
        return 1;
    }
    if (!require(session.document().findNode(catalogButtonId) != nullptr,
                 "catalog button exists after add")) {
        return 1;
    }
    if (!require(session.undo(error),
                 "undo catalog button creation: " + error)) {
        return 1;
    }
    if (!require(session.document().findNode(catalogButtonId) == nullptr,
                 "catalog button removed by undo")) {
        return 1;
    }
    if (!require(session.redo(error),
                 "redo catalog button creation: " + error)) {
        return 1;
    }
    if (!require(session.document().findNode(catalogButtonId) != nullptr,
                 "catalog button restored by redo")) {
        return 1;
    }
    if (!require(session.renameNode(
                     catalogButtonId, "RenamedButton", error),
                 "rename node: " + error)) {
        return 1;
    }
    if (!require(
            session.document().findNode(catalogButtonId)->name ==
                "RenamedButton",
            "renamed node name")) {
        return 1;
    }
    if (!require(session.undo(error), "undo rename: " + error))
        return 1;
    if (!require(
            session.document().findNode(catalogButtonId)->name ==
                catalogButton.name,
            "rename undo restores old name")) {
        return 1;
    }
    if (!require(session.redo(error), "redo rename: " + error))
        return 1;

    if (!require(
            session.setNodeRect(
                "button", 20.0f, 30.0f, 0.0f,
                140.0f, 60.0f, true, error),
            "continuous rect edit: " + error)) {
        return 1;
    }
    if (!require(
            session.setNodeRect(
                "button", 30.0f, 40.0f, 0.0f,
                160.0f, 70.0f, true, error),
            "merged continuous rect edit: " + error)) {
        return 1;
    }
    if (!require(session.undo(error), "undo merged rect edit: " + error))
        return 1;
    if (!require(
            session.document().findNode("button")->properties.at("position") ==
                    "Vector3(10.0, 20.0, 0.0)" &&
                session.document().findNode("button")->properties.at("size") ==
                    "Vector2(100.0, 40.0)",
            "merged rect edit restores original transform")) {
        return 1;
    }

    const auto* imageType = nodeTypes.find("MRImage");
    if (!require(
            imageType &&
                imageType->defaultProperties.count("texture_asset") == 1,
            "image type has texture resource property")) {
        return 1;
    }
    auto imageNode = nodeTypes.createNode(
        *imageType, "root", session.document());
    const auto imageId = imageNode.id;
    if (!require(session.addNode(imageNode, error),
                 "add image node: " + error)) {
        return 1;
    }
    if (!require(session.selectNode(imageId, false, error),
                 "select image node: " + error)) {
        return 1;
    }
    const auto imageProperties = session.model().inspectSelected();
    const auto hasProperty =
        [&imageProperties](const std::string& name) {
            return std::any_of(
                imageProperties.begin(), imageProperties.end(),
                [&name](const morrow::editor::InspectorProperty& property) {
                    return property.name == name;
                });
        };
    const auto hasComponent =
        [&imageProperties](const std::string& component) {
            return std::any_of(
                imageProperties.begin(), imageProperties.end(),
                [&component](const morrow::editor::InspectorProperty& property) {
                    return property.component == component;
                });
        };
    if (!require(
            hasProperty("position") && hasProperty("rotation") &&
                hasProperty("scale") && hasProperty("size") &&
                hasProperty("visible") &&
                hasProperty("texture_asset") && hasProperty("mesh") &&
                hasProperty("renderer_enabled") &&
                hasProperty("material") && hasProperty("material_color") &&
                hasComponent("Transform") &&
                hasComponent("Mesh Filter") &&
                hasComponent("Mesh Renderer"),
            "image inspector exposes component panels and properties")) {
        return 1;
    }
    if (!require(
            session.addNode(
                {"nested_parent", "SceneNode", "root", "NestedParent",
                 {{"position", "Vector3(100.0, 50.0, 0.0)"},
                  {"size", "Vector2(300.0, 200.0)"}},
                 0},
                error),
            "add nested parent: " + error)) {
        return 1;
    }
    if (!require(
            session.addNode(
                {"nested_child", "MRImage", "nested_parent", "NestedChild",
                 {{"position", "Vector3(10.0, 20.0, 0.0)"},
                  {"size", "Vector2(40.0, 30.0)"}},
                 0},
                error),
            "add nested child: " + error)) {
        return 1;
    }
    float worldX = 0.0f;
    float worldY = 0.0f;
    float worldZ = 0.0f;
    float worldWidth = 0.0f;
    float worldHeight = 0.0f;
    if (!require(
            session.model().nodeWorldRect(
                "nested_child", worldX, worldY, worldZ,
                worldWidth, worldHeight) &&
                worldX == 110.0f && worldY == 70.0f &&
                worldWidth == 40.0f && worldHeight == 30.0f,
            "nested node world rectangle")) {
        return 1;
    }
    if (!require(session.reparentNode("button_added", "", error),
                 "reparent node: " + error)) {
        return 1;
    }
    morrow::editor::EditorInputRouter input(session);
    if (!require(input.handleShortcut(true, false, 's', error),
                 "Ctrl+S: " + error)) {
        return 1;
    }
    if (!require(!session.isDirty(), "Ctrl+S clears dirty state")) return 1;

    fs::remove_all(temporaryDirectory, filesystemError);
    std::cout << "SceneDocumentTests passed\n";
    return 0;
}
