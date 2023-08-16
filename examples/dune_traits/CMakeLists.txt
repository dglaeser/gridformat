# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_dune_example)

include(../find_gridformat.cmake)

# Dune links against some libs, but its cmake setup doesn't seem to look
# for the packages again when linked in downstream projects, so we need
# to add a bunch of commands to find dependencies before linking against Dune :/.
find_package(Threads)
find_package(TBB)

# Unfortunately, transitive linking seems to not work properly in dune, and
# we need to find further modules manually here and link against them :/ ...
# if you use GridFormat within a dune module as git-submodule or sub-folder
# in a standard dune development environment, everything should work fine...
find_package(dune-grid REQUIRED)
find_package(dune-geometry REQUIRED)
find_package(dune-common REQUIRED)
find_package(dune-localfunctions REQUIRED)
find_package(dune-functions REQUIRED)

add_executable(dune_traits dune_traits.cpp)
add_test(NAME dune_traits COMMAND ./dune_traits)
target_link_libraries(dune_traits PRIVATE gridformat::gridformat dunegrid dunegeometry dunecommon)

# the dune cmake setup seems to not set the include dirs via target_link_libraries appropriately,
# which is required in case it is installed into a non-standard location. To make this case work,
# we add them manually here.
target_include_directories(
    dune_traits
    PRIVATE
        ${dune-grid_INCLUDE_DIRS} ${dune-functions_INCLUDE_DIRS} ${dune-localfunctions_INCLUDE_DIRS}
)
