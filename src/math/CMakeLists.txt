file(GLOB MATH_SOURCES *.cpp)
file(GLOB MATH_HEADERS *.h *.hpp)

add_library(morrow_math STATIC ${MATH_SOURCES} ${MATH_HEADERS})

target_link_libraries(morrow_math PUBLIC morrow_utils)

set_target_properties(morrow_math PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_compile_features(morrow_math PUBLIC cxx_std_17)

target_include_directories(morrow_math PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)