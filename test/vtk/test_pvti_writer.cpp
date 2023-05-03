// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/pvti_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    {
        GridFormat::Test::VTK::WriterTester tester{
            GridFormat::Test::StructuredGrid<2>{
                {{1.0, 1.0}},
                {{10, 10}},
                {{xoffset, yoffset}}
            },
            ".pvti",
            rank == 0
        };
        tester.test([&] (const auto& grid, const auto& opts) {
            return GridFormat::PVTIWriter{grid, MPI_COMM_WORLD, opts};
        });
    }
    {
        GridFormat::Test::StructuredGrid<2> grid{
            {{1.0, 1.0}},
            {{10, 10}},
            {{xoffset, yoffset}}
        };
        grid.invert();
        GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".pvti", (rank == 0), "inverted"};
        tester.test([&] (const auto& grid, const auto& opts) {
            return GridFormat::PVTIWriter{grid, MPI_COMM_WORLD, opts};
        });
    }
    // the vtkPImageDataReader seems to not yet read the `Direction` attribute
    // (see https://gitlab.kitware.com/vtk/vtk/-/issues/18971)
    // Once clarity on this issue arises, we should test oriented images
    // {
    //     constexpr auto sqrt2_half = 1.0/std::numbers::sqrt2;
    //     double oriented_xoffset = xoffset*sqrt2_half - yoffset*sqrt2_half;
    //     double oriented_yoffset = xoffset*sqrt2_half - yoffset*sqrt2_half;
    //     GridFormat::Test::OrientedStructuredGrid<2> grid{
    //         {
    //             std::array<double, 2>{sqrt2_half, sqrt2_half},
    //             std::array<double, 2>{-sqrt2_half, sqrt2_half}
    //         },
    //         {{1.0, 1.0}},
    //         {{10, 10}},
    //         {{oriented_xoffset, oriented_yoffset}}
    //     };
    //     GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".pvti", (rank == 0), "oriented"};
    //     tester.test([&] (const auto& grid, const auto& opts) {
    //         return GridFormat::PVTIWriter{grid, MPI_COMM_WORLD, opts};
    //     });
    // }

    MPI_Finalize();
    return 0;
}
