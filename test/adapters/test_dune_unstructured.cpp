// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dune.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <dune/grid/yaspgrid.hh>
#pragma GCC diagnostic pop

#include <gridformat/grid/discontinuous.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../vtk/vtk_writer_tester.hpp"
#include "../make_test_data.hpp"

template<typename GridView>
void test(const GridView& grid_view) {
    std::string base_filename =
        "dune_vtu_"
        + std::to_string(GridView::dimension) + "d_in_"
        + std::to_string(GridView::dimensionworld) + "d";

    GridFormat::VTUWriter writer{grid_view};
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    std::cout << "Wrote '" << writer.write(base_filename) << "'" << std::endl;

    if (GridView::dimension < 3) {
        GridFormat::VTPWriter poly_writer{grid_view};
        GridFormat::Test::add_meta_data(poly_writer);
        poly_writer.set_point_field("pfunc", [&] (const auto& vertex) {
            return GridFormat::Test::test_function<double>(vertex.geometry().center());
        });
        poly_writer.set_cell_field("cfunc", [&] (const auto& element) {
            return GridFormat::Test::test_function<double>(element.geometry().center());
        });
        std::cout << "Wrote '" << poly_writer.write(base_filename + "_as_poly") << "'" << std::endl;
    }

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
    std::cout << "Wrote '" << discontinuous_writer.write(base_filename + "_discontinuous") << "'" << std::endl;
}

int main(int argc, char** argv) {
    Dune::MPIHelper::instance(argc, argv);
    Dune::YaspGrid<2> grid_2d{{1.0, 1.0}, {6, 7}};
    Dune::YaspGrid<3> grid_3d{{1.0, 1.0, 1.0}, {5, 6, 7}};
    test(grid_2d.leafGridView());
    test(grid_3d.leafGridView());
    return 0;
}
