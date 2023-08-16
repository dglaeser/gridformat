# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_dolfinx_example)

include(../find_gridformat.cmake)
find_package(DOLFINX 0.6.0 REQUIRED)

add_executable(dolfinx_traits dolfinx_traits.cpp)
add_test(NAME dolfinx_traits COMMAND ./dolfinx_traits)
target_link_libraries(dolfinx_traits PRIVATE gridformat::gridformat dolfinx)
