# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

gridformat_add_test(test_unstructured_grid test_unstructured_grid.cpp)
gridformat_add_test(test_grid_writer test_grid_writer.cpp)
gridformat_add_test(test_discontinuous_grid test_discontinuous_grid.cpp)
gridformat_add_regression_test(test_image_grid test_image_grid.cpp "*image_grid_test*")
gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_converter
    test_converter.cpp
    "converter_*out*"
    SCRIPT_ARGS "--skip-metadata"
)
