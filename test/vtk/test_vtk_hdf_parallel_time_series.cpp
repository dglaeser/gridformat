// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    bool verbose = rank == 0;

    {
        const auto grid = GridFormat::Test::make_unstructured<2, 2>();
        GridFormat::VTKHDFTimeSeriesWriter writer{
            grid,
            MPI_COMM_WORLD,
            "pvtk_hdf_time_series_2d_in_2d_unstructured"
        };
        GridFormat::Test::write_test_time_series<2>(writer, 5, {}, verbose);
    }

    {
        const double xoffset = static_cast<double>(rank%2);
        const double yoffset = static_cast<double>(rank/2);
        GridFormat::Test::StructuredGrid<2> structured_grid{
            {1.0, 1.0},
            {5, 7},
            {xoffset, yoffset}
        };
        GridFormat::VTKHDFTimeSeriesWriter writer{
            structured_grid,
            MPI_COMM_WORLD,
            "pvtk_hdf_time_series_2d_in_2d_image"
        };
        GridFormat::Test::write_test_time_series<2>(writer, 5, {.write_meta_data = false}, verbose);
    }

    MPI_Finalize();
    return 0;
}
