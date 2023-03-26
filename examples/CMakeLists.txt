# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)
project(gridformat_examples)

if (gridformat_ROOT)
    message(STATUS "Using local gridformat installation")
    find_package(gridformat REQUIRED)
else ()
    if (NOT GRIDFORMAT_FETCH_TREE)
        execute_process(
            COMMAND git rev-parse HEAD
            OUTPUT_VARIABLE GRIDFORMAT_FETCH_TREE
        )
        if (NOT GRIDFORMAT_FETCH_TREE)
            set(GRIDFORMAT_FETCH_TREE "main" CACHE STRING "gridformat tree to fetch" FORCE)
        endif ()
    endif ()
    message(STATUS "Fetching gridformat at ${GRIDFORMAT_FETCH_TREE}")
    include(FetchContent)
    FetchContent_Declare(
        gridformat
        GIT_REPOSITORY https://github.com/dglaeser/gridformat
        GIT_TAG ${GRIDFORMAT_FETCH_TREE}
    )
    FetchContent_MakeAvailable(gridformat)
endif ()

enable_testing()
function(gridformat_add_example NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PUBLIC gridformat::gridformat)
    target_compile_options(${NAME} PRIVATE -fconcepts-diagnostics-depth=20)
    add_test(NAME ${NAME} COMMAND ./${NAME})
endfunction()

gridformat_add_example(example_unstructured example_unstructured.cpp)
