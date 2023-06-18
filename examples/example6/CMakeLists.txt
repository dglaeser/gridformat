# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)
project(gridformat_example_6)

include(../find_gridformat.cmake)
find_package(mfem 4.5.2 CONFIG REQUIRED NAMES MFEM)

add_executable(example6 example6.cpp)
add_test(NAME example6 COMMAND ./example6)
target_link_libraries(example6 PRIVATE gridformat::gridformat mfem)
configure_file(turtle.msh turtle.msh COPYONLY)
