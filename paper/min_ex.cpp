#include <array>

struct MyGrid {
    std::array<std::size_t, 2> cells;
    std::array<double, 2> dx;
};

// bring in the library, this also brings in the traits declarations
#include <gridformat/gridformat.hpp>

namespace GridFormat::Traits {

// Expose a range over grid cells. Here, we simply use the MDIndexRange provided
// by GridFormat, which allows iterating over all index tuples within the given dimensions
template<> struct Cells<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0], grid.cells[1]}};
    };
};

// Expose a range over grid points.
template<> struct Points<MyGrid> {
    static auto get(const MyGrid& grid) {
        // let's simply return a range over index pairs for the given grid dimensions
        return GridFormat::MDIndexRange{{grid.cells[0]+1, grid.cells[1]+1}};
    };
};

// Expose the number of cells of our "image grid" per direction.
template<> struct Extents<MyGrid> {
    static auto get(const MyGrid& grid) {
        return grid.cells;
    }
};

// Expose the size of the cells per direction.
template<> struct Spacing<MyGrid> {
    static auto get(const MyGrid& grid) {
        return std::array{grid.dx[0], grid.dx[1]};
    }
};

// Expose the position of the grid origin.
template<> struct Origin<MyGrid> {
    static auto get(const MyGrid& grid) {
        // our grid always starts at (0, 0)
        return std::array{0.0, 0.0};
    }
};

// For a given point or cell, expose its location (i.e. index tuple) within the
// structured grid arrangement. Our point/cell types are the same, namely
// GridFormat::MDIndex, because we used MDIndexRange in the Points/Cells traits.
template<> struct Location<MyGrid, GridFormat::MDIndex> {
    static auto get(const MyGrid& grid, const GridFormat::MDIndex& i) {
        return std::array{i.get(0), i.get(1)};
    }
};

}  // namespace GridFormat::Traits

int main() {
    std::size_t nx = 15, ny = 20;
    double dx = 0.1, dy = 0.2;
    // Here, there could be a call to our simulation code:
    // std::vector<double> = solve_problem(nx, ny, dx, dy);
    // But for this example, let's just create a vector filled with 1.0 ...
    std::vector<double> values(nx*ny, 1.0);

    // To write out this solution, let's construct an instance of `MyGrid`
    MyGrid grid{.cells = {nx, ny}, .dx = {dx, dy}};

    // ... and construct a writer, lerting GridFormat choose a format.
    const auto file_format = GridFormat::default_for(grid);
    GridFormat::Writer writer{file_format, grid};

    // We can now write out our numerical solution as a field on grid cells:
    using GridFormat::MDIndex;
    writer.set_cell_field("cfield", [&] (const MDIndex& cell_location) {
        const auto flat_cell_index = cell_location.get(1)*nx + cell_location.get(0);
        return values[flat_cell_index];
    });

    // But we can also just set am analytical function evaluated at cells/points
    writer.set_point_field("pfield", [&] (const MDIndex& point_location) {
        const double x = point_location.get(0);
        const double y = point_location.get(1);
        return x*y;
    });

    writer.write("example"); // gridformat adds the extension
    return 0;
}
