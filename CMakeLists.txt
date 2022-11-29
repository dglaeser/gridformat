# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)

project(gridformat VERSION 1.0.0)
set(GFMT_NAMESPACE "GridFormat")

include(GNUInstallDirs)
add_library(gridformat INTERFACE)
target_compile_features(gridformat INTERFACE cxx_std_20)
target_include_directories(gridformat
    INTERFACE $<BUILD_INTERFACE:${gridformat_SOURCE_DIR}>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

list(APPEND CMAKE_MODULE_PATH "${gridformat_SOURCE_DIR}/cmake/modules")
include(cmake/link_external_libs.cmake)

set(GRIDFORMAT_INSTALL_CMAKE_DATA_DIR "${CMAKE_INSTALL_DATAROOTDIR}/gridformat/cmake")
install(
    DIRECTORY gridformat
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    DIRECTORY "${gridformat_SOURCE_DIR}/cmake/modules"
    DESTINATION ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
)
install(
    TARGETS gridformat
    EXPORT gridformat_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
    EXPORT gridformat_Targets
    FILE gridformatTargets.cmake
    NAMESPACE GridFormat::
    DESTINATION ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/gridformat-config.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/pkg/gridformatConfig.cmake"
    PATH_VARS
        CMAKE_INSTALL_INCLUDEDIR
        GRIDFORMAT_INSTALL_CMAKE_DATA_DIR
    INSTALL_DESTINATION
        ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
)
install(
    FILES
        "${PROJECT_BINARY_DIR}/cmake/pkg/gridformatConfig.cmake"
    DESTINATION
        ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
)

option(GRIDFORMAT_BUILD_TESTS "whether or not examples should be built" OFF)
if (GRIDFORMAT_BUILD_TESTS)
    message(STATUS "Including test suite")
    enable_testing()
    add_library(GridFormat::gridformat ALIAS gridformat)
    add_subdirectory(test)
endif ()
