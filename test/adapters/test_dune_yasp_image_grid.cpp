// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dune.hpp>

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
#pragma GCC diagnostic pop
#endif  // GRIDFORMAT_HAVE_DUNE_ALUGRID

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/gridformat.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

#include "../make_test_data.hpp"


template<typename Grid, typename Writer>
std::string write(Writer& writer, const std::string& prefix) {
    static constexpr int dim = Grid::dimension;
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    const auto filename = writer.write(prefix + "_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d");
    std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
    return filename;
}


int main(int argc, char** argv) {
    Dune::MPIHelper::instance(argc, argv);

    {
        using Grid = Dune::YaspGrid<2, Dune::EquidistantCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{1.0, 2.0}, {3, 4}};
        const auto& grid_view = grid.leafGridView();
        GridFormat::VTIWriter writer{grid_view};
        const auto filename = write<Grid>(writer, "dune_vti_equidistant");

#if GRIDFORMAT_HAVE_DUNE_ALUGRID
        // test reading the grid via the grid factory and check the result
        GridFormat::Reader reader; reader.open(filename);
        const auto pfield = reader.point_field("pfunc")->export_to(std::vector<double>{});
        const auto cfield = reader.cell_field("cfunc")->export_to(std::vector<double>{});

        Dune::GridFactory<Dune::ALUGrid<2, 2, Dune::cube, Dune::nonconforming>> factory; {
            GridFormat::Dune::GridFactoryAdapter adapter{factory};
            reader.export_grid(adapter);
        }

        const auto check_equal = [&] <typename E> (const E& entity, const auto& container) {
            const auto x = container[factory.insertionIndex(entity)];
            const auto expected = GridFormat::Test::test_function<double>(entity.geometry().center());
            const std::string entity_type = E::Geometry::mydimension == 0 ? "cell" : "point";
            if (std::abs(expected - x) > 1e-5)
                throw GridFormat::InvalidState(
                    "Unexpected " + entity_type + " field value at "
                    + GridFormat::as_string(entity.geometry().center()) + ": "
                    + GridFormat::as_string(x) + " - " + GridFormat::as_string(expected)
                );
        };

        auto alu_grid = factory.createGrid();
        const auto& alu_grid_view = alu_grid->leafGridView();
        for (const auto& element : GridFormat::cells(alu_grid_view)) check_equal(element, cfield);
        for (const auto& vertex : GridFormat::points(alu_grid_view)) check_equal(vertex, pfield);
#endif  // GRIDFORMAT_HAVE_DUNE_ALUGRID
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::EquidistantCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{1.0, 2.0, 3.0}, {3, 4, 5}};
        const auto& grid_view = grid.leafGridView();
        GridFormat::VTIWriter writer{grid_view};
        write<Grid>(writer, "dune_vti_equidistant");
    }

    {
        using Grid = Dune::YaspGrid<2, Dune::EquidistantOffsetCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{0.5, 0.25}, {1.0, 2.0}, {4, 3}};
        const auto& grid_view = grid.leafGridView();
        GridFormat::VTIWriter writer{grid_view};
        write<Grid>(writer, "dune_vti_offset");
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::EquidistantOffsetCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{0.5, 0.25, 0.1}, {1.0, 2.0, 0.5}, {4, 3, 3}};
        const auto& grid_view = grid.leafGridView();
        GridFormat::VTIWriter writer{grid_view};
        write<Grid>(writer, "dune_vti_offset");
    }

    return 0;
}
