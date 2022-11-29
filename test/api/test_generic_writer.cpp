// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/writer.hpp>
#include "../grid/unstructured_grid.hpp"
#include "../make_test_data.hpp"

int main() {
    using GridFormat::Test::test_function;
    using GridFormat::Test::_compute_cell_center;

    auto grid = GridFormat::Test::make_unstructured_2d<2>();
    auto writer = GridFormat::make_writer(grid);
    writer.set_point_field("point_func", [&] (const auto& p) {
        return test_function<double>(GridFormat::coordinates(grid, p));
    });
    writer.set_cell_field("cell_func", [&] (const auto& c) {
        return test_function<double>(_compute_cell_center(grid, c));
    });
    writer.write("generic_writer_2d_in_2d");

    writer = GridFormat::make_writer(grid, GridFormat::FileFormat::vtu);
    writer.set_point_field("point_func", [&] (const auto& p) {
        return test_function<double>(GridFormat::coordinates(grid, p));
    });
    writer.set_cell_field("cell_func", [&] (const auto& c) {
        return test_function<double>(_compute_cell_center(grid, c));
    });
    writer.write("generic_writer_2d_in_2d");

    return 0;
}
