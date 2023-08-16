# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_examples)

enable_testing()

add_subdirectory(analytical)
add_subdirectory(minimal)
add_subdirectory(voxels)
add_subdirectory(dem)

find_package(MPI)
if (MPI_FOUND)
    add_subdirectory(parallel)
else ()
    message(STATUS "Skipping parallel example because MPI was not found")
endif ()

find_package(CGAL)
if (CGAL_FOUND)
    add_subdirectory(cgal_traits)
else ()
    message(STATUS "Skipping cgal example because CGAL was not found")
endif ()

find_package(dune-functions)
if (dune-functions_FOUND)
    # unfortunately we have to repeat those here :/
    # see CMakeLists.txt of dune_traits for more details
    find_package(Threads)
    find_package(TBB)
    add_subdirectory(dune_traits)
else ()
    message(STATUS "Skipping dune example because Dune was not found")
endif ()

find_package(DOLFINX)
if (DOLFINX_FOUND)
    add_subdirectory(dolfinx_traits)
else ()
    message(STATUS "Skipping dolfinx example because dolfinx was not found")
endif ()

find_package(mfem CONFIG NAMES MFEM)
if (mfem_FOUND)
    add_subdirectory(mfem_traits)
else ()
    message(STATUS "Skipping mfem example because mfem was not found")
endif ()
