// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <set>

#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/image_grid.hpp>
#include <gridformat/grid/discontinuous.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid.hpp>

#include "../testing.hpp"
#include "../make_test_data.hpp"

int main() {
    GridFormat::ImageGrid<2, double> host_grid{{1.0, 1.0}, {15, 10}};
    GridFormat::DiscontinuousGrid grid{host_grid};
    static_assert(GridFormat::Concepts::UnstructuredGrid<decltype(grid)>);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "discontinuous_grid_cell_range"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::cells(grid))),
            host_grid.number_of_cells()
        ));
    };

    "discontinuous_grid_point_range"_test = [&] () {
        std::size_t num_expected_points = 0;
        for (const auto& c : GridFormat::cells(host_grid))
            for ([[maybe_unused]] const auto& p : GridFormat::points(host_grid, c))
                num_expected_points++;
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::points(grid))),
            num_expected_points
        ));
    };

    "discontinuous_grid_point_unique_ids"_test = [&] () {
        std::size_t num_expected_points = 0;
        for (const auto& c : GridFormat::cells(host_grid))
            for ([[maybe_unused]] const auto& p : GridFormat::points(host_grid, c))
                num_expected_points++;

        std::set<std::size_t> point_ids;
        std::ranges::for_each(GridFormat::points(grid), [&] (const auto& p) {
            point_ids.insert(GridFormat::id(grid, p));
        });
        expect(eq(point_ids.size(), num_expected_points));
    };

    return 0;
}
