# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(deal.II)
if (deal.II_FOUND)
    deal_ii_initialize_cached_variables()
    set(GRIDFORMAT_HAVE_PARALLEL_DEAL_II ${MPI_FOUND})
endif()

function (gridformat_setup_deal_ii_target TARGET)
    if (deal.II_FOUND)
        deal_ii_setup_target(${TARGET})
        # two flags added by dealii produce (in our opinion) false positives with gcc (there are related open issues in gcc)
        target_compile_options(${TARGET} PRIVATE -Wno-overloaded-virtual -Wno-missing-braces -frounding-math)
    endif ()
endfunction ()

gridformat_add_regression_test_if(deal.II_FOUND
    test_dealii_sequential
    test_dealii_sequential.cpp
    "dealii_vtu*"
    SKIP_MEMCHECK
)
gridformat_add_parallel_regression_test_if(GRIDFORMAT_HAVE_PARALLEL_DEAL_II
    test_dealii_parallel
    test_dealii_parallel.cpp
    2
    "dealii_pvtu*.pvt*"
)
gridformat_setup_deal_ii_target(test_dealii_sequential)
gridformat_setup_deal_ii_target(test_dealii_parallel)

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
gridformat_add_regression_test_if(DUNE_FOUND
    test_dune_yasp_image_grid
    test_dune_yasp_image_grid.cpp
    "dune_vti*"
)
gridformat_add_regression_test_if(DUNE_FOUND
    test_dune_yasp_rectilinear_grid
    test_dune_yasp_rectilinear_grid.cpp
    "dune_vtr*"
)
gridformat_add_parallel_regression_test_if(DUNE_FOUND
    test_dune_unstructured_parallel
    test_dune_unstructured_parallel.cpp
    4
    "dune_pvtu*pvtu"
)
gridformat_add_parallel_regression_test_if(DUNE_FOUND
    test_dune_yasp_parallel_image_grid
    test_dune_yasp_parallel_image_grid.cpp
    4
    "dune_pvti*pvti"
)
gridformat_add_parallel_regression_test_if(DUNE_FOUND
    test_dune_yasp_parallel_rectilinear_grid
    test_dune_yasp_parallel_rectilinear_grid.cpp
    4
    "dune_pvtr*pvtr"
)
if (${DUNE_FOUND})
    list(
        APPEND
            dune_targets
            test_dune_unstructured
            test_dune_unstructured_parallel
            test_dune_yasp_image_grid
            test_dune_yasp_rectilinear_grid
            test_dune_yasp_parallel_image_grid
            test_dune_yasp_parallel_rectilinear_grid
    )
    foreach (dune_target IN LISTS dune_targets)
        target_link_libraries(${dune_target} dunecommon dunegeometry dunegrid)

        # the dune cmake setup seems to not properly set the include dirs via target_link_libraries
        # (in case it is installed in non-standard location)
        target_include_directories(
            ${dune_target}
            PRIVATE
                ${dune-grid_INCLUDE_DIRS} ${dune-geometry_INCLUDE_DIRS} ${dune-common_INCLUDE_DIRS}
        )

        # dune uses this preprocessor variable to know if MPI is present (but doesn't seem to set it itself)
        if (${MPI_FOUND} AND ${dune_target} MATCHES "parallel")
            target_compile_definitions(${dune_target} PUBLIC HAVE_MPI=1)
        endif ()
    endforeach ()
endif ()
