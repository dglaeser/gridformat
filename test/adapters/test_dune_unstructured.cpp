// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dune.hpp>

#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <dune/grid/yaspgrid.hh>

#if GRIDFORMAT_HAVE_DUNE_ALUGRID
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wuse-after-free"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include <dune/alugrid/grid.hh>
#include <dune/grid/common/gridfactory.hh>
#include <dune/grid/io/file/gmshreader.hh>
#endif  // GRIDFORMAT_HAVE_DUNE_ALUGRID

#if GRIDFORMAT_HAVE_DUNE_FUNCTIONS
#include <dune/functions/gridfunctions/analyticgridviewfunction.hh>
#endif  // GRIDFORMAT_HAVE_DUNE_FUNCTIONS

#pragma GCC diagnostic pop

#include <gridformat/grid/discontinuous.hpp>

#include <gridformat/vtk/vtu_writer.hpp>
#include <gridformat/vtk/vtp_writer.hpp>

#include "../vtk/vtk_writer_tester.hpp"
#include "../make_test_data.hpp"
#include "../testing.hpp"

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

template<typename GridView>
void test_lagrange(const GridView& grid_view, const std::string& suffix = "") {
#if GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS
    std::string base_filename =
        "dune_vtu_lagrange_"
        + std::to_string(GridView::dimension) + "d_in_"
        + std::to_string(GridView::dimensionworld) + "d";
    base_filename += suffix.empty() ? "" : "_" + suffix;

    for (auto order : std::vector<unsigned int>{1, 2, 3}) {
        GridFormat::Dune::LagrangePolynomialGrid lagrange_grid{grid_view, order};
        GridFormat::VTUWriter writer{lagrange_grid, {.encoder = GridFormat::Encoding::ascii}};
        GridFormat::Test::add_meta_data(writer);
        writer.set_point_field("pfield", [&] (const auto& p) {
            return GridFormat::Test::test_function<double>(p.coordinates);
        });
        writer.set_cell_field("cfield", [&] (const auto& element) {
            return GridFormat::Test::test_function<double>(element.geometry().center());
        });
        writer.set_cell_field("cfield_from_element", [&] (const auto& element) {
            return GridFormat::Test::test_function<double>(element.geometry().center());
        });

#if GRIDFORMAT_HAVE_DUNE_FUNCTIONS
        auto scalar = Dune::Functions::makeAnalyticGridViewFunction([] (const auto& x) {
            return GridFormat::Test::test_function<double>(x);
        }, grid_view);
        auto vector = Dune::Functions::makeAnalyticGridViewFunction([] (const auto& x) {
            std::array<double, GridView::dimension> result;
            std::ranges::fill(result, GridFormat::Test::test_function<double>(x));
            return result;
        }, grid_view);
        auto tensor = Dune::Functions::makeAnalyticGridViewFunction([] (const auto& x) {
            std::array<double, GridView::dimension> row;
            std::ranges::fill(row, GridFormat::Test::test_function<double>(x));
            std::array<std::array<double, GridView::dimension>, GridView::dimension> result;
            std::ranges::fill(result, row);
            return result;
        }, grid_view);
        GridFormat::Dune::set_point_function(scalar, writer, "dune_scalar_function");
        GridFormat::Dune::set_point_function(vector, writer, "dune_vector_function");
        GridFormat::Dune::set_point_function(tensor, writer, "dune_tensor_function");
        GridFormat::Dune::set_cell_function(scalar, writer, "dune_scalar_cell_function");
        GridFormat::Dune::set_cell_function(vector, writer, "dune_vector_cell_function");
        GridFormat::Dune::set_cell_function(tensor, writer, "dune_tensor_cell_function");

        const auto prec = GridFormat::float32;
        GridFormat::Dune::set_point_function(scalar, writer, "dune_scalar_function_custom_prec", prec);
        GridFormat::Dune::set_cell_function(scalar, writer, "dune_scalar_cell_function_custom_prec", prec);

#endif  // GRIDFORMAT_HAVE_DUNE_FUNCTIONS

        std::cout << "Wrote '" << writer.write(base_filename + "_order_" + std::to_string(order)) << "'" << std::endl;
#endif  // GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS

        // run a bunch of unit tests
        using GridFormat::Testing::operator""_test;
        using GridFormat::Testing::expect;
        using GridFormat::Testing::eq;

        "lagrange_grid_num_cells"_test = [&] () {
            expect(eq(lagrange_grid.number_of_cells(), static_cast<std::size_t>(grid_view.size(0))));
        };

        "lagrange_grid_clear"_test = [&] () {
            lagrange_grid.clear();
            expect(eq(lagrange_grid.number_of_cells(), std::size_t{0}));
        };

        "lagrange_grid_update"_test = [&] () {
            lagrange_grid.update(grid_view);
            expect(eq(lagrange_grid.number_of_cells(), static_cast<std::size_t>(grid_view.size(0))));
        };
    }
}

int main(int argc, char** argv) {
    Dune::MPIHelper::instance(argc, argv);
    Dune::YaspGrid<2> grid_2d{{1.0, 1.0}, {2, 3}};
    Dune::YaspGrid<3> grid_3d{{1.0, 1.0, 1.0}, {2, 3, 2}};
    test(grid_2d.leafGridView());
    test(grid_3d.leafGridView());
    test_lagrange(grid_2d.leafGridView());
    test_lagrange(grid_3d.leafGridView());

#if GRIDFORMAT_HAVE_DUNE_ALUGRID
    using Grid2D = Dune::ALUGrid<2, 2, Dune::simplex, Dune::conforming>;
    Dune::GridFactory<Grid2D> factory_2d;
    Dune::GmshReader<Grid2D>::read(factory_2d, MESH_FILE_2D);
    const auto lagrange_grid_2d = std::shared_ptr<Grid2D>(factory_2d.createGrid());

    using Grid3D = Dune::ALUGrid<3, 3, Dune::simplex, Dune::conforming>;
    Dune::GridFactory<Grid3D> factory_3d;
    Dune::GmshReader<Grid3D>::read(factory_3d, MESH_FILE_3D);
    const auto lagrange_grid_3d = std::shared_ptr<Grid3D>(factory_3d.createGrid());

    // mesh file paths are defined in CMakeLists.txt
    test_lagrange(lagrange_grid_2d->leafGridView(), "triangles");
    test_lagrange(lagrange_grid_3d->leafGridView(), "tets");
#endif
    return 0;
}
