add_library(morrow_editor_document STATIC
    ProjectSettings.cpp
    scene/SceneDocument.cpp
    scene/SceneEditorModel.cpp
    scene/EditorSession.cpp
    scene/NodeTypeCatalog.cpp
    commands/CommandHistory.cpp
    assets/AssetDatabase.cpp
    assets/ImportQueue.cpp
    filesystem/ProjectFileSystemModel.cpp
    filesystem/FileSystemWatcher.cpp
    EditorInputRouter.cpp
    ui/DockLayout.cpp
    build/BuildQueue.cpp
)

target_include_directories(morrow_editor_document PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(morrow_editor_runtime STATIC
    scene/SceneInstantiator.cpp
    filesystem/ThumbnailService.cpp
    ui/DockDropOverlay.cpp
    ui/CreateNodeDialog.cpp
    ui/FileSystemPanel.cpp
    EditorShell.cpp
)

target_link_libraries(morrow_editor_runtime
    PUBLIC
    morrow_editor_document
    morrow
)
target_include_directories(morrow_editor_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(MorrowEditor Editor.cpp)
target_link_libraries(MorrowEditor PRIVATE morrow_editor_runtime)

target_compile_definitions(MorrowEditor PRIVATE
    MORROW_EDITOR_TEST_PROJECT="${CMAKE_SOURCE_DIR}/editor/morrow.gui/MorrowUI.morrow"
)

add_custom_command(TARGET MorrowEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:MorrowEditor>/assets"
    COMMENT "Copying assets to $<TARGET_FILE_DIR:MorrowEditor>/assets"
)

if(BUILD_TESTING)
    add_executable(EditorEventsTests
        ${CMAKE_SOURCE_DIR}/tests/EditorEventsTests.cpp
    )
    target_link_libraries(EditorEventsTests PRIVATE morrow_editor_document morrow)
    add_test(NAME EditorEventsTests COMMAND EditorEventsTests)

    add_executable(SceneDocumentTests
        ${CMAKE_SOURCE_DIR}/tests/SceneDocumentTests.cpp
    )
    target_link_libraries(SceneDocumentTests PRIVATE morrow_editor_document)
    add_test(NAME SceneDocumentTests COMMAND SceneDocumentTests)

    add_executable(ProjectFileSystemModelTests
        ${CMAKE_SOURCE_DIR}/tests/ProjectFileSystemModelTests.cpp
    )
    target_link_libraries(ProjectFileSystemModelTests PRIVATE morrow_editor_document)
    add_test(NAME ProjectFileSystemModelTests COMMAND ProjectFileSystemModelTests)

    add_executable(FileSystemEnhancementTests
        ${CMAKE_SOURCE_DIR}/tests/FileSystemEnhancementTests.cpp
    )
    target_link_libraries(FileSystemEnhancementTests PRIVATE morrow_editor_document)
    add_test(NAME FileSystemEnhancementTests COMMAND FileSystemEnhancementTests)
endif()
