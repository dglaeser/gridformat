// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>
#include <gridformat/vtk/time_series_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto grid = GridFormat::Test::make_unstructured<2, 2>();
    GridFormat::VTKTimeSeriesWriter writer{
        GridFormat::VTKHDFWriter{grid, MPI_COMM_WORLD},
        "pvtk_hdf_time_series_2d_in_2d_unstructured"
    };

    GridFormat::Test::add_meta_data(writer);
    for (double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const auto test_data = GridFormat::Test::make_test_data<2, double>(grid, t);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
        const auto filename = writer.write(t);
        if (GridFormat::Parallel::rank(MPI_COMM_WORLD) == 0)
            std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
