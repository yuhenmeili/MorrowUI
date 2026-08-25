#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "build/BuildQueue.h"

using namespace morrow::editor;

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAILED] " << message << '\n';
        ++g_failures;
    }
}

}  // namespace

int main() {
    const auto testRoot = std::filesystem::temp_directory_path() /
                          "Morrow BuildQueue Argument Test";
    const auto sourceRoot = testRoot / "Source With Spaces";
    const auto buildRoot = testRoot / "Build With Spaces";
    std::error_code error;
    std::filesystem::remove_all(testRoot, error);
    std::filesystem::create_directories(sourceRoot, error);
    {
        std::ofstream project(sourceRoot / "CMakeLists.txt");
        project << "cmake_minimum_required(VERSION 3.20)\n"
                   "project(BuildQueueArgumentTest LANGUAGES CXX)\n"
                   "add_executable(ArgumentEcho main.cpp)\n";
    }
    {
        std::ofstream source(sourceRoot / "main.cpp");
        source << "#include <string>\n"
                  "int main(int argc, char** argv) {\n"
                  "    return argc == 3 &&\n"
                  "                   std::string(argv[1]) == \"first value\" &&\n"
                  "                   std::string(argv[2]) == \"second value\"\n"
                  "               ? 0\n"
                  "               : 7;\n"
                  "}\n";
    }

    BuildQueue queue;
    const auto result =
        queue.buildAndRun(
            sourceRoot, buildRoot, "ArgumentEcho", "MinGW Makefiles",
            {"first value", "second value"});

    expect(
        result.success,
        "BuildQueue::buildAndRun should preserve path and runtime arguments: " +
            result.output);
    expect(
        std::filesystem::exists(buildRoot / "CMakeCache.txt"),
        "BuildQueue::configure should create the CMake cache");

    std::filesystem::remove_all(testRoot, error);
    if (g_failures != 0) {
        std::cerr << g_failures << " BuildQueue test(s) failed\n";
        return 1;
    }
    std::cout << "All BuildQueue tests passed\n";
    return 0;
}
