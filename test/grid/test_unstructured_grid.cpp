#include <ranges>
#include <algorithm>

#include <boost/ut.hpp>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/grid.hpp>

#include "unstructured_grid.hpp"

template<GridFormat::Concepts::UnstructuredGrid Grid>
void check_grid(const Grid& grid) {
    const std::ranges::range auto& cells = GridFormat::Grid::cells(grid);
    const std::size_t cell_count = std::ranges::distance(
        std::ranges::begin(cells), std::ranges::end(cells)
    );

    const std::ranges::range auto& points = GridFormat::Grid::points(grid);
    const std::size_t point_count = std::ranges::distance(
        std::ranges::begin(points), std::ranges::end(points)
    );

    using boost::ut::expect;
    using boost::ut::eq;
    expect(eq(point_count, grid.points().size()));
    expect(eq(cell_count, grid.cells().size()));
    expect(eq(GridFormat::Grid::num_cells(grid), grid.cells().size()));
    expect(eq(GridFormat::Grid::num_points(grid), grid.points().size()));
    expect(std::ranges::equal(cells, grid.cells()));
    expect(std::ranges::equal(points, grid.points()));

    std::size_t i = 0;
    for (const auto& p : GridFormat::Grid::points(grid)) {
        expect(eq(grid.points()[i].id, GridFormat::Grid::id(grid, p)));
        expect(std::ranges::equal(
            grid.points()[i].coordinates,
            GridFormat::Grid::coordinates(grid, p)
        ));
        i++;
    }

    i = 0;
    for (const auto& c : GridFormat::Grid::cells(grid)) {
        expect(grid.cells()[i].cell_type == GridFormat::Grid::type(grid, c));
        expect(std::ranges::equal(
            grid.cells()[i].corners,
            GridFormat::Grid::corners(grid, c) | std::views::transform([] (const auto& point) {
                return point.id;
            })
        ));
        i++;
    }
}

int main() {
    using namespace boost::ut;
    "unstructured_grid_1d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_1d());  };
    "unstructured_grid_2d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_2d());  };
    return 0;
}