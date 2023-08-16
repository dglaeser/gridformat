# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_example_6)

include(../find_gridformat.cmake)
find_package(mfem 4.5.2 CONFIG REQUIRED NAMES MFEM)

add_executable(mfem_traits mfem_traits.cpp)
add_test(NAME mfem_traits COMMAND ./mfem_traits)
target_link_libraries(mfem_traits PRIVATE gridformat::gridformat mfem)
configure_file(turtle.msh turtle.msh COPYONLY)
