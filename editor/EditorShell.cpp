#include "EditorShell.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <utility>

#include "Engine.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "base/MeshFilter.h"
#include "base/MeshRenderer.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "base/Mesh.h"
#include "renderer/resource/ssbo/layouts/ButtonSSBOLayout.h"
#include "platform/Window.h"
#include "scene/SceneInstantiator.h"
#include "wgl/OpenglHeader.h"

namespace {
std::unordered_map<GLFWwindow*, morrow::editor::EditorShell*>& shells() {
    static std::unordered_map<GLFWwindow*, morrow::editor::EditorShell*> value;
    return value;
}

std::unordered_map<GLFWwindow*, GLFWkeyfun>& previousKeyCallbacks() {
    static std::unordered_map<GLFWwindow*, GLFWkeyfun> callbacks;
    return callbacks;
}

std::unordered_map<GLFWwindow*, GLFWcharfun>& previousCharCallbacks() {
    static std::unordered_map<GLFWwindow*, GLFWcharfun> callbacks;
    return callbacks;
}

std::shared_ptr<morrow::UIWidget> makePanel(float x, float y, float width, float height,
                                            const morrow::Math::Vector4& color = morrow::Math::Vector4(0.12f, 0.14f, 0.17f, 1.0f)) {
    auto panel = morrow::MRButton::create();
    panel->setInteractive(false);
    panel->setBackgroundColor(color);
    panel->setCornerRadius(0.0f);
    panel->setClipChildren(true);
    auto transform = panel->getTransform();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    return panel;
}

class EditorGuideWidget final : public morrow::UIWidget {
public:
    explicit EditorGuideWidget(const morrow::Math::Vector4& color) : morrow::UIWidget(true) {
        // Reuse the button shader because it already has a compatible SSBO
        // layout in the renderer's batch path.
        auto material = getComponent<morrow::MeshRenderer>()->getMaterial();
        material->setShader("button");
        material->setSSBOLayout(std::make_shared<morrow::ButtonSSBOLayout>());
        material->setFloat("rounding", 0.0f);
        material->setFloat("useTexture", 0.0f);
        material->setVector("color", color);
        material->setFloat("alpha", color.w);
    }

    void setColor(const morrow::Math::Vector4& color) {
        auto material = getComponent<morrow::MeshRenderer>()->getMaterial();
        material->setVector("color", color);
        material->setFloat("alpha", color.w);
    }

    void setGeometry(std::vector<morrow::Math::Vector3> vertices, std::vector<int16_t> indices, float x, float y,
                     float width = 0.0f, float height = 0.0f) {
        auto mesh = getComponent<morrow::MeshFilter>()->getMesh();
        mesh->setVertices(vertices);
        mesh->setIndices(indices);
        if (width > 0.0f && height > 0.0f)
            getTransform()->setSize(width, height);
        if (width > 0.0f && height > 0.0f)
            getComponent<morrow::MeshRenderer>()->getMaterial()->setVector("displaySize", morrow::Math::Vector3(width, height, 0.0f));
        getTransform()->setPosition(x, y, 0.0f);
    }
};

void addQuad(std::vector<morrow::Math::Vector3>& vertices, std::vector<int16_t>& indices,
             float left, float top, float right, float bottom) {
    const auto start = static_cast<int16_t>(vertices.size());
    vertices.emplace_back(left, top, 0.0f);
    vertices.emplace_back(right, top, 0.0f);
    vertices.emplace_back(right, bottom, 0.0f);
    vertices.emplace_back(left, bottom, 0.0f);
    indices.insert(indices.end(), {start, static_cast<int16_t>(start + 1), static_cast<int16_t>(start + 2),
                                   static_cast<int16_t>(start + 2), static_cast<int16_t>(start + 3), start});
}

std::shared_ptr<EditorGuideWidget> makeGuide(const morrow::Math::Vector4& color) {
    return std::make_shared<EditorGuideWidget>(color);
}

}  // namespace

namespace morrow::editor {

EditorShell::EditorShell(const std::shared_ptr<Window>& window, const std::shared_ptr<Engine>& engine, std::filesystem::path projectPath, std::filesystem::path scenePath,
                         std::filesystem::path assetRoot) :
    m_window(window), m_engine(engine), m_projectPath(std::move(projectPath)), m_scenePath(std::move(scenePath)), m_assetRoot(std::move(assetRoot)),
    m_session(std::make_shared<EditorSession>(m_scenePath)), m_dockLayoutPath(m_projectPath.parent_path() / ".morrow" / "editor.layout") {
    m_selectionChangedConnection = m_events.onSelectionChanged.connect(
        [this](const SelectionState& selection) {
            m_selectedNodeId =
                selection.nodeIds.empty() ? std::string{} : selection.nodeIds.back();
            rebuildRuntime();
            refreshInspector();
        });
}

EditorShell::~EditorShell() {
    if (m_buildFuture.valid())
        m_buildFuture.wait();
    m_inputConnection.disconnect();
    if (m_window) {
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getSurface());
        if (glfwWindow) {
            if (const auto iterator = previousKeyCallbacks().find(glfwWindow);
                iterator != previousKeyCallbacks().end()) {
                glfwSetKeyCallback(glfwWindow, iterator->second);
                previousKeyCallbacks().erase(iterator);
            }
            if (const auto iterator = previousCharCallbacks().find(glfwWindow);
                iterator != previousCharCallbacks().end()) {
                glfwSetCharCallback(glfwWindow, iterator->second);
                previousCharCallbacks().erase(iterator);
            }
            shells().erase(glfwWindow);
        }
    }
}

EditorEvents& EditorShell::events() {
    return m_events;
}

void EditorShell::setStatus(const std::string& text) {
    m_status = text;
    m_outputLines.push_back(text);
    if (m_outputLines.size() > 6)
        m_outputLines.erase(m_outputLines.begin());
    refreshOutput();
}

void EditorShell::setPreviewState(PreviewState state) {
    m_previewState = state;
    const char* label = "Stopped";
    switch (state) {
        case PreviewState::Starting: label = "Starting"; break;
        case PreviewState::Running: label = "Running"; break;
        case PreviewState::Outdated: label = "Outdated"; break;
        case PreviewState::Failed: label = "Failed"; break;
        case PreviewState::Stopped: break;
    }
    setStatus(std::string("Preview: ") + label);
}

void EditorShell::stopPreview() {
    if (m_buildFuture.valid() && m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        m_buildQueue.cancel();
        setStatus("Preview stop requested");
    } else {
        setPreviewState(PreviewState::Stopped);
    }
}

void EditorShell::runImportQueue() {
    std::vector<ImportTaskResult> results;
    std::string error;
    const bool success = m_importQueue.importAll(m_assets, m_projectPath.parent_path(), "windows", results, error);
    size_t imported = 0;
    size_t skipped = 0;
    size_t failed = 0;
    for (const auto& result : results) {
        if (!result.success)
            ++failed;
        else if (result.skipped)
            ++skipped;
        else
            ++imported;
    }
    setStatus("Assets: imported=" + std::to_string(imported) + " unchanged=" + std::to_string(skipped) + " failed=" + std::to_string(failed));
    if (!success && !error.empty())
        setStatus("Import failed: " + error);
    std::string scanError;
    if (!m_assets.scan(
            m_projectPath.parent_path(), m_assetRoot, scanError)) {
        setStatus("Asset rescan failed: " + scanError);
    }
    if (!m_fileSystem.projectRoot().empty()) {
        if (!m_fileSystem.refresh(scanError)) {
            setStatus("FileSystem refresh failed: " + scanError);
        } else if (m_fileSystemPanel) {
            m_fileSystemPanel->refreshView();
        }
    }
    if (!results.empty()) {
        m_events.onAssetDatabaseChanged.notify(m_assets);
    }
}

void EditorShell::notifySelectionChanged() {
    const auto& selection = m_session->model().selection();
    if (selection.nodeIds == m_lastNotifiedSelection) {
        return;
    }
    m_lastNotifiedSelection = selection.nodeIds;
    m_events.onSelectionChanged.notify(selection);
}

void EditorShell::addLabel(const std::shared_ptr<UIWidget>& parent, const std::string& text, float x, float y, float width, float height) {
    auto label = std::make_shared<MRLabel>();
    label->setText(std::wstring(text.begin(), text.end()), "default");
    label->setFontSize(16.0f);
    label->setFontColor(1.0f, 1.0f, 1.0f, 1.0f);
    auto transform = label->getComponent<Transform>();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    parent->addChild(label);
}

std::shared_ptr<MRButton> EditorShell::addButton(const std::shared_ptr<UIWidget>& parent, const std::wstring& text, float x, float y, float width, float height,
                                                 std::function<void()> callback) {
    auto button = MRButton::create();
    button->setText(text, "default");
    button->setTextFontSize(16.0f);
    button->setAutoWrap(false);
    button->setBackgroundColor(morrow::Math::Vector4(0.16f, 0.31f, 0.63f, 1.0f));
    button->setHoverColor(morrow::Math::Vector4(0.30f, 0.55f, 0.95f, 1.0f));
    button->setPressedColor(morrow::Math::Vector4(0.10f, 0.22f, 0.48f, 1.0f));
    m_buttonConnections.erase(
        std::remove_if(
            m_buttonConnections.begin(),
            m_buttonConnections.end(),
            [](const Observable<BaseButton&>::Connection& connection) {
                return !connection.connected();
            }),
        m_buttonConnections.end());
    m_buttonConnections.emplace_back(
        button->events().onClicked.connect(
            [callback = std::move(callback)](BaseButton&) {
                if (callback) {
                    callback();
                }
            }));
    auto transform = button->getComponent<Transform>();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    parent->addChild(button);
    return button;
}

void EditorShell::buildLayout() {
    m_shellRoot = std::make_shared<UIWidget>(false);
    m_shellRoot->setWidgetName("EditorShell");
    m_shellRoot->getTransform()->setPosition(0.0f, 0.0f, 100.0f);
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    if (m_window && m_window->getSurface()) {
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_window->getSurface()), &framebufferWidth, &framebufferHeight);
        if (framebufferWidth > 0 && framebufferHeight > 0) {
            windowWidth = static_cast<float>(framebufferWidth);
            windowHeight = static_cast<float>(framebufferHeight);
        }
    }
    m_shellRoot->getTransform()->setSize(windowWidth, windowHeight);

    const DockLayout defaultLayout = DockLayout::defaultLayout(windowWidth, windowHeight);
    m_dockLayout = defaultLayout;
    std::string layoutError;
    if (std::filesystem::exists(m_dockLayoutPath)) {
        DockLayout loadedLayout;
        if (loadedLayout.load(m_dockLayoutPath, layoutError)) {
            for (const auto& defaultSplit : defaultLayout.splits()) {
                if (!loadedLayout.findSplit(defaultSplit.id))
                    loadedLayout.splits().push_back(defaultSplit);
            }
            m_dockLayout = std::move(loadedLayout);
        }
    }

    m_toolbarPanel = makePanel(0.0f, 0.0f, windowWidth, 40.0f);
    m_sceneTreePanel = makePanel(0.0f, 0.0f, 240.0f, 570.0f, morrow::Math::Vector4(0.13f, 0.15f, 0.19f, 1.0f));
    m_viewportPanel = makePanel(0.0f, 0.0f, 740.0f, 570.0f, morrow::Math::Vector4(0.10f, 0.12f, 0.15f, 1.0f));
    m_inspectorPanel = makePanel(0.0f, 0.0f, 300.0f, 570.0f, morrow::Math::Vector4(0.13f, 0.15f, 0.19f, 1.0f));
    m_statusPanel = makePanel(0.0f, 0.0f, 1280.0f, 140.0f, morrow::Math::Vector4(0.11f, 0.13f, 0.16f, 1.0f));
    m_fileSystemPanel = FileSystemPanel::create(
        m_fileSystem,
        [this](const std::string& status) { setStatus(status); });
    m_previewRoot = makePanel(0.0f, 32.0f, 740.0f, 538.0f);
    m_previewRoot->setWidgetName("PreviewRoot");

    m_workspaceSplit = MRSplitContainer::create();
    m_workspaceSplit->setOrientation(SplitOrientation::Vertical);
    m_workspaceSplit->setFirstMinSize(260.0f);
    m_workspaceSplit->setSecondMinSize(120.0f);
    m_workspaceSplit->setHandleWidth(5.0f);
    m_workspaceSplit->getTransform()->setPosition(0.0f, 40.0f, 0.0f);
    m_workspaceSplit->getTransform()->setSize(windowWidth, std::max(1.0f, windowHeight - 40.0f));

    m_mainSplit = MRSplitContainer::create();
    m_mainSplit->setOrientation(SplitOrientation::Horizontal);
    m_mainSplit->setFirstMinSize(220.0f);
    m_mainSplit->setSecondMinSize(520.0f);
    m_mainSplit->setHandleWidth(5.0f);

    m_leftSplit = MRSplitContainer::create();
    m_leftSplit->setOrientation(SplitOrientation::Vertical);
    m_leftSplit->setFirstMinSize(140.0f);
    m_leftSplit->setSecondMinSize(180.0f);
    m_leftSplit->setHandleWidth(5.0f);

    m_centerSplit = MRSplitContainer::create();
    m_centerSplit->setOrientation(SplitOrientation::Horizontal);
    m_centerSplit->setFirstMinSize(320.0f);
    m_centerSplit->setSecondMinSize(260.0f);
    m_centerSplit->setHandleWidth(5.0f);

    m_centerSplit->setFirst(m_viewportPanel);
    m_centerSplit->setSecond(m_inspectorPanel);
    m_leftSplit->setFirst(m_sceneTreePanel);
    m_leftSplit->setSecond(m_fileSystemPanel);
    m_mainSplit->setFirst(m_leftSplit);
    m_mainSplit->setSecond(m_centerSplit);
    m_workspaceSplit->setFirst(m_mainSplit);
    m_workspaceSplit->setSecond(m_statusPanel);

    m_shellRoot->addChild(m_toolbarPanel);
    m_shellRoot->addChild(m_workspaceSplit);
    m_viewportPanel->addChild(m_previewRoot);
    m_window->addChild(m_shellRoot);

    const auto connectSplit = [this](
                                  const std::string& id,
                                  const std::shared_ptr<MRSplitContainer>& split) {
        m_splitConnections.emplace_back(
            split->events().onSplitRatioChanged.connect(
                [this, id](MRSplitContainer&, float ratio) {
                    if (auto* state = m_dockLayout.findSplit(id))
                        state->ratio = ratio;
                    applyDockLayout();
                }));
        m_splitConnections.emplace_back(
            split->events().onDragFinished.connect(
                [this, id](MRSplitContainer&, float ratio) {
                    if (auto* state = m_dockLayout.findSplit(id))
                        state->ratio = ratio;
                    saveDockLayout();
                }));
    };
    connectSplit("workspace", m_workspaceSplit);
    connectSplit("left", m_mainSplit);
    connectSplit("center", m_centerSplit);
    connectSplit("left_stack", m_leftSplit);

    addLabel(m_toolbarPanel, "MorrowEditor", 8.0f, 5.0f, 150.0f, 28.0f);
    addButton(m_toolbarPanel, L"Save", 170.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->save(error))
            setStatus("Saved");
        else
            setStatus(error);
    });
    addButton(m_toolbarPanel, L"Undo", 248.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->undo(error)) {
            rebuildRuntime();
            refreshSceneTree();
            refreshInspector();
        }
        setStatus(error.empty() ? "Undo" : error);
    });
    addButton(m_toolbarPanel, L"Redo", 326.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->redo(error)) {
            rebuildRuntime();
            refreshSceneTree();
            refreshInspector();
        }
        setStatus(error.empty() ? "Redo" : error);
    });
    addButton(m_toolbarPanel, L"Configure", 420.0f, 4.0f, 100.0f, 30.0f, [this] { runBuild(BuildTaskKind::Configure); });
    addButton(m_toolbarPanel, L"Build", 526.0f, 4.0f, 80.0f, 30.0f, [this] { runBuild(BuildTaskKind::Build); });
    addButton(m_toolbarPanel, L"Build & Run", 612.0f, 4.0f, 112.0f, 30.0f, [this] { runBuild(BuildTaskKind::BuildAndRun); });
    addButton(m_toolbarPanel, L"Run Last", 730.0f, 4.0f, 92.0f, 30.0f, [this] { runBuild(BuildTaskKind::Run); });
    addButton(m_toolbarPanel, L"Stop", 828.0f, 4.0f, 72.0f, 30.0f, [this] { stopPreview(); });
    addButton(m_toolbarPanel, L"Import", 906.0f, 4.0f, 82.0f, 30.0f, [this] { runImportQueue(); });
    addButton(m_toolbarPanel, L"Assets", 994.0f, 4.0f, 82.0f, 30.0f, [this] { showAssetBrowser(); });
    addLabel(m_sceneTreePanel, "Scene", 8.0f, 6.0f, 220.0f, 28.0f);
    addLabel(m_viewportPanel, "2D Viewport", 8.0f, 6.0f, 220.0f, 28.0f);
    addLabel(m_inspectorPanel, "Inspector", 8.0f, 6.0f, 260.0f, 28.0f);
    applyDockLayout();
    refreshOutput();
}

void EditorShell::applyDockLayout() {
    if (const auto* split = m_dockLayout.findSplit("workspace"))
        m_workspaceSplit->setSplitRatio(split->ratio);
    if (const auto* split = m_dockLayout.findSplit("left"))
        m_mainSplit->setSplitRatio(split->ratio);
    if (const auto* split = m_dockLayout.findSplit("center"))
        m_centerSplit->setSplitRatio(split->ratio);
    if (const auto* split = m_dockLayout.findSplit("left_stack"))
        m_leftSplit->setSplitRatio(split->ratio);

    const Vector3 viewportSize = m_viewportPanel->getTransform()->getSize();
    m_viewportWidth = viewportSize.x;
    m_viewportHeight = viewportSize.y;
    m_previewRoot->getTransform()->setSize(
        viewportSize.x, std::max(1.0f, viewportSize.y - 32.0f));
    if (m_previewCanvas || m_previewGrid)
        refreshViewportGuides();
}

void EditorShell::saveDockLayout() {
    std::string error;
    if (!m_dockLayout.save(m_dockLayoutPath, error))
        setStatus(error);
}

void EditorShell::handleFramebufferResize(const Vector2& size) {
    if (!m_shellRoot || size.x <= 0.0f || size.y <= 0.0f)
        return;
    m_shellRoot->getTransform()->setSize(size.x, size.y);
    m_toolbarPanel->getTransform()->setSize(size.x, 40.0f);
    m_workspaceSplit->getTransform()->setPosition(0.0f, 40.0f, 0.0f);
    m_workspaceSplit->getTransform()->setSize(
        size.x, std::max(1.0f, size.y - 40.0f));
    applyDockLayout();
    refreshOutput();
}

void EditorShell::refreshOutput() {
    if (!m_statusPanel)
        return;
    m_statusPanel->m_children.clear();
    addLabel(m_statusPanel, "Output / Assets / Build", 8.0f, 2.0f, 360.0f, 22.0f);
    float y = 26.0f;
    const float contentWidth =
        std::max(1.0f, m_statusPanel->getTransform()->getSize().x - 16.0f);
    for (const auto& line : m_outputLines) {
        addLabel(m_statusPanel, line, 8.0f, y, contentWidth, 20.0f);
        y += 21.0f;
    }
}

void EditorShell::runBuild(BuildTaskKind kind) {
    if (m_buildFuture.valid() && m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        setStatus("A build process is already running");
        return;
    }
    const auto projectRoot = m_project.projectRoot();
    const auto buildRoot = m_project.pathValue("build_root", "build");
    const auto target = m_project.value("preview_target", "MorrowEditor");
    const auto configuredExecutable = buildRoot / (target + ".exe");
    if (m_lastSuccessfulExecutable.empty()) {
        std::error_code searchError;
        if (std::filesystem::exists(configuredExecutable))
            m_lastSuccessfulExecutable = configuredExecutable;
        else if (std::filesystem::exists(buildRoot)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, searchError)) {
                if (!searchError && entry.is_regular_file() && entry.path().filename() == target + ".exe") {
                    m_lastSuccessfulExecutable = entry.path();
                    break;
                }
            }
        }
    }
    if (kind == BuildTaskKind::Run && !m_lastSuccessfulExecutable.empty() && std::filesystem::exists(m_lastSuccessfulExecutable))
        setStatus("Run last successful started");
    else if (kind == BuildTaskKind::Run)
        setStatus("No successful preview executable; using configured target");
    else
        setStatus(kind == BuildTaskKind::Configure ? "Configure started" : (kind == BuildTaskKind::Build ? "Build started" : "Build & Run started"));
    m_pendingBuildKind = kind;
    if (kind == BuildTaskKind::Run || kind == BuildTaskKind::BuildAndRun)
        setPreviewState(PreviewState::Starting);
    m_buildFuture = std::async(std::launch::async, [this, kind, projectRoot, buildRoot, target] {
        if (kind == BuildTaskKind::Configure)
            return m_buildQueue.configure(projectRoot, buildRoot);
        if (kind == BuildTaskKind::Build)
            return m_buildQueue.build(projectRoot, buildRoot, target);
        if (kind == BuildTaskKind::BuildAndRun)
            return m_buildQueue.buildAndRun(projectRoot, buildRoot, target);
        auto executable = m_lastSuccessfulExecutable.empty() ? buildRoot / (target + ".exe") : m_lastSuccessfulExecutable;
        return m_buildQueue.runTarget(executable, executable.parent_path());
    });
}

void EditorShell::pollBuild() {
    bool outputChanged = false;
    for (const auto& chunk : m_buildQueue.drainOutput()) {
        auto& remainder = chunk.stderrStream ? m_stderrRemainder : m_stdoutRemainder;
        remainder += chunk.text;
        size_t newline = 0;
        while ((newline = remainder.find('\n')) != std::string::npos) {
            auto line = remainder.substr(0, newline);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (!line.empty())
                m_outputLines.push_back(std::string(chunk.stderrStream ? "[stderr] " : "[stdout] ") + line);
            remainder.erase(0, newline + 1);
            outputChanged = true;
        }
    }
    while (m_outputLines.size() > 6) {
        m_outputLines.erase(m_outputLines.begin());
        outputChanged = true;
    }
    if (outputChanged)
        refreshOutput();

    if (!m_buildFuture.valid())
        return;
    if (m_buildQueue.state() == BuildProcessState::Running &&
        (m_pendingBuildKind == BuildTaskKind::Run || m_pendingBuildKind == BuildTaskKind::BuildAndRun) &&
        m_previewState == PreviewState::Starting) {
        m_previewState = PreviewState::Running;
    }
    if (m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;
    const auto result = m_buildFuture.get();
    appendBuildResult(result);
    if (result.success && (m_pendingBuildKind == BuildTaskKind::Build || m_pendingBuildKind == BuildTaskKind::BuildAndRun)) {
        const auto buildRoot = m_project.pathValue("build_root", "build");
        const auto target = m_project.value("preview_target", "MorrowEditor");
        m_lastSuccessfulExecutable = buildRoot / (target + ".exe");
        if (!std::filesystem::exists(m_lastSuccessfulExecutable)) {
            std::error_code searchError;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, searchError)) {
                if (!searchError && entry.is_regular_file() && entry.path().filename() == target + ".exe") {
                    m_lastSuccessfulExecutable = entry.path();
                    break;
                }
            }
        }
        setPreviewState(m_pendingBuildKind == BuildTaskKind::BuildAndRun ? PreviewState::Stopped : PreviewState::Outdated);
    } else if (m_pendingBuildKind == BuildTaskKind::Run || m_pendingBuildKind == BuildTaskKind::BuildAndRun) {
        setPreviewState(result.cancelled ? PreviewState::Stopped : (result.success ? PreviewState::Stopped : PreviewState::Failed));
    }
    setStatus(result.cancelled ? "Process cancelled" : (result.success ? "Process succeeded, exit=" + std::to_string(result.exitCode) :
                                                                 "Process failed, exit=" + std::to_string(result.exitCode)));
}

void EditorShell::appendBuildResult(const BuildTaskResult& result) {
    std::istringstream stdoutStream(result.stdoutText);
    std::string line;
    while (std::getline(stdoutStream, line))
        if (!line.empty())
            m_outputLines.push_back("[stdout] " + line);
    std::istringstream stderrStream(result.stderrText);
    while (std::getline(stderrStream, line))
        if (!line.empty())
            m_outputLines.push_back("[stderr] " + line);
    for (const auto& diagnostic : result.diagnostics) {
        m_outputLines.push_back(std::string(diagnostic.error ? "[error] " : "[warning] ") +
                                diagnostic.file.string() + ":" + std::to_string(diagnostic.line) + ":" +
                                std::to_string(diagnostic.column) + " " + diagnostic.message);
    }
    while (m_outputLines.size() > 6)
        m_outputLines.erase(m_outputLines.begin());
    refreshOutput();
}

void EditorShell::showAssetBrowser() {
    std::string error;
    if (!m_fileSystem.refresh(error)) {
        setStatus("FileSystem refresh failed: " + error);
        return;
    }
    if (m_fileSystemPanel)
        m_fileSystemPanel->refreshView();
    setStatus(
        "FileSystem: " + std::to_string(m_fileSystem.entries().size()) +
        " entries, " + std::to_string(m_assets.assets().size()) +
        " assets");
}

void EditorShell::refreshSceneTree() {
    if (!m_sceneTreePanel)
        return;
    while (m_sceneTreePanel->m_children.size() > 1) {
        m_sceneTreePanel->m_children.pop_back();
    }
    float y = 38.0f;
    for (const auto& item : m_session->model().buildSceneTree()) {
        const auto id = item.id;
        auto button = addButton(m_sceneTreePanel, std::wstring(item.depth * 2, L' ') + std::wstring(item.name.begin(), item.name.end()), 8.0f + item.depth * 12.0f, y,
                                220.0f - item.depth * 12.0f, 30.0f, [this, id] {
                                    std::string error;
                                    if (m_session->selectNode(id, false, error)) {
                                        notifySelectionChanged();
                                        setStatus("Selected " + id);
                                    } else
                                        setStatus(error);
                                });
        button->setWidgetName("SceneTree_" + item.id);
        y += 32.0f;
    }
}

void EditorShell::refreshInspector() {
    if (!m_inspectorPanel)
        return;
    while (m_inspectorPanel->m_children.size() > 1) {
        m_inspectorPanel->m_children.pop_back();
    }
    float y = 38.0f;
    for (const auto& property : m_session->model().inspectSelected()) {
        addLabel(m_inspectorPanel, property.name + " [" + property.type + "]", 8.0f, y, 280.0f, 22.0f);
        y += 24.0f;
        if (property.type == "bool" && !property.mixed) {
            addButton(m_inspectorPanel, property.value == "true" ? L"[x] true" : L"[ ] false", 8.0f, y, 280.0f, 30.0f, [this, property] {
                m_editProperty = property.name;
                m_editValue = property.value == "true" ? "false" : "true";
                commitPropertyEdit();
            });
        } else if (property.type == "number" && !property.mixed) {
            addButton(m_inspectorPanel, L"-", 8.0f, y, 36.0f, 30.0f, [this, property] {
                try {
                    m_editProperty = property.name;
                    m_editValue = std::to_string(std::stof(property.value) - 1.0f);
                    commitPropertyEdit();
                } catch (...) { setStatus("Invalid numeric property"); }
            });
            addButton(m_inspectorPanel, std::wstring(property.value.begin(), property.value.end()), 48.0f, y, 196.0f, 30.0f,
                      [this, property] { beginPropertyEdit(property.name, property.value); });
            addButton(m_inspectorPanel, L"+", 248.0f, y, 40.0f, 30.0f, [this, property] {
                try {
                    m_editProperty = property.name;
                    m_editValue = std::to_string(std::stof(property.value) + 1.0f);
                    commitPropertyEdit();
                } catch (...) { setStatus("Invalid numeric property"); }
            });
        } else {
            std::wstring value(property.value.begin(), property.value.end());
            if (property.type == "Color")
                value = L"\u25a0 " + value;
            addButton(m_inspectorPanel, value, 8.0f, y, 280.0f, 30.0f, [this, property] {
                beginPropertyEdit(property.name, property.mixed ? std::string{} : property.value);
            });
        }
        y += 36.0f;
    }
}

void EditorShell::beginPropertyEdit(const std::string& property, const std::string& value) {
    m_editProperty = property;
    m_editValue = value;
    setStatus("Editing " + property + ": type a value and press Enter");
}

void EditorShell::commitPropertyEdit() {
    if (m_editProperty.empty())
        return;
    std::string error;
    bool changed = false;
    for (const auto& nodeId : m_session->model().selection().nodeIds) {
        if (!m_session->setProperty(nodeId, m_editProperty, m_editValue, false, error))
            break;
        changed = true;
    }
    if (changed) {
        rebuildRuntime();
        refreshInspector();
        setStatus("Applied " + m_editProperty + " = " + m_editValue);
    } else if (!error.empty()) {
        setStatus(error);
    }
    m_editProperty.clear();
    m_editValue.clear();
}

void EditorShell::handleChar(unsigned int codepoint) {
    if (m_editProperty.empty() || codepoint < 32 || codepoint > 126)
        return;
    m_editValue.push_back(static_cast<char>(codepoint));
    setStatus("Editing " + m_editProperty + ": " + m_editValue);
}

void EditorShell::rebuildRuntime() {
    if (!m_previewRoot)
        return;
    m_previewRoot->m_children.clear();
    refreshViewportGuides();
    std::string error;
    const bool runtimeInstantiated =
        SceneInstantiator::instantiate(m_session->document(), m_previewRoot, &m_assets, error);
    if (!runtimeInstantiated) {
        setStatus(error);
    }
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (m_session->model().selectedRect(x, y, width, height)) {
        auto frame = makeGuide(morrow::Math::Vector4(0.35f, 0.78f, 1.0f, 0.95f));
        std::vector<morrow::Math::Vector3> vertices;
        std::vector<int16_t> indices;
        constexpr float thickness = 2.0f;
        addQuad(vertices, indices, -width * 0.5f, -height * 0.5f, width * 0.5f, -height * 0.5f + thickness);
        addQuad(vertices, indices, -width * 0.5f, height * 0.5f, width * 0.5f, height * 0.5f - thickness);
        addQuad(vertices, indices, -width * 0.5f, height * 0.5f, -width * 0.5f + thickness, -height * 0.5f);
        addQuad(vertices, indices, width * 0.5f - thickness, height * 0.5f, width * 0.5f, -height * 0.5f);
        frame->setDisplayLayer(10);
        frame->setGeometry(std::move(vertices), std::move(indices), x, y, width, height);
        m_selectionFrame = frame;
        m_previewRoot->addChild(frame);
    } else {
        m_selectionFrame.reset();
    }
    if (runtimeInstantiated) {
        m_events.onRuntimeRebuilt.notify();
    }
}

void EditorShell::refreshViewportGuides() {
    if (!m_previewRoot)
        return;

    if (m_previewCanvas) {
        m_previewRoot->removeChild(m_previewCanvas);
        m_previewCanvas.reset();
    }
    if (m_previewGrid) {
        m_previewRoot->removeChild(m_previewGrid);
        m_previewGrid.reset();
    }
}

void EditorShell::handleViewportPointer(const TouchEvent& event) {
    const Math::Rect viewportBounds = m_viewportPanel->getScreenSpaceAABB();
    m_viewportX = viewportBounds.Min.x;
    m_viewportY = viewportBounds.Min.y;
    m_viewportWidth = viewportBounds.GetWidth();
    m_viewportHeight = viewportBounds.GetHeight();
    const float panelX = event.positionX - m_viewportX;
    const float panelY = event.positionY - m_viewportY;
    const float x = (panelX - m_viewPanX) / m_viewZoom;
    const float y = (panelY - 32.0f - m_viewPanY) / m_viewZoom;
    if (panelX < 0.0f || panelY < 32.0f || panelX > m_viewportWidth || panelY > m_viewportHeight)
        return;
    if (event.eventType == TOUCH_EVENT_TYPE_WHEEL) {
        const float oldZoom = m_viewZoom;
        m_viewZoom = std::clamp(m_viewZoom * (event.wheelDeltaY > 0.0f ? 1.1f : 0.9f), 0.2f, 5.0f);
        m_viewPanX = panelX - x * m_viewZoom;
        m_viewPanY = panelY - 32.0f - y * m_viewZoom;
        m_previewRoot->getTransform()->setPosition(m_viewPanX, 32.0f + m_viewPanY, 0.0f);
        m_previewRoot->getTransform()->setScale(m_viewZoom, m_viewZoom, 1.0f);
        setStatus("Viewport zoom " + std::to_string(m_viewZoom) + " (was " + std::to_string(oldZoom) + ")");
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_MIDDLE) {
        m_panning = true;
        m_lastPointerX = panelX;
        m_lastPointerY = panelY;
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_panning) {
        m_viewPanX += panelX - m_lastPointerX;
        m_viewPanY += panelY - m_lastPointerY;
        m_lastPointerX = panelX;
        m_lastPointerY = panelY;
        m_previewRoot->getTransform()->setPosition(m_viewPanX, 32.0f + m_viewPanY, 0.0f);
        return;
    }
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        std::string error;
        const bool additive = (event.modifiers & TOUCH_MODIFIER_CTRL) != 0;
        if (m_session->selectAt(x, y, error, additive)) {
            notifySelectionChanged();
            float nodeX = 0.0f;
            float nodeY = 0.0f;
            float nodeWidth = 0.0f;
            float nodeHeight = 0.0f;
            m_resizing = m_session->model().selectedRect(nodeX, nodeY, nodeWidth, nodeHeight) &&
                         x >= nodeX + nodeWidth - 12.0f && y >= nodeY + nodeHeight - 12.0f;
            m_dragging = !m_resizing;
            m_lastPointerX = panelX;
            m_lastPointerY = panelY;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_resizing && !m_selectedNodeId.empty()) {
        float nodeX = 0.0f;
        float nodeY = 0.0f;
        float nodeWidth = 0.0f;
        float nodeHeight = 0.0f;
        if (!m_session->model().selectedRect(nodeX, nodeY, nodeWidth, nodeHeight))
            return;
        const float dx = (panelX - m_lastPointerX) / m_viewZoom;
        const float dy = (panelY - m_lastPointerY) / m_viewZoom;
        std::string error;
        if (m_session->resizeGizmo(m_selectedNodeId, std::max(1.0f, nodeWidth + dx), std::max(1.0f, nodeHeight + dy), true, error)) {
            rebuildRuntime();
            m_lastPointerX = panelX;
            m_lastPointerY = panelY;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_dragging && !m_selectedNodeId.empty()) {
        float dx = (panelX - m_lastPointerX) / m_viewZoom;
        float dy = (panelY - m_lastPointerY) / m_viewZoom;
        std::string error;
        if (m_session->moveSelection(dx, dy, true, error)) {
            rebuildRuntime();
            m_lastPointerX = panelX;
            m_lastPointerY = panelY;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        m_dragging = false;
        m_resizing = false;
        m_panning = false;
    }
}

void EditorShell::handleInput(std::vector<TouchEvent>& events) {
    pollBuild();
    for (const auto& event : events) {
        auto target = event.target;
        bool viewportTarget = false;
        while (target) {
            if (target == m_viewportPanel) {
                viewportTarget = true;
                break;
            }
            target = target->m_parent;
        }
        if (viewportTarget) {
            handleViewportPointer(event);
        }
    }
}

void EditorShell::handleKey(int key, int action, int mods) {
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        if (m_engine) {
            m_engine->toggleDebugOverlay();
        }
        return;
    }

    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    if (!m_editProperty.empty()) {
        if (key == GLFW_KEY_ENTER) {
            commitPropertyEdit();
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            m_editProperty.clear();
            m_editValue.clear();
            setStatus("Property edit cancelled");
            return;
        }
        if (key == GLFW_KEY_BACKSPACE && !m_editValue.empty()) {
            m_editValue.pop_back();
            setStatus("Editing " + m_editProperty + ": " + m_editValue);
            return;
        }
    }
    char command = 0;
    if (key == GLFW_KEY_S)
        command = 's';
    else if (key == GLFW_KEY_Z)
        command = 'z';
    else if (key == GLFW_KEY_Y)
        command = 'y';
    if (!command || !(mods & GLFW_MOD_CONTROL))
        return;
    std::string error;
    const bool changed =
        command == 's' ? m_session->save(error) : (command == 'z' ? ((mods & GLFW_MOD_SHIFT) ? m_session->redo(error) : m_session->undo(error)) : m_session->redo(error));
    if (changed && command != 's')
        rebuildRuntime();
    setStatus(error.empty() ? "Shortcut applied" : error);
    refreshSceneTree();
    refreshInspector();
}

void EditorShell::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (const auto iterator = previousKeyCallbacks().find(window);
        iterator != previousKeyCallbacks().end() && iterator->second) {
        iterator->second(window, key, scancode, action, mods);
    }
    const auto iterator = shells().find(window);
    if (iterator != shells().end() && iterator->second) {
        iterator->second->handleKey(key, action, mods);
    }
}

void EditorShell::charCallback(GLFWwindow* window, unsigned int codepoint) {
    if (const auto iterator = previousCharCallbacks().find(window);
        iterator != previousCharCallbacks().end() && iterator->second) {
        iterator->second(window, codepoint);
    }
    const auto iterator = shells().find(window);
    if (iterator != shells().end() && iterator->second)
        iterator->second->handleChar(codepoint);
}

bool EditorShell::initialize(std::string& error) {
    if (!m_project.load(m_projectPath, error))
        return false;
    if (!m_session->load(error))
        return false;
    if (!m_assets.scan(m_projectPath.parent_path(), m_assetRoot, error)) {
        return false;
    }
    if (!m_fileSystem.scan(
            m_project.projectRoot(), &m_assets, error)) {
        return false;
    }
    buildLayout();
    if (m_window) {
        m_framebufferSizeConnection =
            m_window->events().onFramebufferSizeChanged.connect(
                [this](const Vector2& size) { handleFramebufferResize(size); });
    }
    runImportQueue();
    refreshSceneTree();
    rebuildRuntime();
    if (m_window) {
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getSurface());
        if (glfwWindow) {
            shells()[glfwWindow] = this;
            previousKeyCallbacks()[glfwWindow] =
                glfwSetKeyCallback(glfwWindow, keyCallback);
            previousCharCallbacks()[glfwWindow] =
                glfwSetCharCallback(glfwWindow, charCallback);
        }
    }
    if (m_engine && m_engine->getFrameState() && m_engine->getFrameState()->inputEventsManager) {
        m_inputConnection = m_engine->getFrameState()->inputEventsManager->getResolvedInputEventsDispatcher().connect(
            [this](std::vector<TouchEvent>& events) { handleInput(events); });
    }
    setStatus("Ready");
    return true;
}

}  // namespace morrow::editor
