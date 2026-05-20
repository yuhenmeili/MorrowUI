# qnx.toolchain.cmake
set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 8.0.4)

# ===== QNX SDK 路径配置 =====
# 你提供的 SDK 根目录
set(QNX_SDK_ROOT "/home/lance/qnx800" CACHE PATH "QNX SDK root path")

# 按 QNX SDP 目录结构推导
set(QNX_HOST   "${QNX_SDK_ROOT}/host/linux/x86_64")
set(QNX_TARGET "${QNX_SDK_ROOT}/target/qnx")

# 如已导出环境变量，优先使用环境变量（便于 CI 或不同机器复用）
if(DEFINED ENV{QNX_HOST} AND NOT "$ENV{QNX_HOST}" STREQUAL "")
    set(QNX_HOST "$ENV{QNX_HOST}")
endif()

if(DEFINED ENV{QNX_TARGET} AND NOT "$ENV{QNX_TARGET}" STREQUAL "")
    set(QNX_TARGET "$ENV{QNX_TARGET}")
endif()

# 基本有效性检查
if(NOT EXISTS "${QNX_HOST}/usr/bin/qcc")
    message(FATAL_ERROR "qcc not found: ${QNX_HOST}/usr/bin/qcc")
endif()

if(NOT EXISTS "${QNX_TARGET}")
    message(FATAL_ERROR "QNX_TARGET not found: ${QNX_TARGET}")
endif()

# qcc requires these environment variables during compile/link checks.
set(ENV{QNX_HOST} "${QNX_HOST}")
set(ENV{QNX_TARGET} "${QNX_TARGET}")
set(ENV{MAKEFLAGS} "-I${QNX_TARGET}/usr/include")
if(NOT "$ENV{PATH}" MATCHES "${QNX_HOST}/usr/bin")
    set(ENV{PATH} "${QNX_HOST}/usr/bin:$ENV{PATH}")
endif()

# ===== 编译器 =====
set(CMAKE_C_COMPILER   "/home/lance/workspace/morrow.gui/qnx/qcc-wrapper.sh")
set(CMAKE_CXX_COMPILER "/home/lance/workspace/morrow.gui/qnx/qpp-wrapper.sh")

# 目标架构（可改成 x86_64）
set(QNX_ARCH "aarch64le" CACHE STRING "QNX target arch (aarch64le/x86_64)")
set(CMAKE_SYSTEM_PROCESSOR "${QNX_ARCH}")

# qcc target profile
set(QNX_QCC_TARGET "gcc_nto${QNX_ARCH}" CACHE STRING "qcc -V target profile")

# 使用 _INIT 避免覆盖用户在命令行传入的 CMAKE_C_FLAGS/CMAKE_CXX_FLAGS
set(CMAKE_C_FLAGS_INIT   "-V${QNX_QCC_TARGET}")
set(CMAKE_CXX_FLAGS_INIT "-V${QNX_QCC_TARGET}")

# ===== 查找策略 =====
set(CMAKE_FIND_ROOT_PATH "${QNX_TARGET}")

# 程序从主机找；库/头/包从目标 sysroot 找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 可选：避免 try_compile 在某些交叉场景下链接失败
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)