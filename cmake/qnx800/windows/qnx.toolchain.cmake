set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION "8.0.4")
set(MORROW_QNX_SDK_LABEL "QNX SDP 8.0")
set(MORROW_QNX_PRIMARY_SDK_ROOT "D:/WorkTools/QNX/qnx800")
set(MORROW_QNX_TARGET_SUBDIR "target/qnx")
set(MORROW_QNX_SEARCH_NAMES "qnx800")

function(_morrow_append_unique_candidate list_var candidate)
    if(NOT candidate OR candidate STREQUAL "")
        return()
    endif()

    file(TO_CMAKE_PATH "${candidate}" _candidate)
    list(FIND ${list_var} "${_candidate}" _candidate_index)
    if(_candidate_index EQUAL -1)
        list(APPEND ${list_var} "${_candidate}")
        set(${list_var} "${${list_var}}" PARENT_SCOPE)
    endif()
endfunction()

function(_morrow_try_register_sdk_root list_var candidate)
    if(NOT candidate OR candidate STREQUAL "")
        return()
    endif()

    file(TO_CMAKE_PATH "${candidate}" _candidate)
    if(EXISTS "${_candidate}/qnxsdp-env.bat")
        _morrow_append_unique_candidate(${list_var} "${_candidate}")
        set(${list_var} "${${list_var}}" PARENT_SCOPE)
    endif()
endfunction()

function(_morrow_try_register_from_qnx_host list_var host_path)
    if(NOT host_path OR host_path STREQUAL "")
        return()
    endif()

    file(TO_CMAKE_PATH "${host_path}" _host_path)
    get_filename_component(_sdk_root "${_host_path}" DIRECTORY)
    get_filename_component(_sdk_root "${_sdk_root}" DIRECTORY)
    get_filename_component(_sdk_root "${_sdk_root}" DIRECTORY)
    _morrow_try_register_sdk_root(${list_var} "${_sdk_root}")
    set(${list_var} "${${list_var}}" PARENT_SCOPE)
endfunction()

function(_morrow_try_register_from_qnx_target list_var target_path)
    if(NOT target_path OR target_path STREQUAL "")
        return()
    endif()

    file(TO_CMAKE_PATH "${target_path}" _target_path)
    get_filename_component(_sdk_root "${_target_path}" DIRECTORY)
    get_filename_component(_sdk_root "${_sdk_root}" DIRECTORY)
    _morrow_try_register_sdk_root(${list_var} "${_sdk_root}")
    set(${list_var} "${${list_var}}" PARENT_SCOPE)
endfunction()

set(_morrow_qnx_sdk_candidates)
if(DEFINED QNX_SDK_ROOT AND NOT QNX_SDK_ROOT STREQUAL "")
    _morrow_try_register_sdk_root(_morrow_qnx_sdk_candidates "${QNX_SDK_ROOT}")
endif()
if(DEFINED ENV{QNX_SDK_ROOT} AND NOT "$ENV{QNX_SDK_ROOT}" STREQUAL "")
    _morrow_try_register_sdk_root(_morrow_qnx_sdk_candidates "$ENV{QNX_SDK_ROOT}")
endif()
if(DEFINED ENV{QNX_BASE} AND NOT "$ENV{QNX_BASE}" STREQUAL "")
    _morrow_try_register_sdk_root(_morrow_qnx_sdk_candidates "$ENV{QNX_BASE}")
endif()
if(DEFINED ENV{QNX_HOST} AND NOT "$ENV{QNX_HOST}" STREQUAL "")
    _morrow_try_register_from_qnx_host(_morrow_qnx_sdk_candidates "$ENV{QNX_HOST}")
endif()
if(DEFINED ENV{QNX_TARGET} AND NOT "$ENV{QNX_TARGET}" STREQUAL "")
    _morrow_try_register_from_qnx_target(_morrow_qnx_sdk_candidates "$ENV{QNX_TARGET}")
endif()
_morrow_try_register_sdk_root(_morrow_qnx_sdk_candidates "${MORROW_QNX_PRIMARY_SDK_ROOT}")

set(_morrow_qnx_default_windows_candidates)
foreach(_sdk_name IN LISTS MORROW_QNX_SEARCH_NAMES)
    list(APPEND _morrow_qnx_default_windows_candidates
        "D:/WorkTools/QNX/${_sdk_name}"
        "E:/WorkTools/QNX/${_sdk_name}"
        "D:/WorkSpace/Client/${_sdk_name}"
        "E:/WorkSpace/Client/${_sdk_name}"
        "D:/QNX/${_sdk_name}"
        "E:/QNX/${_sdk_name}"
        "C:/QNX/${_sdk_name}"
        "D:/${_sdk_name}"
        "E:/${_sdk_name}"
        "C:/${_sdk_name}")
endforeach()
foreach(_candidate IN LISTS _morrow_qnx_default_windows_candidates)
    _morrow_try_register_sdk_root(_morrow_qnx_sdk_candidates "${_candidate}")
endforeach()

set(_morrow_program_files "$ENV{ProgramFiles}")
set(_morrow_program_w6432 "$ENV{ProgramW6432}")
set(_morrow_local_app_data "$ENV{LOCALAPPDATA}")
set(_morrow_user_profile "$ENV{USERPROFILE}")
foreach(_base_dir IN ITEMS "${_morrow_program_files}" "${_morrow_program_w6432}" "${_morrow_local_app_data}" "${_morrow_user_profile}")
    if(_base_dir AND NOT _base_dir STREQUAL "")
        foreach(_sdk_name IN LISTS MORROW_QNX_SEARCH_NAMES)
            file(GLOB _sdk_globs LIST_DIRECTORIES true
                "${_base_dir}/QNX/${_sdk_name}"
                "${_base_dir}/${_sdk_name}"
                "${_base_dir}/QNX*/${_sdk_name}"
                "${_base_dir}/*/QNX/${_sdk_name}")
            foreach(_candidate IN LISTS _sdk_globs)
                _morrow_try_register_sdk_root(_morrow_qnx_sdk_candidates "${_candidate}")
            endforeach()
        endforeach()
    endif()
endforeach()

list(LENGTH _morrow_qnx_sdk_candidates _morrow_qnx_sdk_candidate_count)
if(_morrow_qnx_sdk_candidate_count EQUAL 0)
    message(FATAL_ERROR
        "Unable to locate ${MORROW_QNX_SDK_LABEL} on this Windows host. "
        "Set QNX_SDK_ROOT or QNX_BASE, or install qnx800 in a standard location such as D:/WorkTools/QNX/qnx800.")
endif()

list(GET _morrow_qnx_sdk_candidates 0 QNX_SDK_ROOT)
set(QNX_SDK_ROOT "${QNX_SDK_ROOT}" CACHE PATH "${MORROW_QNX_SDK_LABEL} root on Windows host" FORCE)
set(QNX_HOST "${QNX_SDK_ROOT}/host/win64/x86_64" CACHE PATH "QNX host tools path" FORCE)
set(QNX_TARGET "${QNX_SDK_ROOT}/${MORROW_QNX_TARGET_SUBDIR}" CACHE PATH "QNX target sysroot path" FORCE)
set(QNX_ENV_BAT "${QNX_SDK_ROOT}/qnxsdp-env.bat" CACHE FILEPATH "QNX SDK environment batch file" FORCE)

get_filename_component(MORROW_QNX_WINDOWS_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
set(MORROW_QNX_QCC_WRAPPER  "${MORROW_QNX_WINDOWS_DIR}/qcc-wrapper.cmd")
set(MORROW_QNX_QPP_WRAPPER  "${MORROW_QNX_WINDOWS_DIR}/qpp-wrapper.cmd")
set(MORROW_QNX_MAKE_WRAPPER "${MORROW_QNX_WINDOWS_DIR}/make-wrapper.cmd")

if(NOT EXISTS "${QNX_ENV_BAT}")
    message(FATAL_ERROR "QNX env batch not found: ${QNX_ENV_BAT}")
endif()
if(NOT EXISTS "${QNX_HOST}/usr/bin/qcc.exe")
    message(FATAL_ERROR "QNX qcc not found: ${QNX_HOST}/usr/bin/qcc.exe")
endif()
if(NOT EXISTS "${QNX_HOST}/usr/bin/q++.exe")
    message(FATAL_ERROR "QNX q++ not found: ${QNX_HOST}/usr/bin/q++.exe")
endif()
if(NOT EXISTS "${QNX_TARGET}")
    message(FATAL_ERROR "QNX target sysroot not found: ${QNX_TARGET}")
endif()

set(ENV{QNX_BASE} "${QNX_SDK_ROOT}")
set(ENV{QNX_SDK_ROOT} "${QNX_SDK_ROOT}")
set(ENV{QNX_HOST} "${QNX_HOST}")
set(ENV{QNX_TARGET} "${QNX_TARGET}")
set(ENV{MAKEFLAGS} "-I${QNX_TARGET}/usr/include")
if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}/.qnx" MORROW_QNX_CONFIGURATION_DIR)
    set(ENV{QNX_CONFIGURATION_EXCLUSIVE} "${MORROW_QNX_CONFIGURATION_DIR}")
    set(ENV{QNX_CONFIGURATION} "${MORROW_QNX_CONFIGURATION_DIR}")
endif()
set(ENV{PYTHONDONTWRITEBYTECODE} 1)
set(ENV{TMPDIR} "$ENV{TMP}")
set(ENV{PATH} "${QNX_HOST}/usr/bin;${QNX_SDK_ROOT}/host/common/bin;${QNX_SDK_ROOT}/jre/bin;$ENV{PATH}")

set(QNX_ARCH "aarch64le" CACHE STRING "QNX target arch (aarch64le/x86_64)")
set_property(CACHE QNX_ARCH PROPERTY STRINGS aarch64le x86_64)
set(CMAKE_SYSTEM_PROCESSOR "${QNX_ARCH}")

if(QNX_ARCH STREQUAL "aarch64le")
    set(QNX_BINUTILS_PREFIX "ntoaarch64")
elseif(QNX_ARCH STREQUAL "x86_64")
    set(QNX_BINUTILS_PREFIX "ntox86_64")
else()
    set(QNX_BINUTILS_PREFIX "")
endif()

set(QNX_QCC_TARGET "gcc_nto${QNX_ARCH}" CACHE STRING "qcc -V target profile")
set(QNX_SYSTEM_INCLUDE_FLAGS "-I${QNX_TARGET}/usr/include")
set(QNX_CXX_STANDARD_INCLUDE_FLAGS "")
if(EXISTS "${QNX_TARGET}/usr/include/c++/v1")
    string(APPEND QNX_CXX_STANDARD_INCLUDE_FLAGS " -isystem${QNX_TARGET}/usr/include/c++/v1")
endif()

set(QNX_COMPILER_BUILTIN_INCLUDE_FLAGS "")
file(GLOB _morrow_qnx_gcc_builtin_include_candidates LIST_DIRECTORIES true
    "${QNX_HOST}/usr/lib/gcc/*/*/include")
set(_morrow_qnx_gcc_builtin_include_dir "")
foreach(_candidate IN LISTS _morrow_qnx_gcc_builtin_include_candidates)
    if(QNX_ARCH STREQUAL "aarch64le" AND _candidate MATCHES "aarch64")
        set(_morrow_qnx_gcc_builtin_include_dir "${_candidate}")
        break()
    elseif(QNX_ARCH STREQUAL "x86_64" AND _candidate MATCHES "x86_64")
        set(_morrow_qnx_gcc_builtin_include_dir "${_candidate}")
        break()
    endif()
endforeach()
if(NOT _morrow_qnx_gcc_builtin_include_dir)
    list(LENGTH _morrow_qnx_gcc_builtin_include_candidates _morrow_qnx_gcc_builtin_include_candidate_count)
    if(_morrow_qnx_gcc_builtin_include_candidate_count GREATER 0)
        list(GET _morrow_qnx_gcc_builtin_include_candidates 0 _morrow_qnx_gcc_builtin_include_dir)
    endif()
endif()
if(_morrow_qnx_gcc_builtin_include_dir)
    string(APPEND QNX_COMPILER_BUILTIN_INCLUDE_FLAGS " -isystem${_morrow_qnx_gcc_builtin_include_dir}")
endif()

unset(CMAKE_SYSROOT)
unset(CMAKE_SYSROOT CACHE)
set(CMAKE_C_FLAGS_INIT   "${QNX_SYSTEM_INCLUDE_FLAGS} -V${QNX_QCC_TARGET}")
set(CMAKE_CXX_FLAGS_INIT "${QNX_SYSTEM_INCLUDE_FLAGS}${QNX_CXX_STANDARD_INCLUDE_FLAGS}${QNX_COMPILER_BUILTIN_INCLUDE_FLAGS} -V${QNX_QCC_TARGET}")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS_INIT}" CACHE STRING "QNX base C flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT}" CACHE STRING "QNX base C++ flags" FORCE)

option(MORROW_QNX_USE_COMPILER_WRAPPERS "Use .cmd wrappers instead of invoking qcc/q++ directly" ON)
option(MORROW_QNX_USE_MAKE_WRAPPER "Use the Windows make wrapper instead of make.exe directly" ON)

set(MORROW_QNX_QCC_EXECUTABLE "${QNX_HOST}/usr/bin/qcc.exe")
set(MORROW_QNX_QPP_EXECUTABLE "${QNX_HOST}/usr/bin/q++.exe")
set(MORROW_QNX_MAKE_EXECUTABLE "${QNX_HOST}/usr/bin/make.exe")

if(MORROW_QNX_USE_COMPILER_WRAPPERS)
    set(CMAKE_C_COMPILER   "${MORROW_QNX_QCC_WRAPPER}" CACHE FILEPATH "QNX C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "${MORROW_QNX_QPP_WRAPPER}" CACHE FILEPATH "QNX C++ compiler" FORCE)
else()
    set(CMAKE_C_COMPILER   "${MORROW_QNX_QCC_EXECUTABLE}" CACHE FILEPATH "QNX C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "${MORROW_QNX_QPP_EXECUTABLE}" CACHE FILEPATH "QNX C++ compiler" FORCE)
endif()

if(MORROW_QNX_USE_MAKE_WRAPPER)
    set(QNX_MAKE_PROGRAM   "${MORROW_QNX_MAKE_WRAPPER}" CACHE FILEPATH "Optional QNX GNU Make wrapper" FORCE)
else()
    set(QNX_MAKE_PROGRAM   "${MORROW_QNX_MAKE_EXECUTABLE}" CACHE FILEPATH "QNX GNU Make executable" FORCE)
endif()

if(QNX_BINUTILS_PREFIX)
    set(_morrow_qnx_binutils_map
        "CMAKE_AR|ar"
        "CMAKE_RANLIB|ranlib"
        "CMAKE_NM|nm"
        "CMAKE_OBJCOPY|objcopy"
        "CMAKE_OBJDUMP|objdump"
        "CMAKE_READELF|readelf"
        "CMAKE_STRIP|strip"
        "CMAKE_ADDR2LINE|addr2line")
    foreach(_tool_pair IN LISTS _morrow_qnx_binutils_map)
        string(REPLACE "|" ";" _tool_pair_parts "${_tool_pair}")
        list(GET _tool_pair_parts 0 _cmake_var)
        list(GET _tool_pair_parts 1 _tool_suffix)
        set(_tool_path "${QNX_HOST}/usr/bin/${QNX_BINUTILS_PREFIX}-${_tool_suffix}.exe")
        if(EXISTS "${_tool_path}")
            set(${_cmake_var} "${_tool_path}" CACHE FILEPATH "QNX ${_tool_suffix} tool" FORCE)
        endif()
    endforeach()
endif()

set(CMAKE_FIND_ROOT_PATH "${QNX_TARGET}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

