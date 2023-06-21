# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

find_package(CGAL)
gridformat_add_regression_test_if(CGAL_FOUND
    test_cgal
    test_cgal.cpp
    "cgal_vtu*"
    SKIP_MEMCHECK  # on the github runner memcheck fails with some error from cgal (could not be produced locally)
)
if (CGAL_FOUND)
    message(STATUS "Testing CGAL traits with CGAL version ${CGAL_VERSION}")
    target_link_libraries(test_cgal CGAL::CGAL)
    target_compile_options(test_cgal PRIVATE -frounding-math)
    target_compile_definitions(test_cgal PRIVATE GRIDFORMAT_IGNORE_CGAL_WARNINGS=1)
endif ()


find_package(DOLFINX)
gridformat_add_parallel_regression_test_if(DOLFINX_FOUND
    test_dolfinx
    test_dolfinx.cpp
    2
    "dolfinx_*nranks_2*"
)
gridformat_add_parallel_regression_test_if(DOLFINX_FOUND
    test_dolfinx_sequential
    test_dolfinx.cpp
    1
    "dolfinx_*nranks_1*"
)
if (DOLFINX_FOUND)
    message(STATUS "Testing dolfinx traits with version ${DOLFINX_VERSION}")
    target_link_libraries(test_dolfinx dolfinx)
    target_link_libraries(test_dolfinx_sequential dolfinx)
endif ()


find_package(deal.II)
if (deal.II_FOUND)
    message(STATUS "Testing deal.ii traits with deal.ii version ${deal.II_VERSION}")
    deal_ii_initialize_cached_variables()
    set(GRIDFORMAT_HAVE_PARALLEL_DEAL_II ${MPI_FOUND})
endif()

function (gridformat_setup_deal_ii_target TARGET)
    if (deal.II_FOUND)
        deal_ii_setup_target(${TARGET})
        # two flags added by dealii produce (in our opinion) false positives with gcc (there are related open issues in gcc)
        target_compile_options(${TARGET} PRIVATE -Wno-overloaded-virtual -Wno-missing-braces -frounding-math)
        target_compile_definitions(${TARGET} PRIVATE
            MESH_FILE_2D="${CMAKE_CURRENT_SOURCE_DIR}/simplices_2d.msh"
            MESH_FILE_3D="${CMAKE_CURRENT_SOURCE_DIR}/simplices_3d.msh"
        )
    endif ()
endfunction ()

gridformat_add_regression_test_if(deal.II_FOUND
    test_dealii_sequential
    test_dealii_sequential.cpp
    "dealii_vtu*"
    SKIP_MEMCHECK  # valgrind complains about something in deal.ii, which is unrelated to our traits.
)
gridformat_add_parallel_regression_test_if(GRIDFORMAT_HAVE_PARALLEL_DEAL_II
    test_dealii_parallel
    test_dealii_parallel.cpp
    2
    "dealii_pvtu*.pvt*"
)
gridformat_setup_deal_ii_target(test_dealii_sequential)
gridformat_setup_deal_ii_target(test_dealii_parallel)


find_package(mfem NAMES MFEM)
gridformat_add_regression_test_if(mfem_FOUND
    test_mfem
    test_mfem.cpp
    "mfem_*"
)
if (mfem_FOUND)
    message(STATUS "Testing mfem traits with mfem version ${mfem_VERSION}")
    target_link_libraries(test_mfem mfem)
endif()


find_package(dune-common QUIET)
find_package(dune-geometry QUIET)
find_package(dune-grid QUIET)
find_package(dune-alugrid QUIET)
find_package(dune-functions QUIET)

# dune may link against the following targets but doesn't seem to check for them downstream :/...
# we need to also prepend the dune-common module path such that it uses the dune FindMETIS module,
# because alugrid links against METIS::METIS, which is defined therein and it doesn't transitively
# search for it... There seem to be several issues with the dune cmake setup, when installed in
# non-standard locations at least...
if (dune-common_FOUND)
    list(PREPEND CMAKE_MODULE_PATH ${dune-common_MODULE_PATH})
endif ()
find_package(Threads)
find_package(TBB)
find_package(MPI)
find_package(METIS)

if (dune-grid_FOUND AND dune-common_FOUND AND dune-geometry_FOUND)
    message(STATUS
        "Testing Dune traits with versions "
        "${dune-common_VERSION} (dune-common), "
        "${dune-grid_VERSION} (dune-grid), "
        "${dune-geometry_VERSION} (dune-geometry)"
    )
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
        target_compile_definitions(${dune_target} PUBLIC GRIDFORMAT_IGNORE_DUNE_WARNINGS=1)
        target_compile_definitions(${dune_target} PUBLIC
            MESH_FILE_2D="${CMAKE_CURRENT_SOURCE_DIR}/simplices_2d.msh"
            MESH_FILE_3D="${CMAKE_CURRENT_SOURCE_DIR}/simplices_3d.msh"
        )

        # the dune cmake setup seems to not properly set the include dirs via target_link_libraries
        # (in case it is installed in non-standard location)
        target_include_directories(
            ${dune_target}
            PRIVATE
                ${dune-grid_INCLUDE_DIRS} ${dune-geometry_INCLUDE_DIRS} ${dune-common_INCLUDE_DIRS}
        )

        if (dune-alugrid_FOUND)
            # clang-16 fails to compile with alugrid included :/
            set(GRIDFORMAT_HAVE_ALUGRID true)
            if (${CMAKE_CXX_COMPILER} MATCHES "clang")
                set(GRIDFORMAT_HAVE_ALUGRID false)
            endif ()
            target_link_libraries(${dune_target} dunealugrid)
            target_include_directories(${dune_target} PRIVATE ${dune-alugrid_INCLUDE_DIRS})
            target_compile_definitions(${dune_target} PUBLIC GRIDFORMAT_HAVE_DUNE_ALUGRID=${GRIDFORMAT_HAVE_ALUGRID})
        endif ()

        if (dune-functions_FOUND)
            target_include_directories(${dune_target} PRIVATE ${dune-functions_INCLUDE_DIRS})
            target_compile_definitions(${dune_target} PUBLIC GRIDFORMAT_HAVE_DUNE_FUNCTIONS=1)
        endif ()

        # dune uses this preprocessor variable to know if MPI is present (but doesn't seem to set it itself)
        if (${MPI_FOUND} AND ${dune_target} MATCHES "parallel")
            target_compile_definitions(${dune_target} PUBLIC HAVE_MPI=1)
        endif ()
    endforeach ()
endif ()
