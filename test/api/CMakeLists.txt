# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

gridformat_add_regression_test(test_generic_writer test_generic_writer.cpp "generic_2d_in_2d*")
gridformat_add_regression_test(test_generic_time_series_writer test_generic_time_series_writer.cpp "generic_time_series_2d_in_2d*pvd*")
gridformat_add_regression_test(
    test_generic_converter
    test_generic_converter.cpp
    "generic_converter*out*||generic_time_series_converter*.pvd"
    SCRIPT_ARGS "--skip-metadata"
)

gridformat_add_parallel_regression_test(test_generic_parallel_writer test_generic_parallel_writer.cpp 2 "generic_parallel_2d_in_2d*.p*")
gridformat_add_parallel_regression_test(test_generic_parallel_time_series_writer test_generic_parallel_time_series_writer.cpp 2 "generic_parallel_time_series_2d_in_2d*pvd*")
gridformat_add_parallel_regression_test(
    test_generic_parallel_converter
    test_generic_converter.cpp
    2
    "generic_parallel_converter*out*||generic_parallel_time_series_converter*pvd"
    SCRIPT_ARGS "--skip-metadata"
)
target_compile_definitions(test_generic_parallel_converter PRIVATE RUN_PARALLEL=1)

gridformat_add_test(test_generic_reader test_generic_reader.cpp)
gridformat_add_parallel_test(test_generic_reader_parallel test_generic_reader.cpp 2)
target_compile_definitions(test_generic_reader PRIVATE TEST_VTK_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../vtk/test_data")
target_compile_definitions(
    test_generic_reader_parallel
    PRIVATE
        TEST_VTK_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../vtk/test_data"
        RUN_PARALLEL=1
)
