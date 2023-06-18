# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_example_1)

include(../find_gridformat.cmake)

add_executable(example1 example1.cpp)
add_test(NAME example1 COMMAND ./example1)
target_link_libraries(example1 PRIVATE gridformat::gridformat)
