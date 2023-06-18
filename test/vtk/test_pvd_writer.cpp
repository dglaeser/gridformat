// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/pvd_writer.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"
#include "vtk_writer_tester.hpp"

template<std::size_t dim, typename Grid>
void test(const Grid& grid) {
    double sim_time = 0.0;
    GridFormat::PVDWriter pvd_writer{GridFormat::VTUWriter{grid}, "pvd_time_series_2d_in_2d"};
    auto test_data = GridFormat::Test::make_test_data<dim, double>(grid, sim_time);
    GridFormat::Test::add_test_data(pvd_writer, test_data, GridFormat::Precision<float>{});
    GridFormat::Test::add_meta_data(pvd_writer);

    std::string filename = pvd_writer.write(sim_time);
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;

    const double end_time = 10.0;
    const double timestep_size = 1.0;
    while (sim_time < end_time - 1e-6) {
        sim_time += timestep_size;
        test_data = GridFormat::Test::make_test_data<dim, double>(grid, sim_time);
        filename = pvd_writer.write(sim_time);
        std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    }
}

int main() {
    const auto grid = GridFormat::Test::make_unstructured_2d();
    test<2>(grid);
    return 0;
}
