# 仓库内工具豁免：编辑器目标共用引擎实现私有 include 集。
set(MORROW_EDITOR_IMPL_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/platform
    ${CMAKE_SOURCE_DIR}/src/renderer
    ${CMAKE_SOURCE_DIR}/src/renderer/device
    ${CMAKE_SOURCE_DIR}/src/renderer/resource
    ${CMAKE_SOURCE_DIR}/src/renderer/resource/ssbo
    ${CMAKE_SOURCE_DIR}/src/renderer/scene3d
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/fonts
    ${CMAKE_SOURCE_DIR}/src/gltf
    ${CMAKE_SOURCE_DIR}/src/debug
    ${CMAKE_SOURCE_DIR}/src/extern/common)

add_library(morrow_editor_document STATIC
    ProjectSettings.cpp
    scene/SceneDocument.cpp
    scene/SceneEditorModel.cpp
    scene/EditorSession.cpp
    scene/NodeTypeCatalog.cpp
    commands/CommandHistory.cpp
    assets/AssetDatabase.cpp
    assets/MaterialAsset.cpp
    assets/ImportQueue.cpp
    filesystem/ProjectFileSystemModel.cpp
    filesystem/FileSystemWatcher.cpp
    EditorInputRouter.cpp
    ui/DockLayout.cpp
    build/BuildQueue.cpp
)

target_include_directories(morrow_editor_document PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(morrow_editor_document PRIVATE ${MORROW_EDITOR_IMPL_DIRS})

add_library(morrow_editor_runtime STATIC
    scene/SceneInstantiator.cpp
    filesystem/ThumbnailService.cpp
    ui/DockDropOverlay.cpp
    ui/CreateNodeDialog.cpp
    ui/CreateAssetDialog.cpp
    ui/RenameNodeDialog.cpp
    panels/FileSystemPanel.cpp
    panels/InspectorPanel.cpp
    panels/SceneTreePanel.cpp
    panels/ViewportPanel.cpp
    panels/ToolbarPanel.cpp
    panels/AssetBrowserPanel.cpp
    panels/OutputPanel.cpp
    panels/BuildPanel.cpp
    layout/EditorLayoutController.cpp
    panels/EditorPanel.cpp
    EditorShell.cpp
)

target_link_libraries(morrow_editor_runtime
    PUBLIC
    morrow_editor_document
    morrow
)
target_include_directories(morrow_editor_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# 仓库内工具豁免：编辑器需要引擎实现私有头（Window/平台编排/帧上下文内部），
# vendor 头与公共边界约束只针对外部集成方。
target_include_directories(morrow_editor_runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/platform
    ${CMAKE_SOURCE_DIR}/src/renderer
    ${CMAKE_SOURCE_DIR}/src/renderer/device
    ${CMAKE_SOURCE_DIR}/src/renderer/resource
    ${CMAKE_SOURCE_DIR}/src/renderer/scene3d
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/fonts
    ${CMAKE_SOURCE_DIR}/src/gltf
    ${CMAKE_SOURCE_DIR}/src/debug)
# 仓库内工具豁免：编辑器缩略图直接用 stb_image（vendor 头不进入引擎公共接口）。
target_include_directories(morrow_editor_runtime PRIVATE ${CMAKE_SOURCE_DIR}/src/extern/common)

add_executable(MorrowEditor Editor.cpp)
target_link_libraries(MorrowEditor PRIVATE morrow_editor_runtime)
target_include_directories(MorrowEditor PRIVATE ${MORROW_EDITOR_IMPL_DIRS})

add_executable(MorrowSceneRuntime SceneRuntime.cpp)
target_link_libraries(MorrowSceneRuntime PRIVATE morrow_editor_runtime)
target_include_directories(MorrowSceneRuntime PRIVATE ${MORROW_EDITOR_IMPL_DIRS})

target_compile_definitions(MorrowEditor PRIVATE
    MORROW_EDITOR_TEST_PROJECT="${CMAKE_SOURCE_DIR}/editor/morrow.gui/MorrowUI.morrow"
)
target_compile_definitions(MorrowSceneRuntime PRIVATE
    MORROW_RUNTIME_TEST_PROJECT="${CMAKE_SOURCE_DIR}/editor/morrow.gui/MorrowUI.morrow"
)

add_custom_command(TARGET MorrowEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:MorrowEditor>/assets"
    COMMENT "Copying assets to $<TARGET_FILE_DIR:MorrowEditor>/assets"
)

add_custom_command(TARGET MorrowSceneRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:MorrowSceneRuntime>/assets"
    COMMENT "Copying runtime assets to $<TARGET_FILE_DIR:MorrowSceneRuntime>/assets"
)

