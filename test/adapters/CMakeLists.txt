# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(dune-common QUIET)
find_package(dune-geometry QUIET)
find_package(dune-grid QUIET)
find_package(Threads)  # dune links against it but doesn't seem to check for it itself
find_package(TBB)  # dune links against it but doesn't seem to check for it itself

if (dune-grid_FOUND AND dune-common_FOUND AND dune-geometry_FOUND)
    message(STATUS "Found dune libraries")
    set(DUNE_FOUND true)
endif ()

gridformat_add_regression_test_if(DUNE_FOUND
    test_dune_unstructured
    test_dune_unstructured.cpp
    "dune_vtu*"
)
gridformat_add_parallel_regression_test_if(DUNE_FOUND
    test_dune_unstructured_parallel
    test_dune_unstructured_parallel.cpp
    4
    "dune_pvtu*pvtu"
)
if (${DUNE_FOUND})
    target_link_libraries(test_dune_unstructured PRIVATE dunecommon dunegeometry dunegrid)
    target_link_libraries(test_dune_unstructured_parallel PRIVATE dunecommon dunegeometry dunegrid)

    # the dune cmake setup seems to not properly set the include dirs via target_link_libraries (in case it is installed in non-standard location)
    target_include_directories(test_dune_unstructured PRIVATE ${dune-grid_INCLUDE_DIRS} ${dune-geometry_INCLUDE_DIRS} ${dune-common_INCLUDE_DIRS})
    target_include_directories(test_dune_unstructured_parallel PRIVATE ${dune-grid_INCLUDE_DIRS} ${dune-geometry_INCLUDE_DIRS} ${dune-common_INCLUDE_DIRS})

    # dune needs this to know if MPI is present
    if (${MPI_FOUND})
        target_compile_definitions(test_dune_unstructured_parallel PUBLIC HAVE_MPI=1)
    endif ()
endif ()
