# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

include(GridFormatFetchDeps)

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

find_package(HighFive QUIET)
if (HighFive_FOUND)
    target_link_libraries(${PROJECT_NAME} INTERFACE HighFive)
    target_compile_definitions(${PROJECT_NAME} INTERFACE GRIDFORMAT_HAVE_HIGH_FIVE)
    set(GRIDFORMAT_HAVE_HIGH_FIVE true)
endif ()
