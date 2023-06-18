# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_example_5)

include(../find_gridformat.cmake)
find_package(DOLFINX 0.6.0 REQUIRED)

add_executable(example5 example5.cpp)
add_test(NAME example5 COMMAND ./example5)
target_link_libraries(example5 PRIVATE gridformat::gridformat dolfinx)
