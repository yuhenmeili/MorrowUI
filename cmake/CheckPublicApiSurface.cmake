# M3 门禁（第二轮）：入口白名单 + 可达性。
# 解析范式与 CheckPublicApiGLFree.cmake 一致（file(STRINGS) + REGEX MATCH）。
#
# 用法：cmake -DAPI_SURFACE_DIR=<include/morrow> -DENTRIES_FILE=<morrow_public_entries.cmake> -P CheckPublicApiSurface.cmake

include(${ENTRIES_FILE})

if(NOT DEFINED API_SURFACE_DIR)
    message(FATAL_ERROR "CheckPublicApiSurface: API_SURFACE_DIR 必须提供")
endif()

file(GLOB_RECURSE _all_files "${API_SURFACE_DIR}/*.h")
set(_all_rel "")
foreach(_f IN LISTS _all_files)
    file(RELATIVE_PATH _r "${API_SURFACE_DIR}" "${_f}")
    list(APPEND _all_rel "${_r}")
endforeach()

# 闭包：从入口沿 morrow/ 引用 BFS（与 GL 门禁同一 idiom）
set(_queue "${MORROW_PUBLIC_ENTRIES}")
set(_reach "${MORROW_PUBLIC_ENTRIES}")

while(_queue)
    list(GET _queue 0 _cur)
    list(REMOVE_AT _queue 0)

    if(NOT EXISTS "${API_SURFACE_DIR}/${_cur}")
        continue()
    endif()

    file(STRINGS "${API_SURFACE_DIR}/${_cur}" _lines)
    foreach(_line IN LISTS _lines)
        string(REGEX MATCH "^[ 	]*#[ 	]*include[ 	]*\"morrow/([^\"]+)\"" _m "${_line}")
        if(NOT _m)
            continue()
        endif()
        set(_inc "${CMAKE_MATCH_1}")
        if(NOT _inc IN_LIST _reach)
            list(APPEND _reach "${_inc}")
            list(APPEND _queue "${_inc}")
        endif()
    endforeach()
endwhile()

# 孤儿检查
set(_missing "")
foreach(_r IN LISTS _all_rel)
    if(NOT _r IN_LIST _reach)
        list(APPEND _missing "${_r}")
    endif()
endforeach()

list(LENGTH _all_rel _total)
list(LENGTH _reach _reach_n)
if(_missing)
    string(REPLACE ";" "
  " _missing_str "${_missing}")
    message(FATAL_ERROR
        "M3 gate FAILED: ${_total} 个公共头中以下不在入口白名单可达闭包内（孤儿滞留公共面）：
  ${_missing_str}
要么从 include/morrow 迁回 src/，要么把入口加入 cmake/morrow_public_entries.cmake（评审动作）。")
endif()
message(STATUS "CheckPublicApiSurface: OK — ${_total} 头全部 ∈ 入口白名单 ∪ 可达闭包")
