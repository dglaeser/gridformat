# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_dem_example)

include(../find_gridformat.cmake)

add_executable(dem dem.cpp)
add_test(NAME dem COMMAND ./dem)
target_link_libraries(dem PRIVATE gridformat::gridformat)
