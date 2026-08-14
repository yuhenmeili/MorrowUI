#include "EditorShell.h"

#include <algorithm>
#include <utility>

#include "Engine.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"
#include "elements/MRButton.h"
#include "elements/MRLabel.h"
#include "platform/Window.h"
#include "scene/SceneInstantiator.h"
#include "wgl/OpenglHeader.h"

namespace {
std::unordered_map<GLFWwindow*, morrow::editor::EditorShell*>& shells() {
    static std::unordered_map<GLFWwindow*, morrow::editor::EditorShell*> value;
    return value;
}

std::shared_ptr<morrow::UIWidget> makePanel(float x, float y, float width, float height) {
    // Layout containers do not render themselves. Avoid empty Materials with
    // no shader source being submitted to the renderer.
    auto panel = std::make_shared<morrow::UIWidget>(false);
    auto transform = panel->getTransform();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    return panel;
}
}  // namespace

namespace morrow::editor {

EditorShell::EditorShell(const std::shared_ptr<Window>& window, const std::shared_ptr<Engine>& engine, std::filesystem::path projectPath, std::filesystem::path scenePath,
                         std::filesystem::path assetRoot) :
    m_window(window), m_engine(engine), m_projectPath(std::move(projectPath)), m_scenePath(std::move(scenePath)), m_assetRoot(std::move(assetRoot)),
    m_session(std::make_shared<EditorSession>(m_scenePath)) {
}

EditorShell::~EditorShell() {
    if (!m_inputObserver.empty() && m_engine && m_engine->getFrameState() && m_engine->getFrameState()->inputEventsManager) {
        m_engine->getFrameState()->inputEventsManager->getInputEventsDispatcher().remove(m_inputObserver);
    }
    if (m_window) {
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getSurface());
        if (glfwWindow)
            shells().erase(glfwWindow);
    }
}

void EditorShell::setStatus(const std::string& text) {
    m_status = text;
    if (m_statusPanel) {
        m_statusPanel->m_children.clear();
        addLabel(m_statusPanel, text, 8.0f, 4.0f, 1200.0f, 24.0f);
    }
}

void EditorShell::runImportQueue() {
    std::vector<ImportTaskResult> results;
    std::string error;
    if (!m_importQueue.importAll(m_assets, m_projectPath.parent_path(), "windows", results, error)) {
        setStatus("Import failed: " + error);
        return;
    }
    setStatus("Imported " + std::to_string(results.size()) + " assets");
}

void EditorShell::addLabel(const std::shared_ptr<UIWidget>& parent, const std::string& text, float x, float y, float width, float height) {
    auto label = std::make_shared<MRLabel>();
    label->setText(std::wstring(text.begin(), text.end()), "default");
    auto transform = label->getComponent<Transform>();
    transform->setPosition(x, y, 0.0f);
    transform->setSize(width, height);
    parent->addChild(label);
}

std::shared_ptr<MRButton> EditorShell::addButton(const std::shared_ptr<UIWidget>& parent, const std::wstring& text, float x, float y, float width, float height,
                                                 std::function<void()> callback) {
    auto button = MRButton::create();
    button->setText(text, "default");
    button->setOnClickCallback(std::move(callback));
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
    m_shellRoot->getTransform()->setSize(1280.0f, 720.0f);

    auto toolbar = makePanel(0.0f, 0.0f, 1280.0f, 40.0f);
    m_sceneTreePanel = makePanel(0.0f, 40.0f, 240.0f, 620.0f);
    m_viewportPanel = makePanel(240.0f, 40.0f, 740.0f, 620.0f);
    m_inspectorPanel = makePanel(980.0f, 40.0f, 300.0f, 620.0f);
    m_statusPanel = makePanel(0.0f, 660.0f, 1280.0f, 60.0f);
    m_previewRoot = makePanel(0.0f, 0.0f, 740.0f, 620.0f);
    m_previewRoot->setWidgetName("PreviewRoot");

    m_shellRoot->addChild(toolbar);
    m_shellRoot->addChild(m_sceneTreePanel);
    m_shellRoot->addChild(m_viewportPanel);
    m_shellRoot->addChild(m_inspectorPanel);
    m_shellRoot->addChild(m_statusPanel);
    m_viewportPanel->addChild(m_previewRoot);
    m_window->addChild(m_shellRoot);

    addLabel(toolbar, "MorrowEditor", 8.0f, 5.0f, 150.0f, 28.0f);
    addButton(toolbar, L"Save", 170.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->save(error))
            setStatus("Saved");
        else
            setStatus(error);
    });
    addButton(toolbar, L"Undo", 248.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->undo(error)) {
            rebuildRuntime();
            refreshSceneTree();
            refreshInspector();
        }
        setStatus(error.empty() ? "Undo" : error);
    });
    addButton(toolbar, L"Redo", 326.0f, 4.0f, 72.0f, 30.0f, [this] {
        std::string error;
        if (m_session->redo(error)) {
            rebuildRuntime();
            refreshSceneTree();
            refreshInspector();
        }
        setStatus(error.empty() ? "Redo" : error);
    });
    addLabel(m_sceneTreePanel, "Scene", 8.0f, 6.0f, 220.0f, 28.0f);
    addLabel(m_viewportPanel, "2D Viewport", 8.0f, 6.0f, 220.0f, 28.0f);
    addLabel(m_inspectorPanel, "Inspector", 8.0f, 6.0f, 260.0f, 28.0f);
    addLabel(m_statusPanel, "Ready", 8.0f, 4.0f, 1200.0f, 24.0f);
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
                                        m_selectedNodeId = id;
                                        refreshInspector();
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
        addButton(m_inspectorPanel, std::wstring(property.value.begin(), property.value.end()), 8.0f, y, 280.0f, 30.0f, [this, property] {
            std::string error;
            m_session->setProperty(m_selectedNodeId, property.name, property.value, false, error);
            setStatus(error.empty() ? "Property applied" : error);
            rebuildRuntime();
        });
        y += 36.0f;
    }
}

void EditorShell::rebuildRuntime() {
    if (!m_previewRoot)
        return;
    m_previewRoot->m_children.clear();
    std::string error;
    if (!SceneInstantiator::instantiate(m_session->document(), m_previewRoot, &m_assets, error)) {
        setStatus(error);
    }
}

void EditorShell::handleViewportPointer(const TouchEvent& event) {
    const float x = event.positionX - m_viewportX;
    const float y = event.positionY - m_viewportY;
    if (x < 0.0f || y < 32.0f || x > m_viewportWidth || y > m_viewportHeight)
        return;
    if (event.eventType == TOUCH_EVENT_TYPE_TOUCH && event.button == TOUCH_MOUSE_BUTTON_LEFT) {
        std::string error;
        if (m_session->selectAt(x, y - 32.0f, error)) {
            m_selectedNodeId = m_session->model().selection().nodeIds.front();
            m_dragging = true;
            m_lastPointerX = x;
            m_lastPointerY = y;
            refreshInspector();
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_MOVE && m_dragging && !m_selectedNodeId.empty()) {
        const auto node = m_session->document().findNode(m_selectedNodeId);
        if (!node)
            return;
        const auto property = node->properties.find("position");
        if (property == node->properties.end())
            return;
        float dx = x - m_lastPointerX;
        float dy = y - m_lastPointerY;
        std::string error;
        std::vector<float> values;
        const auto body = property->second.substr(property->second.find('(') + 1, property->second.size() - property->second.find('(') - 2);
        size_t start = 0;
        while (start <= body.size()) {
            auto separator = body.find(',', start);
            try {
                values.push_back(std::stof(body.substr(start, separator - start)));
            } catch (...) {
                return;
            }
            if (separator == std::string::npos)
                break;
            start = separator + 1;
        }
        if (values.size() == 3 && m_session->moveGizmo(m_selectedNodeId, values[0] + dx, values[1] + dy, values[2], true, error)) {
            rebuildRuntime();
            m_lastPointerX = x;
            m_lastPointerY = y;
        }
    } else if (event.eventType == TOUCH_EVENT_TYPE_RELEASE) {
        m_dragging = false;
    }
}

void EditorShell::handleInput(std::vector<TouchEvent>& events) {
    for (const auto& event : events)
        handleViewportPointer(event);
}

void EditorShell::handleKey(int key, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
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

void EditorShell::keyCallback(GLFWwindow* window, int key, int, int action, int mods) {
    const auto iterator = shells().find(window);
    if (iterator != shells().end() && iterator->second) {
        iterator->second->handleKey(key, action, mods);
    }
}

bool EditorShell::initialize(std::string& error) {
    if (!m_session->load(error))
        return false;
    if (!m_assets.scan(m_projectPath.parent_path(), m_assetRoot, error)) {
        return false;
    }
    buildLayout();
    runImportQueue();
    refreshSceneTree();
    rebuildRuntime();
    if (m_window) {
        auto* glfwWindow = static_cast<GLFWwindow*>(m_window->getSurface());
        if (glfwWindow) {
            shells()[glfwWindow] = this;
            glfwSetKeyCallback(glfwWindow, keyCallback);
        }
    }
    if (m_engine && m_engine->getFrameState() && m_engine->getFrameState()->inputEventsManager) {
        m_inputObserver = m_engine->getFrameState()->inputEventsManager->getInputEventsDispatcher().add([this](std::vector<TouchEvent>& events) { handleInput(events); });
    }
    setStatus("Ready");
    return true;
}

}  // namespace morrow::editor
