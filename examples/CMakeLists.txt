# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(gridformat_examples)

enable_testing()
add_subdirectory(voxels)
add_subdirectory(dem)

find_package(CGAL)
if (CGAL_FOUND)
    add_subdirectory(cgal)
else ()
    message(STATUS "Skipping cgal example because CGAL was not found")
endif ()

find_package(dune-functions)
if (dune-functions_FOUND)
    # unfortunately we have to repeat those here :/
    # see CMakeLists.txt of example4 for more details
    find_package(Threads)
    find_package(TBB)
    add_subdirectory(example4)
else ()
    message(STATUS "Skipping example 4 because Dune was not found")
endif ()

find_package(DOLFINX)
if (DOLFINX_FOUND)
    add_subdirectory(example5)
else ()
    message(STATUS "Skipping example 5 because dolfinx was not found")
endif ()

find_package(mfem CONFIG NAMES MFEM)
if (mfem_FOUND)
    add_subdirectory(example6)
else ()
    message(STATUS "Skipping example 6 because mfem was not found")
endif ()

find_package(MPI)
if (MPI_FOUND)
    add_subdirectory(example7)
else ()
    message(STATUS "Skipping example 7 because mfem was not found")
endif ()
