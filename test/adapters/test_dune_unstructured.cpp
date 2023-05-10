// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <dune/grid/yaspgrid.hh>
#pragma GCC diagnostic pop

#include <gridformat/grid/adapters/dune.hpp>
#include <gridformat/vtk/vtu_writer.hpp>
#include "../vtk/vtk_writer_tester.hpp"
#include "../make_test_data.hpp"

int main() {
    Dune::YaspGrid<2> grid{
        {1.0, 1.0},
        {10, 10}
    };

    const auto& grid_view = grid.leafGridView();
    GridFormat::VTUWriter writer{grid_view};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    writer.write("dune_vtu_2d_in_2d");

    return 0;
}
