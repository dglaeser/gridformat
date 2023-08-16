# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_analytical_example)

include(../find_gridformat.cmake)

add_executable(analytical analytical.cpp)
add_test(NAME analytical COMMAND ./analytical)
target_link_libraries(analytical PRIVATE gridformat::gridformat)
