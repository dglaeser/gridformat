# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)
project(gridformat_example_1)

include(../find_gridformat.cmake)

add_executable(example2 example2.cpp)
add_test(example2 COMMAND ./example2)
target_link_libraries(example2 PRIVATE gridformat::gridformat)
