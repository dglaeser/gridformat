# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_example_4)

include(../find_gridformat.cmake)

# Dune links against some libs, but its cmake setup doesn't seem to look
# for the packages again when linked in downstream projects, so we need
# to add a bunch of commands to find dependencies before linking against Dune.
find_package(Threads)
find_package(TBB)

# We actually only need dune-grid, which again depends on geometry and common.
# Unfortunately, the transitive linking seems to not work properly in dune, and
# we need to find all three packages and manually link against them...
find_package(dune-grid REQUIRED)
find_package(dune-geometry REQUIRED)
find_package(dune-common REQUIRED)

add_executable(example4 example4.cpp)
add_test(NAME example4 COMMAND ./example4)
target_link_libraries(example4 PRIVATE gridformat::gridformat dunegrid dunegeometry dunecommon)

# the dune cmake setup seems to not set the include dirs via target_link_libraries appropriately,
# which is required in case it is installed into a non-standard location. To make this case work,
# we add them manually here.
target_include_directories(example4 PRIVATE ${dune-grid_INCLUDE_DIRS})
