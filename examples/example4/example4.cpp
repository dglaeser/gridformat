// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <ranges>

#include <dune/grid/yaspgrid.hh>

#include <gridformat/grid/adapters/dune.hpp>
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

    return 0;
}
