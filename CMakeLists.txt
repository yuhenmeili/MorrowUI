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

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /GR /std:c++17")
elseif(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(APPEND CMAKE_C_FLAGS " -g -O0 -Wno-error")
    string(APPEND CMAKE_CXX_FLAGS " -std=c++17")
else()
    string(APPEND CMAKE_C_FLAGS " -fPIC -fpermissive -g -O0 -Wno-error")
    string(APPEND CMAKE_CXX_FLAGS " -fPIC -frtti -std=gnu++17 -D_GLIBCXX_USE_C99=1 -fpermissive")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fPIE")
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
    add_executable(MainThreadDispatcherTests
        tests/MainThreadDispatcherTests.cpp
        src/core/MainThreadDispatcher.cpp)
    target_include_directories(MainThreadDispatcherTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(MainThreadDispatcherTests morrow_utils)
    add_test(NAME MainThreadDispatcherTests COMMAND MainThreadDispatcherTests)

    add_executable(ObservableTests tests/ObservableTests.cpp)
    target_include_directories(ObservableTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(ObservableTests morrow_utils)
    add_test(NAME ObservableTests COMMAND ObservableTests)

    add_executable(BatchBuilderTests tests/BatchBuilderTests.cpp src/core/BatchBuilder.cpp)
    target_include_directories(BatchBuilderTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME BatchBuilderTests COMMAND BatchBuilderTests)

    add_executable(BatchCompatibilityKeyCacheTests tests/BatchCompatibilityKeyCacheTests.cpp)
    target_link_libraries(BatchCompatibilityKeyCacheTests morrow)
    add_test(NAME BatchCompatibilityKeyCacheTests COMMAND BatchCompatibilityKeyCacheTests)

    add_executable(RequestRenderTests tests/RequestRenderTests.cpp)
    target_link_libraries(RequestRenderTests morrow)
    add_test(NAME RequestRenderTests COMMAND RequestRenderTests)

    add_executable(InputInteractionTests tests/InputInteractionTests.cpp)
    target_link_libraries(InputInteractionTests morrow)
    add_test(NAME InputInteractionTests COMMAND InputInteractionTests)

    add_executable(SelectionControlsTests tests/SelectionControlsTests.cpp)
    target_link_libraries(SelectionControlsTests morrow)
    add_test(NAME SelectionControlsTests COMMAND SelectionControlsTests)

    add_executable(MenuControlsTests tests/MenuControlsTests.cpp)
    target_link_libraries(MenuControlsTests morrow)
    add_test(NAME MenuControlsTests COMMAND MenuControlsTests)

    add_executable(ScrollControlsTests tests/ScrollControlsTests.cpp)
    target_link_libraries(ScrollControlsTests morrow)
    add_test(NAME ScrollControlsTests COMMAND ScrollControlsTests)

    add_executable(SpinBoxLayoutElementsTests tests/SpinBoxLayoutElementsTests.cpp)
    target_link_libraries(SpinBoxLayoutElementsTests morrow)
    add_test(NAME SpinBoxLayoutElementsTests COMMAND SpinBoxLayoutElementsTests)

    add_executable(TextEditTests tests/TextEditTests.cpp)
    target_link_libraries(TextEditTests morrow)
    add_test(NAME TextEditTests COMMAND TextEditTests)

    add_executable(RichTextLabelTests tests/RichTextLabelTests.cpp)
    target_link_libraries(RichTextLabelTests morrow)
    add_test(NAME RichTextLabelTests COMMAND RichTextLabelTests)

    add_executable(ParticlesCanvasModulateTests tests/ParticlesCanvasModulateTests.cpp)
    target_link_libraries(ParticlesCanvasModulateTests morrow)
    add_test(NAME ParticlesCanvasModulateTests COMMAND ParticlesCanvasModulateTests)

    add_executable(ItemTreeVideoTests tests/ItemTreeVideoTests.cpp)
    target_link_libraries(ItemTreeVideoTests morrow)
    add_test(NAME ItemTreeVideoTests COMMAND ItemTreeVideoTests)

    add_executable(Transform3DTests tests/Transform3DTests.cpp)
    target_link_libraries(Transform3DTests morrow)
    add_test(NAME Transform3DTests COMMAND Transform3DTests)

    add_executable(Scene3DNormalizationTests tests/Scene3DNormalizationTests.cpp)
    target_link_libraries(Scene3DNormalizationTests morrow)
    add_test(NAME Scene3DNormalizationTests COMMAND Scene3DNormalizationTests)

    add_executable(Scene3DRedrawTests tests/Scene3DRedrawTests.cpp)
    target_link_libraries(Scene3DRedrawTests morrow)
    add_test(NAME Scene3DRedrawTests COMMAND Scene3DRedrawTests)

    add_executable(SSBOLayoutTests tests/SSBOLayoutTests.cpp)
    target_link_libraries(SSBOLayoutTests morrow)
    add_test(NAME SSBOLayoutTests COMMAND SSBOLayoutTests)
  endif()
