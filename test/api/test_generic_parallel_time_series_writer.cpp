// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <mpi.h>

#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"

template<typename Writer, typename Comm>
void write(Writer&& writer, const Comm& comm, std::string suffix = "") {
    const auto& grid = writer.grid();
    GridFormat::Test::add_meta_data(writer);

    for (double time_value : {0.0, 0.5, 1.0}) {
        writer.set_point_field("point_func", [&] (const auto& p) {
            return GridFormat::Test::test_function<double>(grid.position(p))*time_value;
        });
        writer.set_cell_field("cell_func", [&] (const auto& c) {
            return GridFormat::Test::test_function<double>(grid.center(c))*time_value;
        });

        suffix = suffix != "" ? "_" + suffix : "";
        const auto filename = writer.write(time_value);
        if (GridFormat::Parallel::rank(comm) == 0)
            std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    if (GridFormat::Parallel::size(MPI_COMM_WORLD)%2 != 0)
        throw GridFormat::ValueError("Communicator size must be a multiple of 2");

    int rank = GridFormat::Parallel::rank(MPI_COMM_WORLD);
    const double xoffset = static_cast<double>(rank%2);
    const double yoffset = static_cast<double>(rank/2);
    GridFormat::ImageGrid<2, double> grid{
        {xoffset, yoffset},
        {1.0, 1.0},
        {10, 15}
    };

    write(
        GridFormat::Writer{
            GridFormat::pvd(GridFormat::vtu),
            grid,
            MPI_COMM_WORLD,
            "generic_parallel_time_series_2d_in_2d_vtu"
        },
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{
            GridFormat::pvd(GridFormat::vti),
            grid,
            MPI_COMM_WORLD,
            "generic_parallel_time_series_2d_in_2d_vti"
        },
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{
            GridFormat::pvd(GridFormat::vtr),
            grid,
            MPI_COMM_WORLD,
            "generic_parallel_time_series_2d_in_2d_vtr"
        },
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{
            GridFormat::pvd(GridFormat::vts),
            grid,
            MPI_COMM_WORLD,
            "generic_parallel_time_series_2d_in_2d_vts"
        },
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{
            GridFormat::pvd(GridFormat::vtp),
            grid,
            MPI_COMM_WORLD,
            "generic_parallel_time_series_2d_in_2d_vtp"
        },
        MPI_COMM_WORLD
    );
    write(
        GridFormat::Writer{
            GridFormat::time_series(GridFormat::vtk_hdf),
            grid,
            MPI_COMM_WORLD,
            "generic_parallel_time_series_2d_in_2d_vtkhdf_unstructured_pvd"
        },
        MPI_COMM_WORLD
    );

    MPI_Finalize();
    return 0;
}
