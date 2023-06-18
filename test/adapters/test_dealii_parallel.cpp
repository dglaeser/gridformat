// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dealii.hpp>

#include <array>
#include <mpi.h>

#include <deal.II/distributed/tria.h>
#include <deal.II/distributed/fully_distributed_tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_description.h>

#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/pvtu_writer.hpp>
#include <gridformat/vtk/pvtp_writer.hpp>

#include "../make_test_data.hpp"
#include "../testing.hpp"

template<int dim, typename T>
auto as_array(const dealii::Point<dim, T>& p) {
    std::array<T, dim> result;
    for (int i = 0; i < dim; ++i)
        result[i] = p[i];
    return result;
}

template<typename Writer>
void add_fields_and_write(Writer& writer, const std::string& filename) {
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(as_array(vertex.center()));
    });
    writer.set_cell_field("cfunc", [&] (const auto& cell) {
        return GridFormat::Test::test_function<double>(as_array(cell.center()));
    });
    const auto written_filename = writer.write(filename);
    std::cout << "Wrote '" << GridFormat::as_highlight(written_filename) << "'" << std::endl;
}

template<template<typename, typename> typename Writer = GridFormat::PVTUWriter, typename Grid>
void write(const Grid& grid, const std::string& prefix) {
    auto writer = Writer{grid, grid.get_communicator()};
    add_fields_and_write(
        writer,
        prefix + "_" + std::to_string(Grid::dimension) + "d_in_"
                     + std::to_string(Grid::space_dimension) + "d"
    );

    // Run a bunch of unit tests with the given grid
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "number_of_cells"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::cells(grid))),
            GridFormat::Traits::NumberOfCells<Grid>::get(grid)
        ));
    };
    "number_of_vertices"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid))),
            GridFormat::Traits::NumberOfPoints<Grid>::get(grid)
        ));
    };
    "number_of_cell_points"_test = [&] () {
        for (const auto& c : GridFormat::cells(grid))
            expect(eq(
                static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid, c))),
                GridFormat::Traits::NumberOfCellPoints<Grid, std::decay_t<decltype(c)>>::get(grid, c)
            ));
    };
}

template<int d, int sd, typename Action>
auto apply_to_triangulation(MPI_Comm comm, const Action& action) {
    dealii::parallel::distributed::Triangulation<d, sd> tria{comm};
    dealii::GridGenerator::hyper_cube(tria);
    tria.refine_global(2);
    tria.repartition();
    action(tria);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    apply_to_triangulation<2, 2>(MPI_COMM_WORLD, [] (const auto& grid) { write(grid, "dealii_pvtu"); });
    apply_to_triangulation<2, 3>(MPI_COMM_WORLD, [] (const auto& grid) { write(grid, "dealii_pvtu"); });
    apply_to_triangulation<3, 3>(MPI_COMM_WORLD, [] (const auto& grid) { write(grid, "dealii_pvtu"); });
    apply_to_triangulation<3, 3>(MPI_COMM_WORLD, [] (const auto& grid) {
        write<GridFormat::PVTPWriter>(grid, "dealii_pvtu_as_poly");
    });
    apply_to_triangulation<3, 3>(MPI_COMM_WORLD, [] (const auto& grid) {
        dealii::parallel::fullydistributed::Triangulation<3, 3> fully_distributed{MPI_COMM_WORLD};
        fully_distributed.create_triangulation(
            dealii::TriangulationDescription::Utilities::create_description_from_triangulation(grid, MPI_COMM_WORLD)
        );
        write(fully_distributed, "dealii_pvtu_fully_dist");
    });

    MPI_Finalize();
    return 0;
}
