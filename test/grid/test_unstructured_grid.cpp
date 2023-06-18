// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <algorithm>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/grid.hpp>
#include "unstructured_grid.hpp"
#include "../testing.hpp"

template<GridFormat::Concepts::UnstructuredGrid Grid>
void check_grid(const Grid& grid) {
    const std::ranges::range auto& cells = GridFormat::cells(grid);
    const std::size_t cell_count = std::ranges::distance(
        std::ranges::begin(cells), std::ranges::end(cells)
    );

    const std::ranges::range auto& points = GridFormat::points(grid);
    const std::size_t point_count = std::ranges::distance(
        std::ranges::begin(points), std::ranges::end(points)
    );

    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    expect(eq(point_count, grid.points().size()));
    expect(eq(cell_count, grid.cells().size()));
    expect(eq(GridFormat::number_of_cells(grid), grid.cells().size()));
    expect(eq(GridFormat::number_of_points(grid), grid.points().size()));
    expect(std::ranges::equal(cells, grid.cells()));
    expect(std::ranges::equal(points, grid.points()));

    std::size_t i = 0;
    for (const auto& p : GridFormat::points(grid)) {
        expect(eq(grid.points()[i].id, GridFormat::id(grid, p)));
        expect(std::ranges::equal(
            grid.points()[i].coordinates,
            GridFormat::coordinates(grid, p)
        ));
        i++;
    }

    i = 0;
    for (const auto& c : GridFormat::cells(grid)) {
        expect(grid.cells()[i].cell_type == GridFormat::type(grid, c));
        expect(std::ranges::equal(
            grid.cells()[i].corners | std::views::transform([&] (const std::integral auto idx) {
                return grid.points()[idx].id;
            }),
            GridFormat::points(grid, c) | std::views::transform([] (const auto& point) {
                return point.id;
            })
        ));
        expect(std::ranges::equal(
            grid.cells()[i].corners | std::views::transform([&] (const std::integral auto idx) {
                return grid.points()[idx].id;
            }),
            GridFormat::points(grid, c) | std::views::transform([&] (const auto& point) {
                return GridFormat::id(grid, point);
            })
        ));
        i++;
    }
}

int main() {
    using GridFormat::Testing::operator""_test;
    "unstructured_grid_0d_in_1d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_0d<1>());  };
    "unstructured_grid_0d_in_2d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_0d<2>());  };
    "unstructured_grid_0d_in_3d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_0d<3>());  };

    "unstructured_grid_1d_in_1d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_1d<1>());  };
    "unstructured_grid_1d_in_2d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_1d<2>());  };
    "unstructured_grid_1d_in_3d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_1d<3>());  };

    "unstructured_grid_2d_in_2d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_2d<2>());  };
    "unstructured_grid_2d_in_3d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_2d<3>());  };

    "unstructured_grid_3d"_test = [] () { check_grid(GridFormat::Test::make_unstructured_2d<3>());  };

    return 0;
}
