cmake_minimum_required(VERSION 3.10)
project(morrow.gui)

include(CTest)

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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fpermissive -g -O0 -Wno-error")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -frtti -std=gnu++17 -D_GLIBCXX_USE_C99=1 -fpermissive")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIE")

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

file(GLOB DEMO_LIST ${CMAKE_CURRENT_SOURCE_DIR}/samples/*.cpp)

foreach(FILE_PATH ${DEMO_LIST})
    get_filename_component(FILE_NAME ${FILE_PATH} NAME_WE)
    add_executable(${FILE_NAME} ${FILE_PATH})
    target_link_libraries(${FILE_NAME} morrow)
    add_custom_command(TARGET ${FILE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${FILE_NAME}>/assets"
        COMMENT "Copying assets to $<TARGET_FILE_DIR:${FILE_NAME}>/assets"
    )
endforeach()

if(BUILD_TESTING)
    add_executable(BatchBuilderTests tests/BatchBuilderTests.cpp src/core/BatchBuilder.cpp)
    target_include_directories(BatchBuilderTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME BatchBuilderTests COMMAND BatchBuilderTests)

    add_executable(BatchCompatibilityKeyCacheTests tests/BatchCompatibilityKeyCacheTests.cpp)
    target_link_libraries(BatchCompatibilityKeyCacheTests morrow)
    add_test(NAME BatchCompatibilityKeyCacheTests COMMAND BatchCompatibilityKeyCacheTests)

    add_executable(RequestRenderTests tests/RequestRenderTests.cpp)
    target_link_libraries(RequestRenderTests morrow)
    add_test(NAME RequestRenderTests COMMAND RequestRenderTests)

    add_executable(Transform3DTests tests/Transform3DTests.cpp)
    target_link_libraries(Transform3DTests morrow)
    add_test(NAME Transform3DTests COMMAND Transform3DTests)

    add_executable(Scene3DNormalizationTests tests/Scene3DNormalizationTests.cpp)
    target_link_libraries(Scene3DNormalizationTests morrow)
    add_test(NAME Scene3DNormalizationTests COMMAND Scene3DNormalizationTests)
endif()
