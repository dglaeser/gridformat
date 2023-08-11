# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

gridformat_add_test(test_unstructured_grid test_unstructured_grid.cpp)
gridformat_add_test(test_grid_writer test_grid_writer.cpp)
gridformat_add_test(test_grid_reader test_grid_reader.cpp)
gridformat_add_test(test_discontinuous_grid test_discontinuous_grid.cpp)
gridformat_add_regression_test(test_image_grid test_image_grid.cpp "*image_grid_test*")
gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_converter
    test_converter.cpp
    "converter_test_file*out*||converter_time_series_*.pvd"
    SCRIPT_ARGS "--skip-metadata"
)
gridformat_add_parallel_regression_test(
    test_parallel_converter
    test_parallel_converter.cpp
    2
    "parallel_converter_*out*||converter_parallel_time_series*.pvd"
    SCRIPT_ARGS "--skip-metadata"
)
