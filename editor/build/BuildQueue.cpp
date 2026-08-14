#include "BuildQueue.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <thread>

namespace {
void readPipe(HANDLE pipe, std::string& output) {
    char buffer[1024];
    DWORD read = 0;
    while (ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) && read > 0)
        output.append(buffer, buffer + read);
}
}  // namespace

namespace morrow::editor {

BuildTaskResult BuildQueue::run(BuildTaskKind kind, const std::string& command, const std::filesystem::path& workingDirectory) const {
    BuildTaskResult result{kind, false, -1, command, {}, {}, {}};
    m_cancelRequested.store(false);
    m_state.store(BuildProcessState::Running);
    std::error_code filesystemError;
    if (!std::filesystem::exists(workingDirectory, filesystemError)) {
        result.output = "working directory does not exist: " + workingDirectory.string();
        m_state.store(BuildProcessState::Failed);
        return result;
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    HANDLE stderrRead = nullptr;
    HANDLE stderrWrite = nullptr;
    if (!CreatePipe(&stdoutRead, &stdoutWrite, &security, 0) || !CreatePipe(&stderrRead, &stderrWrite, &security, 0)) {
        result.output = "failed to create process pipes";
        m_state.store(BuildProcessState::Failed);
        return result;
    }
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = stdoutWrite;
    startup.hStdError = stderrWrite;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process{};
    std::string commandLine = "cmd.exe /D /S /C \"" + command + "\"";
    const BOOL started = CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                                        workingDirectory.string().c_str(), &startup, &process);
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);
    if (!started) {
        CloseHandle(stdoutRead);
        CloseHandle(stderrRead);
        result.output = "failed to start process, error=" + std::to_string(GetLastError());
        m_state.store(BuildProcessState::Failed);
        return result;
    }
    std::thread stdoutThread(readPipe, stdoutRead, std::ref(result.stdoutText));
    std::thread stderrThread(readPipe, stderrRead, std::ref(result.stderrText));
    while (WaitForSingleObject(process.hProcess, 20) == WAIT_TIMEOUT) {
        if (m_cancelRequested.load()) {
            result.cancelled = true;
            TerminateProcess(process.hProcess, ERROR_CANCELLED);
            break;
        }
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    result.exitCode = static_cast<int>(exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    stdoutThread.join();
    stderrThread.join();
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    result.output = result.stdoutText + result.stderrText;
    result.success = !result.cancelled && result.exitCode == 0;
    m_state.store(result.cancelled ? BuildProcessState::Cancelled : (result.success ? BuildProcessState::Succeeded : BuildProcessState::Failed));
    return result;
}

BuildTaskResult BuildQueue::configure(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& generator) const {
    const auto command = "cmake -S \"" + projectRoot.string() + "\" -B \"" + buildRoot.string() + "\" -G \"" + generator + "\"";
    return run(BuildTaskKind::Configure, command, projectRoot);
}

BuildTaskResult BuildQueue::build(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target) const {
    const auto command = "cmake --build \"" + buildRoot.string() + "\" --target \"" + target + "\"";
    return run(BuildTaskKind::Build, command, projectRoot);
}

BuildTaskResult BuildQueue::runTarget(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory,
                                      const std::vector<std::string>& arguments) const {
    std::string command = "\"" + executable.string() + "\"";
    for (const auto& argument : arguments)
        command += " \"" + argument + "\"";
    return run(BuildTaskKind::Run, command, workingDirectory);
}

void BuildQueue::cancel() const {
    m_cancelRequested.store(true);
}

BuildProcessState BuildQueue::state() const {
    return m_state.load();
}

}  // namespace morrow::editor
