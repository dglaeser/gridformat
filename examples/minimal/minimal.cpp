// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <array>
#include <vector>
#include <iostream>
#include <ranges>

#include <gridformat/gridformat.hpp>

// Data structure to represent an image grid
struct MyGrid {
    std::array<std::size_t, 2> cells;
    std::array<double, 2> dx;
};

// traits specializations for MyGrid
namespace GridFormat::Traits {

// Expose a range over grid cells. Here, we simply use the MDIndexRange provided
// by GridFormat, which allows iterating over all index tuples within the given
// dimensions (in our case the number of cells in each coordinate direction)
// MDIndexRange yields objects of type MDIndex, and thus, GridFormat deduces
// that MDIndex is the cell type of `MyGrid`.
template<> struct Cells<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0], grid.cells[1]}};
    };
};

// Expose a range over grid points
template<> struct Points<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0]+1, grid.cells[1]+1}};
    };
};

// Expose the number of cells of the image grid per coordinate direction
template<> struct Extents<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.cells;
    }
};

// Expose the size of the cells per coordinate direction
template<> struct Spacing<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.dx;
    }
};

// Expose the position of the grid origin
template<> struct Origin<MyGrid> {
    static auto get(const MyGrid& grid) {
        // The origin for our grid's coordinate system is (0, 0)
        return std::array{0.0, 0.0};
    }
};

// For a given point or cell, expose its location (i.e. index tuple) within
// the structured grid arrangement. Our point/cell types are the same, i.e.
// GridFormat::MDIndex, since we used MDIndexRange in the Points/Cells traits.
template<> struct Location<MyGrid, GridFormat::MDIndex> {
    static auto get(const MyGrid& grid, const GridFormat::MDIndex& i) {
        return std::array{i.get(0), i.get(1)};
    }
};

}  // namespace GridFormat::Traits

int main() {
    // Let us check against the concept to see if we implemented the traits correctly.
    // When implementing the grid traits for a data structure, it is helpful to make
    // such static_asserts pass before actually using the GridFormat API.
    static_assert(GridFormat::Concepts::ImageGrid<MyGrid>);

    std::size_t nx = 15, ny = 20;
    double dx = 0.1, dy = 0.2;

    // Here, there could be a call to our simulation code, but for this
    // simple example, let's just create a "solution vector" of indices.
    std::vector<std::size_t> values(nx*ny);
    std::ranges::copy(std::views::iota(std::size_t{0}, nx*ny), values.begin());

    // To write out this solution, let's construct an instance of `MyGrid`
    // and construct a writer with it, letting GridFormat choose a format
    MyGrid grid{.cells = {nx, ny}, .dx = {dx, dy}};
    const auto file_format = GridFormat::default_for(grid);
    GridFormat::Writer writer{file_format, grid};

    // We can now write out our numerical solution as a field on grid cells
    writer.set_cell_field("solution", [&] (const GridFormat::MDIndex& cell_location) {
        // let's assume the solution vector uses flattened row-major ordering
        const auto x_index = cell_location.get(0);
        const auto y_index = cell_location.get(1);
        const auto flat_index = y_index*nx + x_index;
        return values[flat_index];
    });
    const auto written_filename = writer.write("minimal");
    std::cout << "Wrote '" << written_filename << "'" << std::endl;

    return 0;
}
