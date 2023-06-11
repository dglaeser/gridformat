# SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
# SPDX-License-Identifier: GPL-3.0-or-later

gridformat_add_regression_test(test_vti_writer test_vti_writer.cpp "vti_*")
gridformat_add_regression_test(test_vtr_writer test_vtr_writer.cpp "vtr_*")
gridformat_add_regression_test(test_vts_writer test_vts_writer.cpp "vts_*")
gridformat_add_regression_test(test_vtu_writer test_vtu_writer.cpp "vtu_*")
gridformat_add_regression_test(test_pvd_writer test_pvd_writer.cpp "pvd_time_series*.pvd")
gridformat_add_regression_test(test_vtp_writer test_vtp_writer.cpp "vtp_*")
gridformat_add_regression_test(test_vtu_time_series test_vtu_time_series.cpp "vtu_time_series*vtu")
gridformat_add_parallel_regression_test(test_pvtp_writer test_pvtp_writer.cpp 2 "pvtp_*.pvtp")
gridformat_add_parallel_regression_test(test_pvtu_writer test_pvtu_writer.cpp 2 "pvtu_*.pvtu")
gridformat_add_parallel_regression_test(test_parallel_pvd_writer test_parallel_pvd_writer.cpp 2 "pvd_parallel_*.pvd")

gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_unstructured_grid_writer
    test_vtk_hdf_unstructured_grid_writer.cpp
    "vtk_hdf_unstructured*hdf"
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

gridformat_add_regression_test_if(
    GRIDFORMAT_HAVE_HIGH_FIVE
    test_vtk_hdf_image_grid_writer
    test_vtk_hdf_image_grid_writer.cpp
    "vtk_hdf_image*hdf"
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

    gridformat_add_parallel_regression_test_if(
        GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
        test_vtk_hdf_parallel_image_grid_writer_nranks_${NR}
        test_vtk_hdf_parallel_image_grid_writer.cpp
        ${NR}
        "pvtk_*image*nranks_${NR}*hdf"
        SCRIPT_ARGS "--skip-metadata"
    )
endforeach ()
