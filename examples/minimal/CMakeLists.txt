# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_minimal_example)

include(../find_gridformat.cmake)

add_executable(minimal minimal.cpp)
add_test(NAME minimal COMMAND ./minimal)
target_link_libraries(minimal PRIVATE gridformat::gridformat)
