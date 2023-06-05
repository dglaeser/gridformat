# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)
project(gridformat_examples)

enable_testing()
add_subdirectory(example1)
add_subdirectory(example2)

find_package(CGAL)
if (CGAL_FOUND)
    add_subdirectory(example3)
endif ()

find_package(dune-grid)
if (dune-grid_FOUND)
    add_subdirectory(example4)
endif ()

find_package(DOLFINX)
if (DOLFINX_FOUND)
    add_subdirectory(example5)
endif ()
