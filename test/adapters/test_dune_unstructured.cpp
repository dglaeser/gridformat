// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dune/grid/yaspgrid.hh>

#include <gridformat/grid/adapters/dune.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../make_test_data.hpp"

int main() {
    Dune::YaspGrid<2> grid{
        {1.0, 1.0},
        {10, 10}
    };

    const auto& grid_view = grid.leafGridView();
    GridFormat::VTUWriter writer{grid_view};
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    writer.write("dune_vtu_2d_in_2d");

    return 0;
}
