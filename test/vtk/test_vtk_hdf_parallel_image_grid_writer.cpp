// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <mpi.h>
#include <numbers>

#include <gridformat/grid/type_traits.hpp>
#include <gridformat/vtk/hdf_writer.hpp>

#include "../grid/structured_grid.hpp"
#include "../make_test_data.hpp"

template<typename Grid, typename Communicator>
void _test(const Grid& grid, const Communicator& comm, const std::string& filename) {
    const bool verbose = GridFormat::Parallel::rank(comm) == 0;
    GridFormat::VTKHDFWriter writer{grid, comm};
    GridFormat::Test::write_test_file<GridFormat::dimension<Grid>>(
        writer,
        filename,
        {},
        verbose
    );
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int num_ranks = GridFormat::Parallel::size(MPI_COMM_WORLD);
    if (num_ranks%2 != 0)
        throw GridFormat::ValueError("Number of ranks must be a multiple of 2");

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3})
            _test(
                GridFormat::Test::StructuredGrid<2>{{{1.0, 1.0}}, {{nx, ny}}, {{xoffset, yoffset}}},
                MPI_COMM_WORLD,
                std::string{"pvtk_2d_in_2d_image_nranks"}
                + "_" + std::to_string(num_ranks)
                + "_" + std::to_string(nx)
                + "_" + std::to_string(ny)
            );

    for (std::size_t nx : {2})
        for (std::size_t ny : {2, 3})
            for (std::size_t nz : {2, 4}) {
                _test(
                    GridFormat::Test::StructuredGrid<3>{
                        {{1.0, 1.0, 1.0}},
                        {{nx, ny, nz}},
                        {{xoffset, yoffset, 0.0}}
                    },
                    MPI_COMM_WORLD,
                    std::string{"pvtk_3d_in_3d_image_nranks"}
                    + "_" + std::to_string(num_ranks)
                    + "_" + std::to_string(nx)
                    + "_" + std::to_string(ny)
                    + "_" + std::to_string(nz)
                );

                _test(
                    GridFormat::Test::StructuredGrid<3>{
                        {{1.0, 1.0, 1.0}},
                        {{nx, ny, nz}},
                        {{xoffset, 0.0, yoffset}}
                    },
                    MPI_COMM_WORLD,
                    std::string{"pvtk_3d_in_3d_image_z_decomposition_nranks"}
                    + "_" + std::to_string(num_ranks)
                    + "_" + std::to_string(nx)
                    + "_" + std::to_string(ny)
                    + "_" + std::to_string(nz)
                );
            }

    constexpr auto sqrt2_half = 1.0/std::numbers::sqrt2;
    double oriented_xoffset = xoffset*sqrt2_half - yoffset*sqrt2_half;
    double oriented_yoffset = xoffset*sqrt2_half + yoffset*sqrt2_half;
    _test(
        GridFormat::Test::OrientedStructuredGrid<2>{
            {
                std::array<double, 2>{sqrt2_half, sqrt2_half},
                std::array<double, 2>{-sqrt2_half, sqrt2_half}
            },
            {{1.0, 1.0}},
            {{2, 3}},
            {{oriented_xoffset, oriented_yoffset}}
        },
        MPI_COMM_WORLD,
        "pvtk_2d_in_2d_image_oriented_nranks_" + std::to_string(num_ranks)
    );

    _test(
        GridFormat::Test::OrientedStructuredGrid<3>{
            {
                std::array<double, 3>{sqrt2_half, sqrt2_half, 0.0},
                std::array<double, 3>{-sqrt2_half, sqrt2_half, 0.0},
                std::array<double, 3>{0.0, 0.0, 1.0}
            },
            {{1.0, 1.0, 1.0}},
            {{2, 3, 4}},
            {{oriented_xoffset, oriented_yoffset, 0.0}}
        },
        MPI_COMM_WORLD,
        "pvtk_3d_in_3d_image_oriented_nranks_" + std::to_string(num_ranks)
    );

    MPI_Finalize();
    return 0;
}
