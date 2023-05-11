// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto size = GridFormat::Parallel::size(MPI_COMM_WORLD);
    const auto grid = GridFormat::Test::make_unstructured_2d();

    GridFormat::VTKHDFWriter writer{grid, MPI_COMM_WORLD};

    {
        auto test_data = GridFormat::Test::make_test_data<2, double>(grid);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
        GridFormat::Test::add_meta_data(writer);
        writer.write("pvtk_2d_in_2d_parallel_unstructured_nranks_" + std::to_string(size));
    }

    {
        auto test_data = GridFormat::Test::make_test_data<3, double>(grid);
        GridFormat::Test::add_test_data(writer, test_data, GridFormat::Precision<float>{});
        GridFormat::Test::add_meta_data(writer);
        writer.write("pvtk_3d_in_3d_parallel_unstructured_nranks_" + std::to_string(size));
    }

    MPI_Finalize();

    return 0;
}
