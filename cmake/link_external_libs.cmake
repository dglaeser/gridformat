# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

find_package(ZLIB)
if (ZLIB_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE ZLIB::ZLIB)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_ZLIB)
    set(GRIDFORMAT_HAVE_ZLIB true)
endif ()

find_package(LZ4)
if (LZ4_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE LZ4::LZ4)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_LZ4)
    set(GRIDFORMAT_HAVE_LZ4 true)
endif ()

find_package(LZMA)
if (LZMA_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE LZMA::LZMA)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_LZMA)
    set(GRIDFORMAT_HAVE_LZMA true)
endif ()

find_package(MPI COMPONENTS CXX)
if (MPI_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE MPI::MPI_CXX)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_MPI)
    set(GRIDFORMAT_HAVE_MPI true)
endif ()

option(GRIDFORMAT_BUILD_HIGH_FIVE "Controls whether HighFive should be included in the build" ON)
set(HDF5_PREFER_PARALLEL true)
find_package(HDF5 QUIET)
find_package(HighFive QUIET)
set(HIGHFIVE_TARGET_NAME HighFive)
if (HighFive_FOUND)
    message(STATUS "Using preinstalled HighFive package")
    if (GRIDFORMAT_SUBPROJECT)
        # When using gridformat via fetchcontent (maybe also as git submodule),
        # cmake raises an error when downstream projects link against gridformat
        # in case highfive is found on the system (and thus not included again when)
        # configuring gridformat. The error says that high five targets are not found.
        # Another find_package call is required in the downstream project for some reason.
        # Without yet fully understanding why, it seems resolved if we link against HighFive_Highfive.
        # Interestingly, we could also not reproduce this locally with cmake 3.26, while in the CI
        # with cmake 3.22 the error occurs.
        set(HIGHFIVE_TARGET_NAME HighFive_HighFive)
    endif ()
elseif (GRIDFORMAT_BUILD_HIGH_FIVE)
    set(GRIDFORMAT_HIGH_FIVE_PATH "${CMAKE_CURRENT_LIST_DIR}/../deps/HighFive")
    if (NOT HDF5_FOUND)
        message(STATUS "Not including HighFive in the build as hdf5 was not found")
    elseif (EXISTS ${GRIDFORMAT_HIGH_FIVE_PATH}/CMakeLists.txt)
        find_package(Boost QUIET COMPONENTS system serialization)
        set(HIGHFIVE_USE_BOOST ${Boost_FOUND})
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


find_package(dune-localfunctions QUIET)
if (dune-localfunctions_FOUND)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS)
    set(GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS true)
endif ()
