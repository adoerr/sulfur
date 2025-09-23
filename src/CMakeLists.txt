add_library(libsulfur libsulfur.cpp)
add_library(sulfur::libsulfur ALIAS libsulfur)

set_target_properties(
    libsulfur
    PROPERTIES OUTPUT_NAME sulfur
)

target_compile_features(libsulfur PUBLIC cxx_std_20)

target_include_directories(
    libsulfur
        PRIVATE include
        PUBLIC ${CMAKE_SOURCE_DIR}/include
)