cmake_minimum_required(VERSION 3.10)
project(morrow.gui)


if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    set(MORROW_PLATFORM "QNX")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(MORROW_PLATFORM "Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(MORROW_PLATFORM "Windows")
else()
    set(MORROW_PLATFORM "Unknown")
endif()

set(MORROW_PLATFORM_IS_QNX FALSE)
set(MORROW_PLATFORM_IS_LINUX FALSE)
set(MORROW_PLATFORM_IS_WINDOWS FALSE)
if(MORROW_PLATFORM STREQUAL "QNX")
    set(MORROW_PLATFORM_IS_QNX TRUE)
elseif(MORROW_PLATFORM STREQUAL "Linux")
    set(MORROW_PLATFORM_IS_LINUX TRUE)
elseif(MORROW_PLATFORM STREQUAL "Windows")
    set(MORROW_PLATFORM_IS_WINDOWS TRUE)
endif()

message(STATUS "Configuring morrow.gui for platform: ${MORROW_PLATFORM} (system='${CMAKE_SYSTEM_NAME}', processor='${CMAKE_SYSTEM_PROCESSOR}')")

# ---------------------------------------------------------------------------
# Build type / optimization
#
# Historically every non-MSVC build (including QNX) unconditionally appended
# "-g -O0", so production binaries shipped unoptimized with full debug
# symbols. Optimization flags are now selected by CMAKE_BUILD_TYPE:
#   Debug            -> -g -O0 (unchanged developer default)
#   Release          -> CMake's -O3 -DNDEBUG + section splitting + --gc-sections
#   RelWithDebInfo   -> -O2 -g -DNDEBUG + section splitting + --gc-sections
#   MinSizeRel       -> -Os -DNDEBUG + section splitting + --gc-sections
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(MORROW_IS_RELEASE_LIKE FALSE)
if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(MORROW_IS_RELEASE_LIKE TRUE)
endif()

option(MORROW_ENABLE_LTO "Enable link-time optimization (-flto) in release-like builds" OFF)
option(MORROW_ENABLE_STRIP "Strip symbols from release-like executables at link (-Wl,-s)" OFF)

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /GR /std:c++17")
elseif(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(APPEND CMAKE_C_FLAGS " -Wno-error")
    string(APPEND CMAKE_CXX_FLAGS " -std=c++17")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        string(APPEND CMAKE_C_FLAGS " -g -O0")
    endif()
else()
    # GCC / MinGW / qcc：通用 ABI 与标准选项。
    string(APPEND CMAKE_C_FLAGS " -fPIC -fpermissive -Wno-error")
    string(APPEND CMAKE_CXX_FLAGS " -fPIC -frtti -std=gnu++17 -D_GLIBCXX_USE_C99=1 -fpermissive")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fPIE")
    if(NOT MORROW_IS_RELEASE_LIKE)
        string(APPEND CMAKE_C_FLAGS " -g -O0")
    else()
        # 优化等级交给 CMake 内建的 per-config flags
        # （Release=-O3 / RelWithDebInfo=-O2 / MinSizeRel=-Os，均带 -DNDEBUG）。
        string(APPEND CMAKE_C_FLAGS " -ffunction-sections -fdata-sections")
        string(APPEND CMAKE_CXX_FLAGS " -ffunction-sections -fdata-sections -fvisibility=hidden")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--gc-sections")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--gc-sections")
        if(MORROW_ENABLE_LTO)
            string(APPEND CMAKE_C_FLAGS " -flto")
            string(APPEND CMAKE_CXX_FLAGS " -flto")
            string(APPEND CMAKE_EXE_LINKER_FLAGS " -flto")
            string(APPEND CMAKE_SHARED_LINKER_FLAGS " -flto")
        endif()
        if(MORROW_ENABLE_STRIP)
            string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,-s")
        endif()
    endif()
endif()

if(MORROW_IS_RELEASE_LIKE)
    message(STATUS "Release-like build: ${CMAKE_BUILD_TYPE} "
                   "(gc-sections=ON, lto=${MORROW_ENABLE_LTO}, strip=${MORROW_ENABLE_STRIP})")
endif()

if(MORROW_PLATFORM_IS_QNX)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64le")
        set(MORROW_QNX_VARIANT_FLAG "-Vgcc_ntoaarch64le")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(MORROW_QNX_VARIANT_FLAG "-Vgcc_ntox86_64")
    endif()

    if(DEFINED MORROW_QNX_VARIANT_FLAG)
        string(APPEND CMAKE_C_FLAGS " ${MORROW_QNX_VARIANT_FLAG}")
        string(APPEND CMAKE_CXX_FLAGS " ${MORROW_QNX_VARIANT_FLAG}")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " ${MORROW_QNX_VARIANT_FLAG}")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${MORROW_QNX_VARIANT_FLAG}")
        message(STATUS "Using QNX compiler variant flag: ${MORROW_QNX_VARIANT_FLAG}")
    else()
        message(WARNING "Unknown QNX processor '${CMAKE_SYSTEM_PROCESSOR}', q++ may fall back to its default architecture")
    endif()
endif()


add_subdirectory(src)
if(MORROW_PLATFORM_IS_WINDOWS)
    add_subdirectory(editor)
endif()

file(GLOB DEMO_LIST ${CMAKE_CURRENT_SOURCE_DIR}/samples/*.cpp)

# 资产拷贝收敛为单一自定义目标：此前每个 demo 的 POST_BUILD 各自
# copy_directory 到同一目录，并行构建时互相竞争导致偶发拷贝失败。
# 自定义目标在一次构建中只执行一次，依赖它的 demo 自动串行等待。
add_custom_target(morrow_copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying assets to ${CMAKE_BINARY_DIR}/assets"
)

foreach(FILE_PATH ${DEMO_LIST})
    get_filename_component(FILE_NAME ${FILE_PATH} NAME_WE)
    add_executable(${FILE_NAME} ${FILE_PATH})
    target_link_libraries(${FILE_NAME} morrow)
    add_dependencies(${FILE_NAME} morrow_copy_assets)
endforeach()

