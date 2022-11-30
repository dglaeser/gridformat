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

if (${DUNE_FOUND})
    gridformat_add_regression_test_if(DUNE_FOUND test_dune_adapters test_dune_adapters.cpp "dune_vtu*")
    target_link_libraries(test_dune_adapters PRIVATE dunecommon dunegeometry dunegrid)
endif ()
