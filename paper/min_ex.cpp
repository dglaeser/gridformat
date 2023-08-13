#include <array>

struct MyGrid {
    std::array<std::size_t, 2> cells;
    std::array<double, 2> dx;
};

// bring in the library, this also brings in the traits declarations
#include <gridformat/gridformat.hpp>

namespace GridFormat::Traits {

// Expose a range over grid cells. Here, we simply use the MDIndexRange provided
// by GridFormat, which allows to iterate over all index tuples within the given
// dimensions (in our case the number of cells in each coordinate direction)
template<> struct Cells<MyGrid> {
    static auto get(const MyGrid& grid) {
        return GridFormat::MDIndexRange{{grid.cells[0], grid.cells[1]}};
    };
};

// Expose a range over grid points.
template<> struct Points<MyGrid> {
    static auto get(const MyGrid& grid) {
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
        return grid.dx;
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

    // Here, there could be a call to our simulation code, for example:
    // std::vector<double> = solve_problem(nx, ny, dx, dy);
    // But for this simple example, let's just create a vector filled with 1.0 ...
    std::vector<double> values(nx*ny, 1.0);

    // To write out this solution, let's construct an instance of `MyGrid`
    MyGrid grid{.cells = {nx, ny}, .dx = {dx, dy}};

    // ... and construct a writer, letting GridFormat choose a format
    const auto file_format = GridFormat::default_for(grid);
    GridFormat::Writer writer{file_format, grid};

    // We can now write out our numerical solution as a field on grid cells
    using GridFormat::MDIndex;
    writer.set_cell_field("cfield", [&] (const MDIndex& cell_location) {
        const auto flat_index = cell_location.get(1)*nx + cell_location.get(0);
        return values[flat_index];
    });

    // But we can also just set an analytical function evaluated at cells/points
    writer.set_point_field("pfield", [&] (const MDIndex& point_location) {
        const double x = point_location.get(0)*dx;
        const double y = point_location.get(1)*dy;
        return x*y;
    });

    // GridFormat adds the extension to the provided filename
    const auto written_filename = writer.write("example");

    // To read the data back in, we can create a reader class, open our file and
    // access/extract the fields contained in it. Note that we can also get the
    // grid points and cell connectivity. See the documentation for details.
    GridFormat::Reader reader;
    reader.open(written_filename);
    reader.cell_field("cfield")->export_to(values);

    return 0;
}
