#include "BuildQueue.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <regex>
#include <sstream>
#include <thread>

namespace {
std::string quoteWindowsArgument(const std::string& argument) {
    if (!argument.empty() && argument.find_first_of(" \t\n\v\"") == std::string::npos) {
        return argument;
    }

    std::string quoted{"\""};
    size_t backslashes = 0;
    for (const char character : argument) {
        if (character == '\\') {
            ++backslashes;
            continue;
        }
        if (character == '"') {
            quoted.append(backslashes * 2 + 1, '\\');
            quoted.push_back('"');
        } else {
            quoted.append(backslashes, '\\');
            quoted.push_back(character);
        }
        backslashes = 0;
    }
    quoted.append(backslashes * 2, '\\');
    quoted.push_back('"');
    return quoted;
}

std::filesystem::path resolveExecutable(const std::filesystem::path& executable) {
    if (executable.has_parent_path())
        return executable;

    std::vector<char> buffer(MAX_PATH);
    while (true) {
        const DWORD length = SearchPathA(nullptr, executable.string().c_str(), ".exe", static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        if (length == 0)
            return executable;
        if (length < buffer.size())
            return std::filesystem::path(std::string(buffer.data(), length));
        buffer.resize(static_cast<size_t>(length) + 1);
    }
}

std::string makeCommandLine(const std::filesystem::path& executable, const std::vector<std::string>& arguments) {
    std::string commandLine = quoteWindowsArgument(executable.string());
    for (const auto& argument : arguments) {
        commandLine.push_back(' ');
        commandLine += quoteWindowsArgument(argument);
    }
    return commandLine;
}

std::filesystem::path findExecutable(const std::filesystem::path& buildRoot, const std::string& target) {
    const auto direct = buildRoot / (target + ".exe");
    if (std::filesystem::exists(direct))
        return direct;
    std::error_code error;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(buildRoot, error)) {
        if (error)
            break;
        if (entry.is_regular_file() && entry.path().filename() == target + ".exe")
            return entry.path();
    }
    return direct;
}

void parseDiagnostics(morrow::editor::BuildTaskResult& result) {
    const std::regex gccPattern(R"(^(.+):([0-9]+):([0-9]+):\s+(error|warning):\s+(.*)$)");
    const std::regex simplePattern(R"(^(.+):([0-9]+):\s+(error|warning):\s+(.*)$)");
    std::istringstream lines(result.output);
    std::string line;
    while (std::getline(lines, line)) {
        std::smatch match;
        morrow::editor::BuildDiagnostic diagnostic;
        if (std::regex_match(line, match, gccPattern)) {
            diagnostic.file = match[1].str();
            diagnostic.line = std::stoi(match[2].str());
            diagnostic.column = std::stoi(match[3].str());
            diagnostic.error = match[4].str() == "error";
            diagnostic.message = match[5].str();
        } else if (std::regex_match(line, match, simplePattern)) {
            diagnostic.file = match[1].str();
            diagnostic.line = std::stoi(match[2].str());
            diagnostic.error = match[3].str() == "error";
            diagnostic.message = match[4].str();
        } else {
            continue;
        }
        result.diagnostics.push_back(std::move(diagnostic));
    }
}
}  // namespace

namespace morrow::editor {

void BuildQueue::pushOutput(bool stderrStream, const char* data, size_t size) const {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    m_liveOutput.push_back({stderrStream, std::string(data, size)});
}

std::vector<BuildOutputChunk> BuildQueue::drainOutput() const {
    std::lock_guard<std::mutex> lock(m_outputMutex);
    std::vector<BuildOutputChunk> result;
    result.swap(m_liveOutput);
    return result;
}

BuildTaskResult BuildQueue::run(BuildTaskKind kind, const std::filesystem::path& executable, const std::vector<std::string>& arguments,
                                const std::filesystem::path& workingDirectory, bool showChildWindows) const {
    const auto resolvedExecutable = resolveExecutable(executable);
    const auto command = makeCommandLine(resolvedExecutable, arguments);
    BuildTaskResult result{kind, false, -1, command, {}, {}, {}};
    m_cancelRequested.store(false);
    m_state.store(BuildProcessState::Running);
    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        m_liveOutput.clear();
    }
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
    startup.dwFlags = STARTF_USESTDHANDLES;
    if (!showChildWindows) {
        startup.dwFlags |= STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_HIDE;
    }
    startup.hStdOutput = stdoutWrite;
    startup.hStdError = stderrWrite;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process{};
    std::string commandLine = command;
    const DWORD creationFlags = showChildWindows ? 0 : CREATE_NO_WINDOW;
    const BOOL started = CreateProcessA(resolvedExecutable.string().c_str(), commandLine.data(), nullptr, nullptr, TRUE, creationFlags, nullptr, workingDirectory.string().c_str(),
                                        &startup, &process);
    CloseHandle(stdoutWrite);
    CloseHandle(stderrWrite);
    if (!started) {
        CloseHandle(stdoutRead);
        CloseHandle(stderrRead);
        result.output = "failed to start process, error=" + std::to_string(GetLastError());
        m_state.store(BuildProcessState::Failed);
        return result;
    }
    HANDLE job = CreateJobObjectA(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
        AssignProcessToJobObject(job, process.hProcess);
    }
    const auto reader = [this](HANDLE pipe, bool stderrStream, std::string& output) {
        char buffer[1024];
        DWORD read = 0;
        while (ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
            output.append(buffer, buffer + read);
            pushOutput(stderrStream, buffer, read);
        }
    };
    std::thread stdoutThread(reader, stdoutRead, false, std::ref(result.stdoutText));
    std::thread stderrThread(reader, stderrRead, true, std::ref(result.stderrText));
    while (WaitForSingleObject(process.hProcess, 20) == WAIT_TIMEOUT) {
        if (m_cancelRequested.load()) {
            result.cancelled = true;
            if (job)
                TerminateJobObject(job, ERROR_CANCELLED);
            else
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
    if (job)
        CloseHandle(job);
    stdoutThread.join();
    stderrThread.join();
    CloseHandle(stdoutRead);
    CloseHandle(stderrRead);
    result.output = result.stdoutText + result.stderrText;
    parseDiagnostics(result);
    result.success = !result.cancelled && result.exitCode == 0;
    m_state.store(result.cancelled ? BuildProcessState::Cancelled : (result.success ? BuildProcessState::Succeeded : BuildProcessState::Failed));
    return result;
}

BuildTaskResult BuildQueue::configure(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& generator) const {
    std::vector<std::string> arguments{
        "-S",
        projectRoot.string(),
        "-B",
        buildRoot.string(),
    };
    if (!std::filesystem::exists(buildRoot / "CMakeCache.txt") && !generator.empty()) {
        arguments.emplace_back("-G");
        arguments.push_back(generator);
    }
    return run(BuildTaskKind::Configure, "cmake.exe", arguments, projectRoot);
}

BuildTaskResult BuildQueue::build(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target, const std::string& generator) const {
    auto configureResult = configure(projectRoot, buildRoot, generator);
    configureResult.kind = BuildTaskKind::Build;
    if (!configureResult.success || configureResult.cancelled)
        return configureResult;
    const std::vector<std::string> arguments{
        "--build",
        buildRoot.string(),
        "--target",
        target,
    };
    auto result = run(BuildTaskKind::Build, "cmake.exe", arguments, projectRoot);
    result.stdoutText = configureResult.stdoutText + result.stdoutText;
    result.stderrText = configureResult.stderrText + result.stderrText;
    result.output = result.stdoutText + result.stderrText;
    result.diagnostics.insert(result.diagnostics.begin(), configureResult.diagnostics.begin(), configureResult.diagnostics.end());
    return result;
}

BuildTaskResult BuildQueue::buildAndRun(const std::filesystem::path& projectRoot, const std::filesystem::path& buildRoot, const std::string& target, const std::string& generator,
                                        const std::vector<std::string>& arguments) const {
    auto result = build(projectRoot, buildRoot, target, generator);
    result.kind = BuildTaskKind::BuildAndRun;
    if (!result.success || result.cancelled)
        return result;
    const auto executable = findExecutable(buildRoot, target);
    auto runResult = runTarget(executable, executable.parent_path(), arguments);
    result.success = runResult.success;
    result.exitCode = runResult.exitCode;
    result.stdoutText += runResult.stdoutText;
    result.stderrText += runResult.stderrText;
    result.output = result.stdoutText + result.stderrText;
    result.cancelled = runResult.cancelled;
    return result;
}

BuildTaskResult BuildQueue::runTarget(const std::filesystem::path& executable, const std::filesystem::path& workingDirectory, const std::vector<std::string>& arguments) const {
    return run(BuildTaskKind::Run, executable, arguments, workingDirectory, true);
}

void BuildQueue::cancel() const {
    m_cancelRequested.store(true);
}

BuildProcessState BuildQueue::state() const {
    return m_state.load();
}

}  // namespace morrow::editor
