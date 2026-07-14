add_subdirectory(utils)
add_subdirectory(math)

set(MORROW_LIB_NAME "morrow")

set(MORROW_SHADER_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
set(MORROW_SHADER_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(GLOB MORROW_SHADER_FILES
    ${MORROW_SHADER_DIR}/*.vert
    ${MORROW_SHADER_DIR}/*.frag
)

add_custom_command(
    OUTPUT
    ${MORROW_SHADER_GENERATED_DIR}/EmbeddedShaders.h
    ${MORROW_SHADER_GENERATED_DIR}/EmbeddedShaders.cpp
    COMMAND ${CMAKE_COMMAND}
    -DSHADER_DIR=${MORROW_SHADER_DIR}
    -DOUTPUT_DIR=${MORROW_SHADER_GENERATED_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/EmbedShaders.cmake
    DEPENDS
    ${MORROW_SHADER_FILES}
    ${CMAKE_SOURCE_DIR}/cmake/EmbedShaders.cmake
    VERBATIM
)

add_custom_target(morrow_generate_embedded_shaders
    DEPENDS
    ${MORROW_SHADER_GENERATED_DIR}/EmbeddedShaders.h
    ${MORROW_SHADER_GENERATED_DIR}/EmbeddedShaders.cpp
)

# 分别收集各个目录的源文件
file(GLOB_RECURSE CORE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
file(GLOB_RECURSE EXTERN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/extern/*.cpp)
file(GLOB_RECURSE UI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp)
file(GLOB_RECURSE RENDERER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/renderer/*.cpp)
file(GLOB_RECURSE FONTS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.cpp)
file(GLOB_RECURSE DEBUG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/debug/*.cpp)
file(GLOB_RECURSE GLTF_SRC ${CMAKE_CURRENT_SOURCE_DIR}/gltf/*.cpp)
set(PLATFORM_SRC
    platform/PlatformFactory.cpp
    platform/Platform.cpp
    platform/Window.cpp
    platform/InputEventsManager.cpp
    platform/utils/OpenglUtils.cpp)

set(PLATFORM_INCLUDE_DIRS)
set(PLATFORM_LIBS)
set(MORROW_PUBLIC_DEFINES)
set(MORROW_IS_QNX FALSE)

if(NOT DEFINED MORROW_PLATFORM)
    if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
        set(MORROW_PLATFORM "QNX")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(MORROW_PLATFORM "Linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(MORROW_PLATFORM "Windows")
    else()
        set(MORROW_PLATFORM "Unknown")
    endif()
endif()

if(MORROW_PLATFORM STREQUAL "QNX")
    set(MORROW_IS_QNX TRUE)
endif()

if(MORROW_PLATFORM STREQUAL "QNX")
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "QCC")
        string(FIND "${CMAKE_CXX_COMPILER}" "q++" MORROW_QPP_POS)
        string(FIND "${CMAKE_CXX_COMPILER}" "qcc" MORROW_QCC_POS)
        string(FIND "${CMAKE_CXX_COMPILER}" "nto" MORROW_NTO_POS)
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "QCC" AND MORROW_QPP_POS EQUAL -1 AND MORROW_QCC_POS EQUAL -1 AND MORROW_NTO_POS EQUAL -1)
        message(FATAL_ERROR
            "QNX build selected, but CMAKE_CXX_COMPILER='${CMAKE_CXX_COMPILER}' is not a QNX compiler. "
            "Reconfigure with the QNX toolchain or run CMake from a shell where q++/qcc is selected.")
    endif()

    message(STATUS "Building for QNX platform")
    if(DEFINED ENV{QNX_TARGET})
        list(APPEND PLATFORM_INCLUDE_DIRS $ENV{QNX_TARGET}/usr/include)
    endif()
    file(GLOB_RECURSE EGL_SOURCES "platform/egl/*.cpp")
    list(APPEND PLATFORM_SRC ${EGL_SOURCES})
    list(APPEND PLATFORM_LIBS screen EGL GLESv2 m)

    list(APPEND MORROW_PUBLIC_DEFINES OPENGL_EGL QNX)
elseif(MORROW_PLATFORM STREQUAL "Linux")
    message(STATUS "Building for Linux platform")
    file(GLOB_RECURSE WGL_SOURCES "platform/wgl/*.cpp" "platform/wgl/*.c")
    list(APPEND PLATFORM_SRC ${WGL_SOURCES})
    list(APPEND PLATFORM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/includes/opengl/wgl)

    list(APPEND MORROW_PUBLIC_DEFINES OPENGL_GLFW)
    set(MORROW_HAS_GLFW FALSE)

    find_package(glfw3 QUIET)
    if(TARGET glfw)
        set(GLFW_LIBRARY glfw)
        set(MORROW_HAS_GLFW TRUE)
    elseif(TARGET glfw3)
        set(GLFW_LIBRARY glfw3)
        set(MORROW_HAS_GLFW TRUE)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(GLFW3 QUIET IMPORTED_TARGET glfw3)
        endif()

        if(TARGET PkgConfig::GLFW3)
            set(GLFW_LIBRARY PkgConfig::GLFW3)
            set(MORROW_HAS_GLFW TRUE)
        else()
            find_library(GLFW_LIBRARY NAMES glfw3 glfw)
            if(GLFW_LIBRARY)
                set(MORROW_HAS_GLFW TRUE)
            endif()
        endif()
    endif()

    if(NOT MORROW_HAS_GLFW)
        message(FATAL_ERROR "GLFW library not found for Linux desktop build. Install glfw3 dev package (e.g. 'sudo apt install libglfw3-dev') or provide a CMake package for glfw3.")
    endif()

    # Linux 下补充常见运行时依赖，避免静态/非导入 glfw 场景链接缺符号。
    find_package(Threads QUIET)
    if(TARGET Threads::Threads)
        list(APPEND PLATFORM_LIBS Threads::Threads)
    endif()
    list(APPEND PLATFORM_LIBS ${CMAKE_DL_LIBS})

    find_package(OpenGL QUIET)
    if(TARGET OpenGL::GL)
        list(APPEND PLATFORM_LIBS OpenGL::GL)
    endif()

    message(STATUS "Found GLFW: ${GLFW_LIBRARY}")
    list(APPEND PLATFORM_LIBS ${GLFW_LIBRARY})
elseif(MORROW_PLATFORM STREQUAL "Windows")
    message(STATUS "Building for Windows platform")
    file(GLOB_RECURSE WGL_SOURCES "platform/wgl/*.cpp" "platform/wgl/*.c")
    list(APPEND PLATFORM_SRC ${WGL_SOURCES})
    list(APPEND PLATFORM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/includes/opengl/wgl)

    list(APPEND MORROW_PUBLIC_DEFINES OPENGL_GLFW)

    find_library(GLFW_LIBRARY
        NAMES glfw3 glfw3dll glfw
        PATHS ${CMAKE_SOURCE_DIR}/libs/GLFW
        PATH_SUFFIXES lib lib64
        NO_DEFAULT_PATH
    )

    if(NOT GLFW_LIBRARY)
        message(FATAL_ERROR "GLFW library not found for Windows build in ${CMAKE_SOURCE_DIR}/libs/GLFW")
    endif()

    message(STATUS "Found GLFW: ${GLFW_LIBRARY}")
    list(APPEND PLATFORM_LIBS ${GLFW_LIBRARY})
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}. Supported: QNX, Linux, Windows")
endif()

set(MORROW_SRC
    ${CORE_SRC}
    ${EXTERN_SRC}
    ${UI_SRC}
    ${RENDERER_SRC}
    ${PLATFORM_SRC}
    ${FONTS_SRC}
    ${DEBUG_SRC}
    ${GLTF_SRC}
    ${MORROW_SHADER_GENERATED_DIR}/EmbeddedShaders.cpp
)

add_library(${MORROW_LIB_NAME} STATIC ${MORROW_SRC})
add_dependencies(${MORROW_LIB_NAME} morrow_generate_embedded_shaders)
target_compile_definitions(${MORROW_LIB_NAME} PUBLIC ${MORROW_PUBLIC_DEFINES})
if(MORROW_PLATFORM STREQUAL "Windows")
    target_link_directories(${MORROW_LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/libs/GLFW)
endif()

target_link_libraries(${MORROW_LIB_NAME}
    morrow_math
    morrow_utils
    ${PLATFORM_LIBS})

target_include_directories(${MORROW_LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/common
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/debug
    ${CMAKE_CURRENT_SOURCE_DIR}/gltf
    ${MORROW_SHADER_GENERATED_DIR}
    ${PLATFORM_INCLUDE_DIRS}
)