# M1 门禁：公共头传递闭包必须零 GLES/EGL/GL/GLFW 词汇。
#
# 从 API_ROOT_HEADERS 出发，沿 `#include "..."` 递归收集闭包（仅在 API_SOURCE_DIR
# 的头文件内解析，系统包含与外部文件忽略），逐行扫描禁用词汇。命中即报错；
# API_GATE_FATAL=OFF 时仅报告（用于记录基线）。
#
# 用法（-P 脚本模式）：
#   cmake -DAPI_SOURCE_DIR=<src 绝对路径>
#         -DAPI_ROOT_HEADERS="src/core/Engine.h;..."
#         -DAPI_GATE_FATAL=ON
#         -P cmake/CheckPublicApiGLFree.cmake
#
# 禁用词汇分三类：
#   1. GL 原生类型：GLuint/GLint/GLenum/GLsizei/GLbitfield/GLfloat/GLboolean/GLclampf
#   2. GL 常量与函数：GL_XXX、glXxx(...)
#   3. 窗口系统/图形 API 头：GLES、EGL、GLFW、glad、screen.h 及引擎的 GL 头 shim

if(NOT DEFINED API_GATE_FATAL)
    set(API_GATE_FATAL ON)
endif()

# API_PRINT_CLOSURE=ON 时仅打印闭包文件清单（相对 API_SOURCE_DIR），供迁移与
# M3/M5 门禁使用，不做词汇扫描。
set(_print_closure OFF)
if(DEFINED API_PRINT_CLOSURE AND API_PRINT_CLOSURE)
    set(_print_closure ON)
endif()

if(NOT DEFINED API_SOURCE_DIR OR NOT DEFINED API_ROOT_HEADERS)
    message(FATAL_ERROR "CheckPublicApiGLFree: API_SOURCE_DIR 与 API_ROOT_HEADERS 必须提供")
endif()

# ---------------------------------------------------------------------------
# 1. 建立文件名 -> 绝对路径索引（用于解析带引号的 include）
# ---------------------------------------------------------------------------
file(GLOB_RECURSE _all_headers "${API_SOURCE_DIR}/*.h")
set(_header_index "")
foreach(_h IN LISTS _all_headers)
    get_filename_component(_name "${_h}" NAME)
    list(APPEND _header_index "${_name}|${_h}")
endforeach()

function(_resolve_include _include_name _includer_file _out_var)
    # 相对 includer 目录优先
    get_filename_component(_dir "${_includer_file}" DIRECTORY)
    if(EXISTS "${_dir}/${_include_name}")
        set(${_out_var} "${_dir}/${_include_name}" PARENT_SCOPE)
        return()
    endif()
    foreach(_entry IN LISTS _header_index)
        string(FIND "${_entry}" "|" _sep)
        string(SUBSTRING "${_entry}" 0 ${_sep} _name)
        if(_name STREQUAL _include_name)
            math(EXPR _path_start "${_sep} + 1")
            string(SUBSTRING "${_entry}" ${_path_start} -1 _path)
            set(${_out_var} "${_path}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${_out_var} "" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# 2. BFS 收集传递闭包
# ---------------------------------------------------------------------------
set(_queue "${API_ROOT_HEADERS}")
set(_closure "${API_ROOT_HEADERS}")

while(_queue)
    list(GET _queue 0 _current)
    list(REMOVE_AT _queue 0)

    if(NOT EXISTS "${_current}")
        continue()
    endif()

    file(STRINGS "${_current}" _lines)

    foreach(_line IN LISTS _lines)
        string(REGEX MATCH "^[ \t]*#[ \t]*include[ \t]*\"([^\"]+)\"" _m "${_line}")
        if(NOT _m)
            continue()
        endif()
        set(_inc "${CMAKE_MATCH_1}")
        get_filename_component(_inc_name "${_inc}" NAME)
        _resolve_include("${_inc_name}" "${_current}" _resolved)
        if(_resolved AND NOT _resolved IN_LIST _closure)
            list(APPEND _closure "${_resolved}")
            list(APPEND _queue "${_resolved}")
        endif()
    endforeach()
endwhile()

# ---------------------------------------------------------------------------
# 3. 逐行扫描禁用词汇
# ---------------------------------------------------------------------------
set(_violations "")

foreach(_file IN LISTS _closure)
    file(STRINGS "${_file}" _lines)
    list(LENGTH _lines _line_count)
    set(_idx 0)
    foreach(_line IN LISTS _lines)
        math(EXPR _idx "${_idx} + 1")

        # 3.1 禁用的 include 目标
        string(REGEX MATCH "^[ \t]*#[ \t]*include[ \t]*[<\"]" _is_include "${_line}")
        if(_is_include)
            foreach(_bad_inc
                    "GLES" "EGL/" "egl\\.h" "eglext" "glad" "glfw" "GLFW"
                    "screen\\.h" "GLESHeader" "EGLHeader" "OpenglHeader"
                    "ESContext" "khrplatform")
                string(REGEX MATCH "${_bad_inc}" _hit "${_line}")
                if(_hit)
                    string(APPEND _violations
                        "${_file}:${_idx}: 禁用的 GL/EGL/GLFW 头包含 -> ${_line}\n")
                    break()
                endif()
            endforeach()
        endif()

        # 3.2 GL 原生类型（带词边界：前置字符类模拟 \b）
        foreach(_tok "GLuint" "GLint" "GLenum" "GLsizei" "GLbitfield"
                     "GLfloat" "GLboolean" "GLclampf")
            string(REGEX MATCH "(^|[^A-Za-z0-9_])${_tok}([^A-Za-z0-9_]|$)" _hit "${_line}")
            if(_hit)
                string(APPEND _violations
                    "${_file}:${_idx}: GL 原生类型 -> ${_line}\n")
                break()
            endif()
        endforeach()

        # 3.3 GL_ 常量 / glXxx 函数 / EGL 与 GLFW 词汇
        foreach(_pat "[^A-Za-z0-9_]GL_[A-Z0-9_]+"
                     "(^|[^A-Za-z0-9_])gl[A-Z][A-Za-z0-9]*"
                     "[^A-Za-z0-9_]EGL[A-Z]"
                     "[^A-Za-z0-9_]GLFW[A-Za-z]*")
            string(REGEX MATCH "${_pat}" _hit "${_line}")
            if(_hit)
                string(APPEND _violations
                    "${_file}:${_idx}: GL/EGL/GLFW 词汇 -> ${_line}\n")
                break()
            endif()
        endforeach()
    endforeach()
endforeach()

list(LENGTH _closure _closure_size)

if(_print_closure)
    foreach(_file IN LISTS _closure)
        file(RELATIVE_PATH _rel "${API_SOURCE_DIR}" "${_file}")
        message("${_rel}")
    endforeach()
    return()
endif()

if(_violations STREQUAL "")
    message(STATUS "CheckPublicApiGLFree: OK — 闭包 ${_closure_size} 个头文件，零 GL/EGL/GLFW 词汇")
    return()
endif()

if(API_GATE_FATAL)
    message(FATAL_ERROR
        "CheckPublicApiGLFree: 公共头闭包存在 ${_closure_size} 个头文件中的 GL/EGL/GLFW 泄漏：\n${_violations}")
else()
    message(STATUS "CheckPublicApiGLFree（报告模式）: 闭包 ${_closure_size} 个头文件，泄漏如下：\n${_violations}")
endif()
