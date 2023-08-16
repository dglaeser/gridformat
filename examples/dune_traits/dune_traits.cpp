// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>

#include <dune/grid/yaspgrid.hh>
#include <dune/functions/gridfunctions/analyticgridviewfunction.hh>

#include <gridformat/traits/dune.hpp>
#include <gridformat/grid/discontinuous.hpp>
#include <gridformat/gridformat.hpp>


// Exemplary usage of writing grid views...
void write_grid_view() {
    Dune::YaspGrid<3> grid{{1.0, 1.0, 1.0}, {100, 100, 100}};
    const auto& grid_view = grid.leafGridView();

    // For general Dune::GridView, the traits for the unstructured grid concept are specialized.
    // However, Dune::YaspGrid is actually an "ImageGrid", and the specializations of the respective
    // traits are defined for Dune::GridView<Dune::YaspGrid<...>>. Therefore, if we ask `GridFormat`
    // for a default format to write to, it selects `.vti`, which is more compact than the generic `.vtu`.
    GridFormat::Writer writer{GridFormat::default_for(grid_view), grid_view};
    writer.set_point_field("pfield", [&] (const auto& vertex) {
        // we get a dune vertex here, and therefore have access to the geometry
        const auto& geometry = vertex.geometry();
        const auto& center = geometry.center();
        return center[0]*center[1];
    });
    writer.set_point_field("cfield", [&] (const auto& element) {
        // we get a dune element here, and therefore have access to the geometry
        const auto& geometry = element.geometry();
        const auto& center = geometry.center();
        return center[0]*center[1];
    });

    // Typically, you would use Dune for numerical simulations in which you store a discrete solution
    // in some vector. We can simply access those values in the lambda we set as point/cell fields.
    // In this example, we just fill those vectors with indices...
    std::vector<int> point_values(grid_view.size(3));
    std::vector<int> cell_values(grid_view.size(0));
    std::ranges::copy(std::views::iota(std::size_t{0}, point_values.size()), point_values.begin());
    std::ranges::copy(std::views::iota(std::size_t{0}, cell_values.size()), cell_values.begin());

    writer.set_cell_field("cell_values_from_vector", [&] (const auto& element) {
        return cell_values[grid_view.indexSet().index(element)];
    });
    writer.set_point_field("point_values_from_vector", [&] (const auto& vertex) {
        return point_values[grid_view.indexSet().index(vertex)];
    });

    const auto filename = writer.write("dune_yasp");
    std::cout << "Wrote '" << filename << "'" << std::endl;
}


// Example to show how discontinuous output can be achieved
// GridFormat provides a wrapper around unstructured grids in order to produce discontinuous
// output. That is, output in which there are different values for points depending on the cell in
// which they are embedded in. What this wrapper effectively does is to join the cell and cell point
// iterators, thereby visiting each point from all cells connected to it.
void write_discontinuous_grid_view() {
    Dune::YaspGrid<2> grid{{1.0, 1.0}, {10, 10}};
    const auto& grid_view = grid.leafGridView();

    // Let's say we store a discontinuous solution in a vector of vectors. In this example, we
    // simply store the element index on each point of a cell s.t. we see a discontinuous field
    // when opening the resulting file...
    std::vector<std::vector<double>> discontinuous_solution(grid_view.size(0));
    for (const auto& element : elements(grid_view)) {
        const auto num_corners = element.subEntities(2);
        const auto element_index = grid_view.indexSet().index(element);
        discontinuous_solution[element_index].resize(num_corners);
        std::ranges::fill(discontinuous_solution[element_index], element_index);
    }

    // We can now simply wrap our grid view as a discontinuous grid and write it
    const auto discontinuous = GridFormat::make_discontinuous(grid_view);
    GridFormat::Writer discontinuous_writer{GridFormat::vtu, discontinuous};
    discontinuous_writer.set_point_field("cell_index_at_points", [&] (const auto& p) {
        // what we receive here is a point that provides access to the cell it is embedded in
        // as well as the cell-local index of the point
        [[maybe_unused]] const auto& host_point = p.host_point();
        const auto& element = p.host_cell();
        const auto local_index = p.index_in_host();
        const auto element_index = grid_view.indexSet().index(element);
        return discontinuous_solution[element_index][local_index];
    });

    // The written file should show piecewise-constant indices per cell - but as point data
    const auto discontinuous_filename = discontinuous_writer.write("dune_yasp_discontinuous");
    std::cout << "Wrote '" << discontinuous_filename << "'" << std::endl;
}


void write_higher_order_dune_function() {
    Dune::YaspGrid<3> grid{{1.0, 1.0, 1.0}, {20, 20, 20}};
    const auto& grid_view = grid.leafGridView();

    // To use higher-order output, let us wrap the grid view in the provided LagrangePolynomialGrid,
    // which exposes a mesh consisting of lagrange cells with the specified order
    unsigned int order = 2;
    GridFormat::Dune::LagrangePolynomialGrid lagrange_grid{grid_view, 2};
    GridFormat::Writer writer{GridFormat::default_for(lagrange_grid), lagrange_grid};

    // we can now use convenience functions to add Dune::Functions, evaluated at the
    // points/cells of the lagrange grid, to the writer...
    // First, let's create a Dune::Function:
    auto function = Dune::Functions::makeAnalyticGridViewFunction([] (const auto& position) {
        return position[0]*position[1];
    }, grid_view);

    // then, let's add this both as point and cell field
    GridFormat::Dune::set_point_function(function, writer, "point_function");
    GridFormat::Dune::set_cell_function(function, writer, "cell_function");

    // ... and write the file
    const auto higher_order_filename = writer.write("dune_quadratic_function");
    std::cout << "Wrote '" << higher_order_filename << "'" << std::endl;

    // The wrapped mesh stores points and connectivity information, and thus, uses
    // additional memory. For time-dependent simulations, you may want to free that memory
    // during time steps, and update the mesh again before the next write. Note that updating
    // is also necessary in case the mesh changes adaptively. Both updating and clearing is
    // exposed in the API of the GridFormat::Dune::LagrangePolynomialGrid:
    lagrange_grid.clear();
    lagrange_grid.update(grid_view);
}


int main() {
    write_grid_view();
    write_discontinuous_grid_view();
    write_higher_order_dune_function();
    return 0;
}
