# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_parallel_example)

include(../find_gridformat.cmake)

set(HAVE_MPI false)
gridformat_have_feature(MPI HAVE_MPI)
if (NOT HAVE_MPI)
    message(FATAL_ERROR "MPI (and gridformat configured with it) is required for this example")
endif ()

add_executable(parallel parallel.cpp)
add_test(NAME parallel COMMAND mpirun -n 2 ./parallel)
target_link_libraries(parallel PRIVATE gridformat::gridformat)
