#include "ui/CommandPalette.h"

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>

#include "EditorShell.h"
#include "morrow/Engine.h"
#include "morrow/base/Interaction.h"
#include "morrow/base/Transform.h"
#include "morrow/elements/MRButton.h"
#include "morrow/elements/MRColor.h"
#include "morrow/elements/MRLabel.h"
#include "panels/AnimationPanel.h"
#include "panels/AssetBrowserPanel.h"
#include "panels/BuildPanel.h"
#include "panels/InspectorPanel.h"
#include "panels/SceneTreePanel.h"
#include "panels/ToolbarPanel.h"
#include "panels/ViewportPanel.h"

namespace {
std::wstring wide(const std::string& text) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
    } catch (...) {
        return std::wstring(text.begin(), text.end());
    }
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}
}  // namespace

namespace morrow::editor {

CommandPalette::CommandPalette(EditorShell& shell) : UIWidget(false), m_shell(shell) {
    setWidgetName("CommandPalette");
    setVisible(false);
}

void CommandPalette::buildChrome() {
    if (m_chromeBuilt)
        return;
    m_chromeBuilt = true;
    m_backdrop = MRColor::create();
    m_backdrop->setColor(Vector4(0.05f, 0.06f, 0.08f, 0.55f));
    m_backdrop->getTransform()->setSize(2000.0f, 1200.0f);
    // 点背景关闭
    if (auto interaction = m_backdrop->getComponent<Interaction>()) {
        interaction->addEventListener(TOUCH_EVENT_TYPE_RELEASE, [this](TouchEvent&) { close(); });
    }
    addChild(m_backdrop);

    auto panel = MRColor::create();
    panel->setColor(Vector4(0.13f, 0.15f, 0.19f, 0.98f));
    panel->setRounding(8.0f);
    panel->getTransform()->setPosition(0.0f, 0.0f, 0.0f);
    panel->getTransform()->setSize(520.0f, 360.0f);
    addChild(panel);

    m_hint = std::make_shared<MRLabel>();
    m_hint->setText(L"Type to filter, Enter to run, Esc to close", "default");
    m_hint->setFontSize(12.0f);
    m_hint->setFontColor(0.55f, 0.60f, 0.68f, 1.0f);
    m_hint->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    m_hint->getTransform()->setPosition(16.0f, 322.0f, 0.0f);
    m_hint->getTransform()->setSize(480.0f, 24.0f);
    addChild(m_hint);

    m_filterLabel = std::make_shared<MRLabel>();
    m_filterLabel->setText(L"> ", "default");
    m_filterLabel->setFontSize(15.0f);
    m_filterLabel->setFontColor(0.88f, 0.92f, 0.98f, 1.0f);
    m_filterLabel->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    m_filterLabel->getTransform()->setPosition(16.0f, 8.0f, 0.0f);
    m_filterLabel->getTransform()->setSize(480.0f, 26.0f);
    addChild(m_filterLabel);

    m_emptyHint = std::make_shared<MRLabel>();
    m_emptyHint->setText(L"No matching commands", "default");
    m_emptyHint->setFontSize(14.0f);
    m_emptyHint->setFontColor(0.60f, 0.64f, 0.72f, 1.0f);
    m_emptyHint->setAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    m_emptyHint->getTransform()->setPosition(16.0f, 40.0f, 0.0f);
    m_emptyHint->getTransform()->setSize(480.0f, 24.0f);
    m_emptyHint->setVisible(false);
    addChild(m_emptyHint);
}

void CommandPalette::open() {
    buildChrome();
    m_open = true;
    m_filter.clear();
    setVisible(true);
    refreshEntries();
    m_shell.m_inspector->beginLegacyPropertyEdit("__palette__", "");
}

void CommandPalette::close() {
    m_open = false;
    setVisible(false);
    m_shell.m_inspector->cancelLegacyEdit();
    m_shell.setStatus("Command palette closed");
}

void CommandPalette::setFilter(const std::string& filter) {
    m_filter = filter;
    refreshEntries();
}

void CommandPalette::executeFirst() {
    if (m_visible.empty()) {
        close();
        return;
    }
    Entry entry = std::move(m_visible.front());
    close();
    if (entry.action)
        entry.action();
}

void CommandPalette::refreshEntries() {
    if (!m_chromeBuilt)
        return;
    m_buttonConnections.clear();
    for (const auto& button : m_commandButtons)
        removeChild(button);
    m_commandButtons.clear();

    const auto all = collectEntries();
    const std::string needle = lowercase(m_filter);
    m_visible.clear();
    for (const auto& entry : all) {
        std::wstring wideLabel = entry.label;
        std::string narrowLabel(wideLabel.begin(), wideLabel.end());
        if (needle.empty() || lowercase(narrowLabel).find(needle) != std::string::npos)
            m_visible.push_back(entry);
        if (m_visible.size() >= 9)
            break;
    }

    m_emptyHint->setVisible(m_visible.empty());
    if (m_filterLabel)
        m_filterLabel->setText(m_filter.empty() ? L"> (type to filter)" : L"> " + wide(m_filter), "default");
    const float top = 40.0f;
    for (size_t index = 0; index < m_visible.size(); ++index) {
        auto button = MRButton::create();
        button->setText(m_visible[index].label, "default");
        button->setTextFontSize(14.0f);
        button->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        button->setTextColor(Vector4(0.90f, 0.92f, 0.95f, 1.0f));
        button->setBackgroundColor(index == 0 ? Vector4(0.18f, 0.28f, 0.44f, 1.0f) : Vector4(0.16f, 0.18f, 0.22f, 1.0f));
        button->setHoverColor(Vector4(0.22f, 0.32f, 0.48f, 1.0f));
        button->setPressedColor(Vector4(0.12f, 0.18f, 0.30f, 1.0f));
        button->setCornerRadius(4.0f);
        button->getTransform()->setPosition(12.0f, top + static_cast<float>(index) * 30.0f, 0.0f);
        button->getTransform()->setSize(496.0f, 26.0f);
        m_buttonConnections.emplace_back(button->events().onClicked.connect([this, index](BaseButton&) {
            if (index >= m_visible.size())
                return;
            Entry entry = std::move(m_visible[index]);
            close();
            if (entry.action)
                entry.action();
        }));
        addChild(button);
        m_commandButtons.push_back(std::move(button));
    }
}

std::vector<CommandPalette::Entry> CommandPalette::collectEntries() const {
    std::vector<Entry> entries;
    const auto add = [&entries](const wchar_t* label, std::function<void()> action) { entries.push_back({label, std::move(action)}); };
    add(L"Save Scene", [this] {
        std::string error;
        if (!m_shell.m_session->save(error))
            m_shell.setStatus(error);
    });
    add(L"Undo", [this] {
        std::string error;
        if (m_shell.m_session->undo(error)) {
            m_shell.m_viewport->rebuildRuntime();
            m_shell.m_sceneTree->refresh();
        }
    });
    add(L"Redo", [this] {
        std::string error;
        if (m_shell.m_session->redo(error)) {
            m_shell.m_viewport->rebuildRuntime();
            m_shell.m_sceneTree->refresh();
        }
    });
    add(L"Build", [this] { m_shell.m_build->run(BuildTaskKind::Build); });
    add(L"Build && Run", [this] { m_shell.m_build->run(BuildTaskKind::BuildAndRun); });
    add(L"Run Last Build", [this] { m_shell.m_build->run(BuildTaskKind::Run); });
    add(L"Stop Running", [this] { m_shell.m_build->stop(); });
    add(L"Import Assets", [this] { m_shell.m_assetsPanel->importAssets(); });
    add(L"Toggle Snapping", [this] {
        m_shell.m_viewport->snapEnabled = !m_shell.m_viewport->snapEnabled;
        m_shell.m_viewport->refreshGuides();
        m_shell.refreshSnapButtons();
    });
    add(L"Clear Guides", [this] { m_shell.editClearGuides(); });
    add(L"Toggle Lock Selected", [this] { m_shell.editToggleLock(); });
    add(L"Focus Selection (F)", [this] { m_shell.m_viewport->focusSelection(); });
    add(L"Canvas: Fit to Panel", [this] { m_shell.m_viewport->applyCanvasPreset(0); });
    add(L"Toggle Debug Overlay (F3)", [this] {
        if (m_shell.m_engine)
            m_shell.m_engine->toggleDebugOverlay();
    });
    add(L"Keyboard Shortcuts", [this] { m_shell.printShortcuts(); });
    return entries;
}

}  // namespace morrow::editor
