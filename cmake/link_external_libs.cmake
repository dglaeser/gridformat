# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

include(FeatureSummary)
function (gridformat_register_feature DESCRIPTION VARIABLE DOCSTRING)
    if (PROJECT_IS_TOP_LEVEL)
        add_feature_info(${DESCRIPTION} ${VARIABLE} ${DOCSTRING})
    endif ()
endfunction ()

find_package(ZLIB)
if (ZLIB_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE ZLIB::ZLIB)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_ZLIB)
    set(GRIDFORMAT_HAVE_ZLIB true)
endif ()
gridformat_register_feature("ZLIB compression" GRIDFORMAT_HAVE_ZLIB "writing and reading arrays compressed with ZLIB")

find_package(LZ4)
if (LZ4_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE LZ4::LZ4)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_LZ4)
    set(GRIDFORMAT_HAVE_LZ4 true)
endif ()
gridformat_register_feature("LZ4 compression" GRIDFORMAT_HAVE_LZ4 "writing and reading arrays compressed with LZ4")

find_package(LZMA)
if (LZMA_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE LZMA::LZMA)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_LZMA)
    set(GRIDFORMAT_HAVE_LZMA true)
endif ()
gridformat_register_feature("LZMA compression" GRIDFORMAT_HAVE_LZMA "writing and reading arrays compressed with LZMA")

find_package(MPI COMPONENTS CXX)
if (MPI_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE MPI::MPI_CXX)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_MPI)
    set(GRIDFORMAT_HAVE_MPI true)
endif ()
add_feature_info("Gridformat parallel I/O" GRIDFORMAT_HAVE_MPI "writing and reading with distributed memory parallelism")

option(GRIDFORMAT_BUILD_HIGH_FIVE "Controls whether HighFive should be included in the build" ON)
set(HDF5_PREFER_PARALLEL true)
find_package(HDF5 QUIET)
find_package(HighFive QUIET)
set(HIGHFIVE_TARGET_NAME HighFive)
if (HighFive_FOUND)
    message(STATUS "Using preinstalled HighFive package")
    # Before version 3, HighFive_ was used as namespace: https://github.com/highfive-devs/highfive/blob/ede97c8d51905c1640038561d12d41da173012ac/CMake/HighFiveTargetExport.cmake#L42
    # With version 3, HighFive:: is used: https://github.com/highfive-devs/highfive/blob/ddcf6321a0f2528750d846f681b8f2223106abaf/CMakeLists.txt#L145
    set(HIGHFIVE_TARGET_NAME HighFive_HighFive)
    if (HighFive_VERSION_MAJOR GREATER_EQUAL 3)
        set(HIGHFIVE_TARGET_NAME HighFive::HighFive)
    elseif (NOT HighFive_VERSION_MAJOR GREATER_EQUAL 2)
        message(WARNING "(Possibly) unsupported HighFive version found")
    endif ()
elseif (GRIDFORMAT_BUILD_HIGH_FIVE)
    set(GRIDFORMAT_HIGH_FIVE_PATH "${CMAKE_CURRENT_LIST_DIR}/../deps/HighFive")
    if (NOT HDF5_FOUND)
        message(STATUS "Not including HighFive in the build as hdf5 was not found")
    elseif (EXISTS ${GRIDFORMAT_HIGH_FIVE_PATH}/CMakeLists.txt)
        set(HIGHFIVE_UNIT_TESTS OFF)
        set(HIGHFIVE_EXAMPLES OFF)
        set(HIGHFIVE_BUILD_DOCS OFF)
        set(HIGHFIVE_PARALLEL_HDF5 ${HDF5_IS_PARALLEL})
        set(GRIDFORMAT_HIGHFIVE_SOURCE_INCLUDED true)
        message(STATUS "Including HighFive in the source tree")
        add_subdirectory(${GRIDFORMAT_HIGH_FIVE_PATH})
    else ()
        message(STATUS "HighFive not found")
    endif ()
endif ()

if (HighFive_FOUND OR GRIDFORMAT_HIGHFIVE_SOURCE_INCLUDED)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${HIGHFIVE_TARGET_NAME})
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_HIGH_FIVE GRIDFORMAT_HAVE_VTK_HDF)
    set(GRIDFORMAT_HAVE_HIGH_FIVE true)
    set(GRIDFORMAT_HAVE_VTK_HDF true)
    if (HDF5_IS_PARALLEL)
        message(STATUS "Parallel HighFive available")
        target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE)
        set(GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE true)
    endif ()
endif ()
add_feature_info("Gridformat VTK HDF support" GRIDFORMAT_HAVE_VTK_HDF "writing and reading the VTK HDF file format")

find_package(dune-localfunctions QUIET)
if (dune-localfunctions_FOUND)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS)
    set(GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS true)
endif ()
