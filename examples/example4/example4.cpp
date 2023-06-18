// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>

#include <dune/grid/yaspgrid.hh>

#include <gridformat/traits/dune.hpp>
#include <gridformat/grid/discontinuous.hpp>
#include <gridformat/gridformat.hpp>

int main() {
    // create a grid with 100x100*100 cells on the unit cube
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


    // GridFormat also provides a wrapper around unstructured grids in order to produce discontinuous
    // output. That is, output in which there are different values for points depending on the cell in
    // which they are embedded in. What this wrapper effectively does is to join the cell and cell point
    // iterators, thereby visiting each point from all cells connected to it. Note that if your grid structure
    // is already represented like this, you don't need to wrap it in order to get discontinuous output...

    // first of all, let's make a small 2d grid so the result will be easier to inspect...
    Dune::YaspGrid<2> small_grid{{1.0, 1.0}, {10, 10}};
    const auto& small_grid_view = small_grid.leafGridView();

    // Let's say we store a discontinuous solution in a vector of vectors. In this example, we
    // simply store the element index on each point of a cell s.t. we see a discontinuous field
    // when opening the resulting file...
    std::vector<std::vector<double>> discontinuous_solution(small_grid_view.size(0));
    for (const auto& element : elements(small_grid_view)) {
        const auto num_corners = element.subEntities(2);
        const auto element_index = small_grid_view.indexSet().index(element);
        discontinuous_solution[element_index].resize(num_corners);
        std::ranges::fill(discontinuous_solution[element_index], element_index);
    }

    // We can now simply wrap our grid view as a discontinuous grid and write it
    const auto discontinuous = GridFormat::make_discontinuous(small_grid_view);
    GridFormat::Writer discontinuous_writer{GridFormat::vtu, discontinuous};
    discontinuous_writer.set_point_field("cell_index_at_points", [&] (const auto& p) {
        // what we receive here is a point that provides access to the cell it is embedded in
        // as well as the cell-local index of the point
        [[maybe_unused]] const auto& host_point = p.host_point();
        const auto& element = p.host_cell();
        const auto local_index = p.index_in_host();
        const auto element_index = small_grid_view.indexSet().index(element);
        return discontinuous_solution[element_index][local_index];
    });

    // The written file should show piecewise-constant indices per cell - but as point data
    const auto discontinuous_filename = discontinuous_writer.write("dune_yasp_discontinuous");
    std::cout << "Wrote '" << discontinuous_filename << "'" << std::endl;

    return 0;
}
