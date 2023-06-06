// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <dune/grid/yaspgrid.hh>
#pragma GCC diagnostic pop

#include <gridformat/grid/adapters/dune.hpp>
#include <gridformat/grid/discontinuous.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../vtk/vtk_writer_tester.hpp"
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    Dune::MPIHelper::instance(argc, argv);
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

    GridFormat::VTPWriter poly_writer{grid_view};
    GridFormat::Test::add_meta_data(poly_writer);
    poly_writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    poly_writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    poly_writer.write("dune_vtu_2d_in_2d_as_poly");

    // test discontinuous output to check that wrapper works with dune grids
    GridFormat::DiscontinuousGrid discontinuous_grid{grid_view};
    GridFormat::VTUWriter discontinuous_writer{discontinuous_grid};
    GridFormat::Test::add_meta_data(discontinuous_writer);
    GridFormat::Test::add_discontinuous_point_field(discontinuous_writer);
    discontinuous_writer.set_point_field("accessor_test", [&] (const auto& p) {
        [[maybe_unused]] const auto& c = p.cell();
        [[maybe_unused]] const auto& hc = p.host_cell();
        [[maybe_unused]] const auto& i = p.index_in_host();
        return p.cell().index();
    });
    discontinuous_writer.write("dune_vtu_2d_in_2d_discontinuous");

    return 0;
}
