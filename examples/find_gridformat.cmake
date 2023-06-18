# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

# We want to allow the examples to run with a local installation of GridFormat
# or using the FetchContent module of cmake. In the CI, we want to be able
# to test both methods. Therefore, we centralize the logic in this file.

if (NOT gridformat_ROOT AND NOT GRIDFORMAT_FETCH_TREE)
    set(GRIDFORMAT_FETCH_TREE "main" CACHE STRING "gridformat tree to fetch" FORCE)
endif ()

if (NOT gridformat_ROOT AND NOT GRIDFORMAT_ORIGIN)
    set(GRIDFORMAT_ORIGIN "https://github.com/dglaeser/gridformat" CACHE STRING "url from where to fetch gridformat" FORCE)
endif ()

if (gridformat_ROOT)
    message(STATUS "Using local gridformat installation at ${gridformat_ROOT}")
    find_package(gridformat REQUIRED)
    # include gridformat module for gridformat_have_feature
    include(${gridformat_MODULE_DIR}/GridFormatHaveFeature.cmake)
else ()
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

    # include gridformat module for gridformat_have_feature
    set(GRIDFORMAT_SRC_DIR "")
    FetchContent_GetProperties(gridformat SOURCE_DIR GRIDFORMAT_SRC_DIR)
    include(${GRIDFORMAT_SRC_DIR}/cmake/modules/GridFormatHaveFeature.cmake)
endif ()
