// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <dune/grid/yaspgrid.hh>
#pragma GCC diagnostic pop

#include <gridformat/common/logging.hpp>
#include <gridformat/grid/adapters/dune.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>


// In the GitHub action runner we run into a compiler warning when
// using release flags. Locally, this could not be reproduced. For
// now, we simply ignore those warnings here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#include <gridformat/vtk/pvti_writer.hpp>
#pragma GCC diagnostic pop

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
        using Grid = Dune::YaspGrid<2, Dune::EquidistantCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{
            {1.0, 0.5}, {10, 12},
            std::bitset<2>{0ULL},  // no periodic boundaries
            int{0}                 // no overlap
        };
        GridFormat::PVTIWriter writer{grid.leafGridView(), mpi_helper.getCommunicator()};
        write<Grid>(writer, "dune_pvti_no_overlap", mpi_helper.rank());
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::EquidistantCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{
            {1.0, 0.5, 0.25}, {10, 12, 8},
            std::bitset<3>{0ULL},  // no periodic boundaries
            int{0}                 // no overlap
        };
        GridFormat::PVTIWriter writer{grid.leafGridView(), mpi_helper.getCommunicator()};
        write<Grid>(writer, "dune_pvti_no_overlap", mpi_helper.rank());
    }

    {
        using Grid = Dune::YaspGrid<2, Dune::EquidistantCoordinates<double, 2>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{
            {1.0, 0.5}, {10, 12},
            std::bitset<2>{0ULL},  // no periodic boundaries
            int{1}                 // with overlap
        };
        GridFormat::PVTIWriter writer{grid.leafGridView(), mpi_helper.getCommunicator()};
        write<Grid>(writer, "dune_pvti_with_overlap", mpi_helper.rank());
    }
    {
        using Grid = Dune::YaspGrid<3, Dune::EquidistantCoordinates<double, 3>>;
        static_assert(GridFormat::Concepts::ImageGrid<typename Grid::LeafGridView>);
        Grid grid{
            {1.0, 0.5, 0.25}, {10, 12, 8},
            std::bitset<3>{0ULL},  // no periodic boundaries
            int{1}                 // with overlap
        };
        GridFormat::PVTIWriter writer{grid.leafGridView(), mpi_helper.getCommunicator()};
        write<Grid>(writer, "dune_pvti_no_overlap", mpi_helper.rank());
    }

    return 0;
}
