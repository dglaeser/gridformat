# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)
project(gridformat_example_1)

include(../find_gridformat.cmake)
find_package(CGAL REQUIRED)

add_executable(example3 example3.cpp)
add_test(NAME example3 COMMAND ./example3)
target_link_libraries(example3 PRIVATE gridformat::gridformat CGAL::CGAL)
