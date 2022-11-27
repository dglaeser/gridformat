# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)

project(gridformat VERSION 1.0.0)
set(GFMT_NAMESPACE "GridFormat")

include(GNUInstallDirs)
add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME}
    INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
include(cmake/link_external_libs.cmake)

enable_testing()
add_library(${GFMT_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
add_subdirectory(test)
