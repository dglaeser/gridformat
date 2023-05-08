# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

option(GRIDFORMAT_FETCH_DEPENDENCIES "automatically fetch dependencies" OFF)
if (GRIDFORMAT_FETCH_DEPENDENCIES)

    find_package(HDF5 QUIET)
    find_package(Boost QUIET COMPONENTS system serialization)
    if (HDF5_FOUND AND Boost_FOUND)
        set(GRIDFORMAT_HIGH_FIVE_VERSION "v2.7.1")
        message(STATUS "Fetching HighFive at ${GRIDFORMAT_HIGH_FIVE_VERSION}")
        include(FetchContent)
        FetchContent_Declare(
            HighFive
            GIT_REPOSITORY https://github.com/BlueBrain/HighFive.git
            GIT_TAG ${GRIDFORMAT_HIGH_FIVE_VERSION}
            FIND_PACKAGE_ARGS
        )
        FetchContent_MakeAvailable(HighFive)
    endif ()
endif ()
