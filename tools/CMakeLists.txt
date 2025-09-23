add_executable(sulfur sulfur.cpp)

target_link_libraries(sulfur PRIVATE sulfur::libsulfur)