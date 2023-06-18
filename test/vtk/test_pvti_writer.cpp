// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>

#include <mpi.h>

#include <gridformat/common/logging.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/vtk/pvti_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

template<typename Grid, typename Communicator>
void _test(Grid&& grid, const Communicator& comm, std::string suffix = "") {
    bool verbose = GridFormat::Parallel::rank(MPI_COMM_WORLD) == 0;
    GridFormat::Test::VTK::WriterTester tester{std::move(grid), ".pvti", verbose, suffix};
    tester.test([&] (const auto& grid, const auto& xml_opts) {
        return GridFormat::PVTIWriter{grid, comm, xml_opts};
    });
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    if (GridFormat::Parallel::size(MPI_COMM_WORLD)%2 != 0)
        throw GridFormat::ValueError("Communicator size must be a multiple of 2");

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3}) {
            const auto base_suffix = std::to_string(nx)
                + "_" + std::to_string(ny)
                + "_nranks_" + std::to_string(GridFormat::Parallel::size(MPI_COMM_WORLD));
            _test(
                GridFormat::Test::StructuredGrid<2>{
                    {{1.0, 1.0}},
                    {{nx, ny}},
                    {{xoffset, yoffset}}
                },
                MPI_COMM_WORLD,
                base_suffix
            );

            GridFormat::Test::StructuredGrid<2> inverted{
                {{1.0, 1.0}},
                {{nx, ny}},
                {{xoffset, yoffset}}
            };
            inverted.invert();
            _test(std::move(inverted), MPI_COMM_WORLD, base_suffix + "_inverted");
        }

    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3})
            for (std::size_t nz : {2, 4}) {
                const auto base_suffix = std::to_string(nx)
                    + "_" + std::to_string(ny)
                    + "_" + std::to_string(nz)
                    + "_nranks_" + std::to_string(GridFormat::Parallel::size(MPI_COMM_WORLD));
                _test(
                    GridFormat::Test::StructuredGrid<3>{
                        {{1.0, 1.0, 1.0}},
                        {{nx, ny, nz}},
                        {{xoffset, yoffset, 0.0}}
                    },
                    MPI_COMM_WORLD,
                    base_suffix
                );
                {
                    GridFormat::Test::StructuredGrid<3> inverted{
                        {{1.0, 1.0, 1.0}},
                        {{nx, ny, nz}},
                        {{xoffset, yoffset, 0.0}}
                    };
                    inverted.invert();
                    _test(std::move(inverted), MPI_COMM_WORLD, base_suffix + "_inverted");
                }

                _test(
                    GridFormat::Test::StructuredGrid<3>{
                        {{1.0, 1.0, 1.0}},
                        {{nx, ny, nz}},
                        {{xoffset, 0.0, yoffset}}
                    },
                    MPI_COMM_WORLD,
                    base_suffix + "_z_decomposition"
                );
                {
                    GridFormat::Test::StructuredGrid<3> inverted{
                        {{1.0, 1.0, 1.0}},
                        {{nx, ny, nz}},
                        {{xoffset, 0.0, yoffset}}
                    };
                    inverted.invert();
                    _test(std::move(inverted), MPI_COMM_WORLD, base_suffix + "_z_decomposition_inverted");
                }
        }

    // the vtkPImageDataReader seems to not yet read the `Direction` attribute
    // (see https://gitlab.kitware.com/vtk/vtk/-/issues/18971)
    // Once clarity on this issue arises, we should test oriented images
    // {
    //     constexpr auto sqrt2_half = 1.0/std::numbers::sqrt2;
    //     double oriented_xoffset = xoffset*sqrt2_half - yoffset*sqrt2_half;
    //     double oriented_yoffset = xoffset*sqrt2_half + yoffset*sqrt2_half;
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
