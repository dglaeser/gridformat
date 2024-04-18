# SPDX-FileCopyrightText: 2024 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_benchmark)

# Fetch GridFormat
if (NOT GRIDFORMAT_FETCH_TREE)
    set(GRIDFORMAT_FETCH_TREE "main" CACHE STRING "gridformat tree to fetch" FORCE)
endif ()

if (NOT GRIDFORMAT_ORIGIN)
    set(GRIDFORMAT_ORIGIN "https://github.com/dglaeser/gridformat" CACHE STRING "url from where to fetch gridformat" FORCE)
endif ()

message(STATUS "Fetching gridformat at ${GRIDFORMAT_FETCH_TREE} from ${GRIDFORMAT_ORIGIN}")
include(FetchContent)
FetchContent_Declare(
    gridformat
    GIT_REPOSITORY ${GRIDFORMAT_ORIGIN}
    GIT_TAG ${GRIDFORMAT_FETCH_TREE}
    GIT_PROGRESS true
    GIT_SHALLOW true
    GIT_SUBMODULES_RECURSE OFF
)
FetchContent_MakeAvailable(gridformat)


enable_testing()
function (add_benchmark NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_compile_options(${NAME} PRIVATE -O3)
    target_link_libraries(benchmark_vtu PRIVATE gridformat::gridformat)
    add_test(NAME ${NAME} COMMAND ./${NAME})
endfunction ()

add_subdirectory(vtu)
