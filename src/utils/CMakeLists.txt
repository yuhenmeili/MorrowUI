file(GLOB UTILS_SOURCES *.cpp)
file(GLOB UTILS_HEADERS *.h)

add_library(morrow_utils STATIC ${UTILS_SOURCES} ${UTILS_HEADERS})

set_target_properties(morrow_utils PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_compile_features(morrow_utils PUBLIC cxx_std_17)

target_include_directories(morrow_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)