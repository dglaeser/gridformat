// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dune.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <dune/grid/yaspgrid.hh>
#pragma GCC diagnostic pop

#include <gridformat/common/logging.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/pvtr_writer.hpp>

#include "../make_test_data.hpp"


template<typename Grid, typename Writer>
void write(Writer& writer, const std::string& prefix, int rank) {
    static constexpr int dim = Grid::dimension;
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    const auto filename = writer.write(prefix + "_" + std::to_string(dim) + "d_in_" + std::to_string(dim) + "d");
    if (rank == 0)
        std::cout << "Wrote '" << GridFormat::as_highlight(filename) << "'" << std::endl;
}


int main(int argc, char** argv) {
    const auto& mpi_helper = Dune::MPIHelper::instance(argc, argv);

    {
        using Grid = Dune::YaspGrid<2, Dune::TensorProductCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::RectilinearGrid<typename Grid::LeafGridView>);
        Grid grid{
            {
                std::vector{0.0, 0.1, 0.3, 0.6, 1.0, 1.6, 2.4},
                std::vector{0.0, 0.05, 0.15, 0.4, 0.8, 1.2, 1.9}
            },
            std::bitset<2>{0ULL},  // no periodic boundaries
            int{0}                 // no overlap
        };
        const auto& grid_view = grid.leafGridView();
        GridFormat::PVTRWriter writer{grid_view, mpi_helper.getCommunicator()};
        write<Grid>(writer, "dune_pvtr_no_overlap", mpi_helper.rank());
    }
    {
        using Grid = Dune::YaspGrid<2, Dune::TensorProductCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::RectilinearGrid<typename Grid::LeafGridView>);
        Grid grid{
            {
                std::vector{0.0, 0.1, 0.3, 0.6, 1.0, 1.6, 2.4, 3.2, 4.2},
                std::vector{0.0, 0.05, 0.15, 0.4, 0.8, 1.2, 1.9, 2.7, 3.8}
            },
            std::bitset<2>{0ULL},  // no periodic boundaries
            int{1}                 // no overlap
        };
        const auto& grid_view = grid.leafGridView();
        GridFormat::PVTRWriter writer{grid_view, mpi_helper.getCommunicator()};
        write<Grid>(writer, "dune_pvtr_with_overlap", mpi_helper.rank());
    }

    return 0;
}
