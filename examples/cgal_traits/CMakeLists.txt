# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_cgal_example)

include(../find_gridformat.cmake)
find_package(CGAL REQUIRED)

add_executable(cgal_traits cgal_traits.cpp)
add_test(NAME cgal_traits COMMAND ./cgal_traits)
target_link_libraries(cgal_traits PRIVATE gridformat::gridformat CGAL::CGAL)
