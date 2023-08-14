# SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: MIT

gridformat_add_regression_test(test_vti_writer test_vti_writer.cpp "vti_*")
gridformat_add_regression_test(test_vti_reader test_vti_reader.cpp "reader_vti_*")
target_compile_definitions(test_vti_reader PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_LIST_DIR}/test_data")

gridformat_add_regression_test(test_vtr_writer test_vtr_writer.cpp "vtr_*")
gridformat_add_regression_test(test_vtr_reader test_vtr_reader.cpp "reader_vtr_*")
target_compile_definitions(test_vtr_reader PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_LIST_DIR}/test_data")

gridformat_add_regression_test(test_vts_writer test_vts_writer.cpp "vts_*")
gridformat_add_regression_test(test_vts_reader test_vts_reader.cpp "reader_vts_*")
target_compile_definitions(test_vts_reader PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_LIST_DIR}/test_data")

gridformat_add_regression_test(test_vtu_writer test_vtu_writer.cpp "vtu_*")
gridformat_add_regression_test(test_vtu_reader test_vtu_reader.cpp "reader_vtu_*")
target_compile_definitions(test_vtu_reader PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_LIST_DIR}/test_data")

gridformat_add_parallel_regression_test(test_pvtu_writer test_pvtu_writer.cpp 2 "pvtu_*.pvtu")
gridformat_add_parallel_regression_test(test_pvtu_reader test_pvtu_reader.cpp 4 "reader_pvtu_*.pvtu")
gridformat_add_parallel_regression_test(test_pvti_reader test_pvti_reader.cpp 2 "reader_pvti_*.pvti||reader_pvti_*.vtu")
gridformat_add_parallel_regression_test(test_pvtr_reader test_pvtr_reader.cpp 2 "reader_pvtr_*.pvtr||reader_pvtr_*.vtu")
gridformat_add_parallel_regression_test(test_pvts_reader test_pvts_reader.cpp 2 "reader_pvts_*.pvts||reader_pvts_*.vtu")

gridformat_add_regression_test(test_vtp_writer test_vtp_writer.cpp "vtp_*")
gridformat_add_regression_test(test_vtp_reader test_vtp_reader.cpp "reader_vtp_*")
target_compile_definitions(test_vtp_reader PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_LIST_DIR}/test_data")

gridformat_add_parallel_regression_test(test_pvtp_writer test_pvtp_writer.cpp 2 "pvtp_*.pvtp")
gridformat_add_parallel_regression_test(test_pvtp_reader test_pvtp_reader.cpp 2 "reader_pvtp_*.pvtp")

gridformat_add_regression_test(test_vtu_time_series test_vtu_time_series.cpp "vtu_time_series*vtu")
gridformat_add_regression_test(test_pvd_writer test_pvd_writer.cpp "pvd_time_series*.pvd")
gridformat_add_regression_test(test_pvd_reader test_pvd_reader.cpp "reader_pvd_sequential*.pvd")
gridformat_add_parallel_regression_test(test_parallel_pvd_reader test_parallel_pvd_reader.cpp 2 "reader_pvd_parallel*.pvd")
gridformat_add_parallel_regression_test(test_parallel_pvd_writer test_parallel_pvd_writer.cpp 2 "pvd_parallel_*.pvd")

gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_unstructured_grid_writer
    test_vtk_hdf_unstructured_grid_writer.cpp
    "vtk_hdf_unstructured*hdf"
)

# TODO: when vtk 9.2.7 is out (with hdftransient support) do regression with time series also
gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_unstructured_grid_reader
    test_vtk_hdf_unstructured_grid_reader.cpp
    "reader_vtk_hdf_unstructured_test*hdf"
)

gridformat_add_parallel_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_parallel_unstructured_grid_reader
    test_vtk_hdf_parallel_unstructured_grid_reader.cpp
    2
)

# TODO: when vtk 9.2.7 is out (with hdftransient support) do regression
gridformat_add_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_time_series
    test_vtk_hdf_time_series.cpp
    # "vtk_hdf_time_series*.hdf"
)

# TODO: when vtk 9.2.7 is out (with hdftransient support) do regression
gridformat_add_parallel_test_if(
    GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
    test_vtk_hdf_parallel_time_series
    test_vtk_hdf_parallel_time_series.cpp
    2
    # "pvtk_hdf_time_series*.hdf"
)

# TODO: when vtk 9.2.7 is out (with the reader fixes) do regression also in 2D
gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_image_grid_writer
    test_vtk_hdf_image_grid_writer.cpp
    "vtk_hdf_image_3d*hdf"
    SCRIPT_ARGS "--skip-metadata"
)

# TODO: when vtk 9.2.7 is out (with hdftransient support) do regression for time series also
gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_image_grid_reader
    test_vtk_hdf_image_grid_reader.cpp
    "reader_vtk_hdf_structured_image_test*hdf"
    SCRIPT_ARGS "--skip-metadata"
)

set(RANKS 2;4)
foreach (NR IN LISTS RANKS)
    gridformat_add_parallel_regression_test(test_pvti_writer_nranks_${NR} test_pvti_writer.cpp ${NR} "pvti_*nranks_${NR}*.pvti")
    gridformat_add_parallel_regression_test(test_pvtr_writer_nranks_${NR} test_pvtr_writer.cpp ${NR} "pvtr_*nranks_${NR}*.pvtr")
    gridformat_add_parallel_regression_test(test_pvts_writer_nranks_${NR} test_pvts_writer.cpp ${NR} "pvts_*nranks_${NR}*.pvts")

    gridformat_add_parallel_regression_test_if(
        GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
        test_vtk_hdf_parallel_unstructured_grid_writer_nranks_${NR}
        test_vtk_hdf_parallel_unstructured_grid_writer.cpp
        ${NR}
        "pvtk_*unstructured*nranks_${NR}*hdf"
    )

    # TODO: once vtk9.2.7 (with the reader fixes) is out, regression-test 2d files also
    gridformat_add_parallel_regression_test_if(
        GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
        test_vtk_hdf_parallel_image_grid_writer_nranks_${NR}
        test_vtk_hdf_parallel_image_grid_writer.cpp
        ${NR}
        "pvtk_3d_*image*nranks_${NR}*hdf"
        SCRIPT_ARGS "--skip-metadata"
    )
endforeach ()
