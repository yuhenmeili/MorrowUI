# Windows MinGW toolchain for the repository's static libs/GLFW/libglfw3.a.
# Configure with:
#   cmake -S . -B build-mingw -G Ninja
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/MinGW-GLFW.cmake
#         -DMORROW_MINGW_ROOT=<path-to-mingw>

set(CMAKE_SYSTEM_NAME Windows)

if(DEFINED MORROW_MINGW_ROOT)
    file(TO_CMAKE_PATH "${MORROW_MINGW_ROOT}" MORROW_MINGW_ROOT)
    set(CMAKE_C_COMPILER "${MORROW_MINGW_ROOT}/bin/gcc.exe" CACHE FILEPATH "MinGW C compiler")
    set(CMAKE_CXX_COMPILER "${MORROW_MINGW_ROOT}/bin/g++.exe" CACHE FILEPATH "MinGW C++ compiler")
    set(CMAKE_RC_COMPILER "${MORROW_MINGW_ROOT}/bin/windres.exe" CACHE FILEPATH "MinGW resource compiler")
    set(CMAKE_FIND_ROOT_PATH "${MORROW_MINGW_ROOT}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
