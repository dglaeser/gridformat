// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <dune/grid/yaspgrid.hh>
#pragma GCC diagnostic pop

#include <gridformat/grid/adapters/dune.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/vti_writer.hpp>
#include <gridformat/vtk/vtr_writer.hpp>

#include "../make_test_data.hpp"


template<typename Grid, typename Writer>
void write(Writer& writer) {
    static constexpr int dim = Grid::dimension;
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    std::cout << "Wrote '" << GridFormat::as_highlight(
        writer.write("dune_vtr_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d")
    ) << "'" << std::endl;
}


int main(int argc, char** argv) {
    Dune::MPIHelper::instance(argc, argv);

    {
        using Grid = Dune::YaspGrid<2, Dune::TensorProductCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::RectilinearGrid<typename Grid::LeafGridView>);
        Grid grid{
            {
                std::vector{0.1, 0.2, 1.0},
                std::vector{0.2, 0.4, 2.0}
            }
        };
        GridFormat::VTRWriter writer{grid.leafGridView()};
        write<Grid>(writer);
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::TensorProductCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::RectilinearGrid<typename Grid::LeafGridView>);
        Grid grid{
            {
                std::vector{0.1, 0.2, 1.0},
                std::vector{0.2, 0.4, 2.0},
                std::vector{0.05, 1.0, 2.0}
            }
        };
        GridFormat::VTRWriter writer{grid.leafGridView()};
        write<Grid>(writer);
    }

    return 0;
}
