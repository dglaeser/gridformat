// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dune/grid/yaspgrid.hh>

#include <gridformat/grid/adapters/dune.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include "../make_test_data.hpp"

int main(int argc, char** argv) {
    const auto& mpi_helper = Dune::MPIHelper::instance(argc, argv);

    Dune::YaspGrid<2> grid{
        {1.0, 1.0},
        {10, 10},
        std::bitset<2>{0ULL},  // no periodic boundaries
        int{0}                 // no overlap
    };
    grid.loadBalance();

    const auto& grid_view = grid.leafGridView();
    GridFormat::PVTUWriter writer{grid_view, mpi_helper.getCommunicator()};
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    writer.write("dune_pvtu_2d_in_2d");

    return 0;
}
