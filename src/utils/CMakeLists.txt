file(GLOB UTILS_SOURCES *.cpp)

add_library(morrow_utils STATIC ${UTILS_SOURCES})

set_target_properties(morrow_utils PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_compile_features(morrow_utils PUBLIC cxx_std_17)

# 公共面：include/（morrow/utils/*）；实现自用：src/utils（转发头）。
target_include_directories(morrow_utils
    PUBLIC ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
