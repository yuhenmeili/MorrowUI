#include "panels/BuildPanel.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <sstream>

#include "EditorShell.h"
#include "base/Transform.h"
#include "elements/MRTextEdit.h"
#include "panels/OutputPanel.h"

namespace morrow::editor {
BuildPanel::BuildPanel(EditorShell& shell) : m_shell(shell) {
}

void BuildPanel::resize() {
    if (panel && log) {
        const Vector3 size = panel->getTransform()->getSize();
        log->getTransform()->setPosition(6.0f, 25.0f, 0.0f);
        log->getTransform()->setSize(std::max(1.0f, size.x - 12.0f), std::max(1.0f, size.y - 31.0f));
    }
}

void BuildPanel::setPreviewState(PreviewState state) {
    m_shell.m_previewState = state;
    const char* label = "Stopped";
    switch (state) {
        case PreviewState::Starting:
            label = "Starting";
            break;
        case PreviewState::Running:
            label = "Running";
            break;
        case PreviewState::Outdated:
            label = "Outdated";
            break;
        case PreviewState::Failed:
            label = "Failed";
            break;
        case PreviewState::Stopped:
            break;
    }
    m_shell.setStatus(std::string("Preview: ") + label);
}

void BuildPanel::stop() {
    if (m_shell.m_buildFuture.valid() && m_shell.m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        m_shell.m_buildQueue.cancel();
        m_shell.setStatus("Preview stop requested");
    } else {
        setPreviewState(PreviewState::Stopped);
    }
}

void BuildPanel::run(BuildTaskKind kind) {
    if (m_shell.m_buildFuture.valid() && m_shell.m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        m_shell.setStatus("A build process is already running");
        return;
    }
    if ((kind == BuildTaskKind::BuildAndRun || kind == BuildTaskKind::Run) && m_shell.m_session->isDirty()) {
        std::string saveError;
        if (!m_shell.m_session->save(saveError)) {
            m_shell.setStatus("Cannot run unsaved scene: " + saveError);
            return;
        }
        m_shell.setStatus("Scene saved for runtime");
    }
    const auto projectRoot = m_shell.m_project.pathValue("cmake_root", m_shell.m_project.projectRoot());
    const auto buildRoot = m_shell.m_project.pathValue("build_root", "build");
    const auto target = m_shell.m_project.value("preview_target", "MorrowEditor");
    const auto generator = m_shell.m_project.value("build_generator", "MinGW Makefiles");
    const std::vector<std::string> runtimeArguments = {
        "--project",
        m_shell.m_projectPath.string(),
        "--scene",
        m_shell.m_scenePath.string(),
    };
    const auto configuredExecutable = buildRoot / (target + ".exe");
    if (m_shell.m_lastSuccessfulExecutable.empty()) {
        std::error_code searchError;
        if (std::filesystem::exists(configuredExecutable))
            m_shell.m_lastSuccessfulExecutable = configuredExecutable;
        else if (std::filesystem::exists(buildRoot)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, searchError)) {
                if (!searchError && entry.is_regular_file() && entry.path().filename() == target + ".exe") {
                    m_shell.m_lastSuccessfulExecutable = entry.path();
                    break;
                }
            }
        }
    }
    if (kind == BuildTaskKind::Run && !m_shell.m_lastSuccessfulExecutable.empty() && std::filesystem::exists(m_shell.m_lastSuccessfulExecutable))
        m_shell.setStatus("Run last successful started");
    else if (kind == BuildTaskKind::Run)
        m_shell.setStatus("No successful preview executable; using configured target");
    else
        m_shell.setStatus(kind == BuildTaskKind::Configure ? "Configure started" : (kind == BuildTaskKind::Build ? "Build started" : "Build & Run started"));
    m_shell.m_pendingBuildKind = kind;
    if (kind == BuildTaskKind::Run || kind == BuildTaskKind::BuildAndRun)
        setPreviewState(PreviewState::Starting);
    m_shell.m_buildFuture = std::async(std::launch::async, [this, kind, projectRoot, buildRoot, target, generator, runtimeArguments] {
        if (kind == BuildTaskKind::Configure)
            return m_shell.m_buildQueue.configure(projectRoot, buildRoot, generator);
        if (kind == BuildTaskKind::Build)
            return m_shell.m_buildQueue.build(projectRoot, buildRoot, target, generator);
        if (kind == BuildTaskKind::BuildAndRun)
            return m_shell.m_buildQueue.buildAndRun(projectRoot, buildRoot, target, generator, runtimeArguments);
        auto executable = m_shell.m_lastSuccessfulExecutable.empty() ? buildRoot / (target + ".exe") : m_shell.m_lastSuccessfulExecutable;
        return m_shell.m_buildQueue.runTarget(executable, executable.parent_path(), runtimeArguments);
    });
}

void BuildPanel::poll() {
    bool outputChanged = false;
    for (const auto& chunk : m_shell.m_buildQueue.drainOutput()) {
        auto& remainder = chunk.stderrStream ? m_shell.m_stderrRemainder : m_shell.m_stdoutRemainder;
        remainder += chunk.text;
        size_t newline = 0;
        while ((newline = remainder.find('\n')) != std::string::npos) {
            auto line = remainder.substr(0, newline);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (!line.empty())
                m_shell.m_outputLines.push_back(std::string(chunk.stderrStream ? "[stderr] " : "[stdout] ") + line);
            remainder.erase(0, newline + 1);
            outputChanged = true;
        }
    }
    while (m_shell.m_outputLines.size() > 6) {
        m_shell.m_outputLines.erase(m_shell.m_outputLines.begin());
        outputChanged = true;
    }
    if (outputChanged)
        m_shell.m_output->refresh();

    if (!m_shell.m_buildFuture.valid())
        return;
    if (m_shell.m_buildQueue.state() == BuildProcessState::Running &&
        (m_shell.m_pendingBuildKind == BuildTaskKind::Run || m_shell.m_pendingBuildKind == BuildTaskKind::BuildAndRun) && m_shell.m_previewState == PreviewState::Starting) {
        m_shell.m_previewState = PreviewState::Running;
    }
    if (m_shell.m_buildFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;
    const auto result = m_shell.m_buildFuture.get();
    appendResult(result);
    if (result.success && (m_shell.m_pendingBuildKind == BuildTaskKind::Build || m_shell.m_pendingBuildKind == BuildTaskKind::BuildAndRun)) {
        const auto buildRoot = m_shell.m_project.pathValue("build_root", "build");
        const auto target = m_shell.m_project.value("preview_target", "MorrowEditor");
        m_shell.m_lastSuccessfulExecutable = buildRoot / (target + ".exe");
        if (!std::filesystem::exists(m_shell.m_lastSuccessfulExecutable)) {
            std::error_code searchError;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, searchError)) {
                if (!searchError && entry.is_regular_file() && entry.path().filename() == target + ".exe") {
                    m_shell.m_lastSuccessfulExecutable = entry.path();
                    break;
                }
            }
        }
        setPreviewState(m_shell.m_pendingBuildKind == BuildTaskKind::BuildAndRun ? PreviewState::Stopped : PreviewState::Outdated);
    } else if (m_shell.m_pendingBuildKind == BuildTaskKind::Run || m_shell.m_pendingBuildKind == BuildTaskKind::BuildAndRun) {
        setPreviewState(result.cancelled ? PreviewState::Stopped : (result.success ? PreviewState::Stopped : PreviewState::Failed));
    }
    m_shell.setStatus(result.cancelled
                          ? "Process cancelled"
                          : (result.success ? "Process succeeded, exit=" + std::to_string(result.exitCode) : "Process failed, exit=" + std::to_string(result.exitCode)));
}

void BuildPanel::appendResult(const BuildTaskResult& result) {
    std::istringstream stdoutStream(result.stdoutText);
    std::string line;
    while (std::getline(stdoutStream, line))
        if (!line.empty())
            m_shell.m_outputLines.push_back("[stdout] " + line);
    std::istringstream stderrStream(result.stderrText);
    while (std::getline(stderrStream, line))
        if (!line.empty())
            m_shell.m_outputLines.push_back("[stderr] " + line);
    for (const auto& diagnostic : result.diagnostics) {
        m_shell.m_outputLines.push_back(std::string(diagnostic.error ? "[error] " : "[warning] ") + diagnostic.file.string() + ":" + std::to_string(diagnostic.line) + ":" +
                                        std::to_string(diagnostic.column) + " " + diagnostic.message);
    }
    while (m_shell.m_outputLines.size() > 6)
        m_shell.m_outputLines.erase(m_shell.m_outputLines.begin());
    m_shell.m_output->refresh();
}

}  // namespace morrow::editor
