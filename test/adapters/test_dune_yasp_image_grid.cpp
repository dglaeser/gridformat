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

#include <gridformat/common/logging.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

// In the GitHub action runner we run into a compiler warning when
// using release flags. Locally, this could not be reproduced. For
// now, we simply ignore those warnings here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#include <gridformat/vtk/vti_writer.hpp>
#pragma GCC diagnostic pop

#include "../make_test_data.hpp"


template<typename Grid, typename Writer>
void write(Writer& writer, const std::string& prefix) {
    static constexpr int dim = Grid::dimension;
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    std::cout << "Wrote '" << GridFormat::as_highlight(
        writer.write(prefix + "_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d")
    ) << "'" << std::endl;
}


int main(int argc, char** argv) {
    Dune::MPIHelper::instance(argc, argv);

    {
        using Grid = Dune::YaspGrid<2, Dune::EquidistantCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{1.0, 2.0}, {3, 4}};
        GridFormat::VTIWriter writer{grid.leafGridView()};
        write<Grid>(writer, "dune_vti_equidistant");
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::EquidistantCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{1.0, 2.0, 3.0}, {3, 4, 5}};
        GridFormat::VTIWriter writer{grid.leafGridView()};
        write<Grid>(writer, "dune_vti_equidistant");
    }

    {
        using Grid = Dune::YaspGrid<2, Dune::EquidistantOffsetCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{0.5, 0.25}, {1.0, 2.0}, {4, 3}};
        GridFormat::VTIWriter writer{grid.leafGridView()};
        write<Grid>(writer, "dune_vti_offset");
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::EquidistantOffsetCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{{0.5, 0.25, 0.1}, {1.0, 2.0, 0.5}, {4, 3, 3}};
        GridFormat::VTIWriter writer{grid.leafGridView()};
        write<Grid>(writer, "dune_vti_offset");
    }

    return 0;
}
