# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED PROJECT_NAME)
  set(GRIDFORMAT_SUBPROJECT OFF)
else()
  set(GRIDFORMAT_SUBPROJECT ON)
endif()

project(gridformat VERSION 1.0.0)
set(GRIDFORMAT_NAMESPACE "gridformat")

include(GNUInstallDirs)
add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME}
    INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Alias for both test suite and sub-projects (e.g. using cmake's FetchContent)
add_library(${GRIDFORMAT_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(cmake/link_external_libs.cmake)

if (NOT GRIDFORMAT_SUBPROJECT)

    set(GRIDFORMAT_INSTALL_CMAKE_DATA_DIR "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake")
    install(
        DIRECTORY ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/modules"
        DESTINATION ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
    )
    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(
        EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${GRIDFORMAT_NAMESPACE}::
        DESTINATION ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
        "${PROJECT_BINARY_DIR}/cmake/pkg/${PROJECT_NAME}Config.cmake"
        PATH_VARS
            CMAKE_INSTALL_INCLUDEDIR
            GRIDFORMAT_NAMESPACE
            GRIDFORMAT_INSTALL_CMAKE_DATA_DIR
        INSTALL_DESTINATION
            ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
    )
    install(
        FILES
            "${PROJECT_BINARY_DIR}/cmake/pkg/${PROJECT_NAME}Config.cmake"
        DESTINATION
            ${GRIDFORMAT_INSTALL_CMAKE_DATA_DIR}
    )
endif ()

option(GRIDFORMAT_BUILD_TESTS "whether or not examples should be built" OFF)
if (GRIDFORMAT_BUILD_TESTS)
    message(STATUS "Including test suite")
    enable_testing()
    add_subdirectory(test)
endif ()
