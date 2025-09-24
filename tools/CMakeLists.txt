add_executable(sulfur sulfur.cpp)

target_link_libraries(sulfur PRIVATE sulfur::libsulfur)

include(GNUInstallDirs)

install(
    TARGETS sulfur
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)