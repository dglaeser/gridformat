// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

// include the traits first to see if they include everything
// they need, i.e. don't force users to include anything before
#include <gridformat/traits/dune.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <dune/grid/yaspgrid.hh>

#if GRIDFORMAT_HAVE_DUNE_FUNCTIONS
#include <dune/functions/gridfunctions/analyticgridviewfunction.hh>
#endif  // GRIDFORMAT_HAVE_DUNE_FUNCTIONS
#pragma GCC diagnostic pop

#include <gridformat/vtk/pvtu_writer.hpp>
#include "../vtk/vtk_writer_tester.hpp"
#include "../make_test_data.hpp"
#include "../testing.hpp"


template<typename Grid>
void run_unit_tests(const Grid& grid) {
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
    GridFormat::Test::add_meta_data(writer);
    writer.set_point_field("pfunc", [&] (const auto& vertex) {
        return GridFormat::Test::test_function<double>(vertex.geometry().center());
    });
    writer.set_cell_field("cfunc", [&] (const auto& element) {
        return GridFormat::Test::test_function<double>(element.geometry().center());
    });
    const auto filename = writer.write("dune_pvtu_2d_in_2d");
    if (GridFormat::Parallel::rank(mpi_helper.getCommunicator()) == 0)
        std::cout << "Wrote '" << filename << "'" << std::endl;

#if GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS
    for (unsigned int order : {1, 2, 3}) {
        GridFormat::Dune::LagrangePolynomialGrid lagrange_grid{grid_view, order};
        GridFormat::PVTUWriter lagrange_writer{lagrange_grid, mpi_helper.getCommunicator()};
        GridFormat::Test::add_meta_data(lagrange_writer);
        lagrange_writer.set_point_field("pfunc", [&] (const auto& point) {
            return GridFormat::Test::test_function<double>(point.coordinates);
        });
        lagrange_writer.set_cell_field("cfunc", [&] (const auto& element) {
            return GridFormat::Test::test_function<double>(element.geometry().center());
        });

#if GRIDFORMAT_HAVE_DUNE_FUNCTIONS
        using GridView = std::decay_t<decltype(grid_view)>;
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
        GridFormat::Dune::set_point_function(scalar, lagrange_writer, "dune_scalar_function");
        GridFormat::Dune::set_point_function(vector, lagrange_writer, "dune_vector_function");
        GridFormat::Dune::set_point_function(tensor, lagrange_writer, "dune_tensor_function");
        GridFormat::Dune::set_cell_function(scalar, lagrange_writer, "dune_scalar_cell_function");
        GridFormat::Dune::set_cell_function(vector, lagrange_writer, "dune_vector_cell_function");
        GridFormat::Dune::set_cell_function(tensor, lagrange_writer, "dune_tensor_cell_function");
#endif  // GRIDFORMAT_HAVE_DUNE_FUNCTIONS

        const auto lagrange_filename = lagrange_writer.write(
            "dune_pvtu_2d_in_2d_lagrange_order_" + std::to_string(order)
        );
        if (GridFormat::Parallel::rank(mpi_helper.getCommunicator()) == 0)
            std::cout << "Wrote '" << lagrange_filename << "'" << std::endl;
    }
#endif  // GRIDFORMAT_HAVE_DUNE_LOCALFUNCTIONS

    run_unit_tests(grid_view);

    return 0;
}
