// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/gridformat.hpp>

#include "../make_test_data.hpp"

template<typename Writer>
void write(Writer&& writer) {
    const auto& grid = writer.grid();
    GridFormat::Test::add_meta_data(writer);
    for (double sim_time : {0.0, 0.5, 1.0}) {
        writer.set_point_field("point_func", [&] (const auto& p) {
            return GridFormat::Test::test_function<double>(grid.position(p))*sim_time;
        });
        writer.set_cell_field("cell_func", [&] (const auto& c) {
            return GridFormat::Test::test_function<double>(grid.center(c))*sim_time;
        });
        std::cout << "Writing at t = " << sim_time << std::endl;
        std::cout << "Wrote '" << GridFormat::as_highlight(writer.write(sim_time)) << "'" << std::endl;
    }
}

int main() {
    using GridFormat::Test::test_function;

    GridFormat::ImageGrid<2, double> grid{{1.0, 1.0}, {4, 5}};
    write(GridFormat::Writer{GridFormat::pvd(GridFormat::vtu), grid, "generic_time_series_2d_in_2d_vtu"});
    write(GridFormat::Writer{GridFormat::pvd(GridFormat::vti), grid, "generic_time_series_2d_in_2d_vti"});
    write(GridFormat::Writer{GridFormat::pvd(GridFormat::vtr), grid, "generic_time_series_2d_in_2d_vtr"});
    write(GridFormat::Writer{GridFormat::pvd(GridFormat::vts({.encoder = GridFormat::Encoding::ascii})), grid, "generic_time_series_2d_in_2d_vts"});

#if GRIDFORMAT_HAVE_HIGH_FIVE
    write(GridFormat::Writer{GridFormat::pvd(GridFormat::vtk_hdf), grid, "generic_time_series_2d_in_2d_unstructured_vtk_hdf"});
#endif

    return 0;
}
