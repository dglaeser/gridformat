# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_voxels_example)

include(../find_gridformat.cmake)

add_executable(voxels voxels.cpp)
add_test(NAME voxels COMMAND ./voxels)
target_link_libraries(voxels PRIVATE gridformat::gridformat)
