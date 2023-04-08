// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    double xoffset = static_cast<double>(rank%2);
    double yoffset = static_cast<double>(rank/2);
    {
        GridFormat::Test::VTK::WriterTester tester{
            GridFormat::Test::StructuredGrid<2>{
                {{1.0, 1.0}},
                {{10, 10}},
                {{xoffset, yoffset}}
            },
            ".pvtr",
            rank == 0
        };
        tester.test([&] (const auto& grid, const auto& opts) {
            return GridFormat::PVTRWriter{grid, MPI_COMM_WORLD, opts};
        });
    }
    {
        GridFormat::Test::StructuredGrid<2> grid{
            {{1.0, 1.0}},
            {{10, 10}},
            {{xoffset, yoffset}}
        };
        grid.invert();
        GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".pvtr", (rank == 0), "inverted"};
        tester.test([&] (const auto& grid, const auto& opts) {
            return GridFormat::PVTRWriter{grid, MPI_COMM_WORLD, opts};
        });
    }

    MPI_Finalize();
    return 0;
}
