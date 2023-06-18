// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <mpi.h>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>

#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    GridFormat::Test::VTK::WriterTester tester{
        GridFormat::Test::make_unstructured_2d<2>(),
        ".pvtu",
        GridFormat::Parallel::rank(MPI_COMM_WORLD) == 0
    };
    tester.test([&] (const auto& grid, const auto& opts) {
        return GridFormat::PVTUWriter{grid, MPI_COMM_WORLD, opts};
    });

    MPI_Finalize();
    return 0;
}
