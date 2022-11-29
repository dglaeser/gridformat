// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <mpi.h>

#include <gridformat/writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    using GridFormat::Test::test_function;
    using GridFormat::Test::_compute_cell_center;

    double sim_time = 0;
    auto grid = GridFormat::Test::make_unstructured_2d<2>();
    auto writer = GridFormat::make_time_series_writer("generic_parallel_time_series_2d_in_2d", grid, MPI_COMM_WORLD);
    writer.set_point_field("point_func", [&] (const auto& p) {
        return sim_time*test_function<double>(GridFormat::coordinates(grid, p));
    });
    writer.set_cell_field("cell_func", [&] (const auto& c) {
        return sim_time*test_function<double>(_compute_cell_center(grid, c));
    });
    writer.write(0.0);
    sim_time = 1.0;
    writer.write(1.0);

    MPI_Finalize();
    return 0;
}
