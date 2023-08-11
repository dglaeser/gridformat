// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto size = GridFormat::Parallel::size(MPI_COMM_WORLD);

    {
        const auto grid = GridFormat::Test::make_unstructured_2d();
        GridFormat::VTKHDFWriter writer{grid, MPI_COMM_WORLD};
        GridFormat::Test::write_test_file<2>(
            writer, "pvtk_2d_in_2d_parallel_unstructured_nranks_" + std::to_string(size)
        );
    }

    {
        const auto grid = GridFormat::Test::make_unstructured_3d();
        GridFormat::VTKHDFWriter writer{grid, MPI_COMM_WORLD};
        GridFormat::Test::write_test_file<3>(
            writer, "pvtk_3d_in_3d_parallel_unstructured_nranks_" + std::to_string(size)
        );
    }

    MPI_Finalize();

    return 0;
}
