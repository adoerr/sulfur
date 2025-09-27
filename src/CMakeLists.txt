add_library(libsulfur process.cpp)

add_library(sulfur::libsulfur ALIAS libsulfur)

set_target_properties(
    libsulfur
    PROPERTIES OUTPUT_NAME sulfur
)

target_compile_features(libsulfur PUBLIC cxx_std_20)

target_include_directories(libsulfur
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/include
)

include(GNUInstallDirs)

install(TARGETS libsulfur
    EXPORT sulfur-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT sulfur-targets
    FILE sulfur-config.cmake
    NAMESPACE sulfur::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sulfur
)
