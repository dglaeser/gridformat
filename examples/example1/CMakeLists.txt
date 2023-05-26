# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)
project(gridformat_example_1)

include(../find_gridformat.cmake)

add_executable(example1 example1.cpp)
add_test(NAME example1 COMMAND ./example1)
target_link_libraries(example1 PRIVATE gridformat::gridformat)
