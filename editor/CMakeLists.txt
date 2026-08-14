add_executable(MorrowEditor
    Editor.cpp
)

target_link_libraries(MorrowEditor PRIVATE morrow)

target_compile_definitions(MorrowEditor PRIVATE
    MORROW_EDITOR_TEST_PROJECT="${CMAKE_SOURCE_DIR}/editor/morrow.gui/MorrowUI.morrow"
)

add_custom_command(TARGET MorrowEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:MorrowEditor>/assets"
    COMMENT "Copying assets to $<TARGET_FILE_DIR:MorrowEditor>/assets"
)
