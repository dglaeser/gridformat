// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main() {
    using GridFormat::Test::test_function;
    using GridFormat::Test::_compute_cell_center;

    double sim_time = 0.0;
    auto grid = GridFormat::Test::make_unstructured_2d<2>();
    auto series_writer = GridFormat::make_time_series_writer("generic_time_series_2d_in_2d", grid);
    series_writer.set_point_field("point_func", [&] (const auto& p) {
        return test_function<double>(GridFormat::coordinates(grid, p))*sim_time;
    });
    series_writer.set_cell_field("cell_func", [&] (const auto& c) {
        return test_function<double>(_compute_cell_center(grid, c))*sim_time;
    });
    series_writer.write(0.0);
    sim_time = 1.0;
    series_writer.write(1.0);

    return 0;
}
