if(NOT DEFINED SHADER_DIR OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "SHADER_DIR and OUTPUT_DIR must be specified")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

file(GLOB SHADER_FILES
    LIST_DIRECTORIES false
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.frag"
)
list(SORT SHADER_FILES)

set(GENERATED_HEADER
"#pragma once

#include <string>

namespace morrow::embedded_shaders {

// Returns true when the requested shader was compiled into the library.
bool get(const std::string& fileName, std::string& source);

} // namespace morrow::embedded_shaders
")

set(GENERATED_SOURCE
"#include \"EmbeddedShaders.h\"

namespace morrow::embedded_shaders {
namespace {

struct ShaderResource {
    const char* fileName;
    const char* source;
};

")

foreach(SHADER_FILE IN LISTS SHADER_FILES)
    get_filename_component(SHADER_NAME "${SHADER_FILE}" NAME)
    string(MAKE_C_IDENTIFIER "${SHADER_NAME}" SHADER_IDENTIFIER)
    file(READ "${SHADER_FILE}" SHADER_CONTENT)

    # Convert the text into a C++ string literal while preserving line breaks
    # so shader compiler diagnostics still have useful line numbers.
    string(REPLACE "\\" "\\\\" SHADER_CONTENT "${SHADER_CONTENT}")
    string(REPLACE "\"" "\\\"" SHADER_CONTENT "${SHADER_CONTENT}")
    string(REPLACE "\r" "" SHADER_CONTENT "${SHADER_CONTENT}")
    string(REPLACE "\n" "\\n\"\n        \"" SHADER_CONTENT "${SHADER_CONTENT}")

    string(APPEND GENERATED_SOURCE
"const char kShader_${SHADER_IDENTIFIER}[] =
    \"${SHADER_CONTENT}\";

")
endforeach()

string(APPEND GENERATED_SOURCE
"const ShaderResource kShaderResources[] = {
")

foreach(SHADER_FILE IN LISTS SHADER_FILES)
    get_filename_component(SHADER_NAME "${SHADER_FILE}" NAME)
    string(MAKE_C_IDENTIFIER "${SHADER_NAME}" SHADER_IDENTIFIER)
    string(APPEND GENERATED_SOURCE
"    {\"${SHADER_NAME}\", kShader_${SHADER_IDENTIFIER}},
")
endforeach()

string(APPEND GENERATED_SOURCE
"};

} // namespace

bool get(const std::string& fileName, std::string& source) {
    for (const auto& resource : kShaderResources) {
        if (fileName == resource.fileName) {
            source.assign(resource.source);
            return true;
        }
    }
    return false;
}

} // namespace morrow::embedded_shaders
")

file(WRITE "${OUTPUT_DIR}/EmbeddedShaders.h" "${GENERATED_HEADER}")
file(WRITE "${OUTPUT_DIR}/EmbeddedShaders.cpp" "${GENERATED_SOURCE}")
