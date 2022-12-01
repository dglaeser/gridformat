// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>
#include <vector>
#include <ranges>
#include <algorithm>

#include <mpi.h>

#include <gridformat/grid.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

template<std::size_t dim, typename Grid, typename Writer>
void test(const Grid& grid, Writer& pvd_writer) {
    double sim_time = 0.0;
    auto test_data = GridFormat::Test::make_test_data<dim, double>(grid, sim_time);
    GridFormat::Test::add_test_data(pvd_writer, test_data, GridFormat::Precision<float>{});

    std::string filename = pvd_writer.write(sim_time);
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;

    const double end_time = 10.0;
    const double timestep_size = 1.0;
    while (sim_time < end_time - 1e-6) {
        sim_time += timestep_size;

        pvd_writer.clear();
        test_data = GridFormat::Test::make_test_data<dim, double>(grid, sim_time);
        GridFormat::Test::add_test_data(pvd_writer, test_data, GridFormat::Precision<float>{});

        filename = pvd_writer.write(sim_time);
        std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    const auto grid = GridFormat::Test::make_unstructured_2d();
    GridFormat::PVDWriter pvd_writer{GridFormat::PVTUWriter{grid, MPI_COMM_WORLD}, "pvd_parallel_time_series_2d_in_2d"};
    test<2>(grid, pvd_writer);

    MPI_Finalize();
    return 0;
}
