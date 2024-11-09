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

struct TestGrid {
    std::vector<int> points{0, 1, 2};
    std::vector<int> cells{0, 1};
};

namespace GridFormat::Traits {

template<> struct Points<TestGrid> {
    static auto get(const TestGrid& grid) {
        return grid.points | std::views::all;
    }
};

template<> struct Cells<TestGrid> {
    static auto get(const TestGrid& grid) {
        return grid.cells | std::views::all;
    }
};

template<> struct PointCoordinates<TestGrid, int> {
    static std::array<double, 1> get(const TestGrid&, const int point) {
        return {static_cast<double>(point)};
    }
};

template<> struct CellPoints<TestGrid, int> {
    static auto get(const TestGrid& grid, const int cell) {
        const int begin_idx = cell == 0 ? 0 : 1;
        return std::views::iota(begin_idx, begin_idx + 2) | std::views::transform([&] (auto i) {
            return grid.points[i];
        });
    }
};

template<> struct PointId<TestGrid, int> {
    static int get(const TestGrid&, const int point) {
        return point;
    }
};

template<> struct CellType<TestGrid, int> {
    static auto get(const TestGrid&, const int) {
        return GridFormat::CellType::segment;
    }
};

}  // namespace GridFormat::Traits

template<typename Grid>
void test_with(const Grid& host_grid) {
    GridFormat::DiscontinuousGrid grid{host_grid};
    static_assert(GridFormat::Concepts::UnstructuredGrid<decltype(grid)>);

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "discontinuous_grid_cell_range"_test = [&] () {
        expect(eq(
            static_cast<std::size_t>(GridFormat::Ranges::size(GridFormat::cells(grid))),
            GridFormat::number_of_cells(host_grid)
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
}


int main() {

    test_with(TestGrid{});
    test_with(GridFormat::ImageGrid<2, double>{{1.0, 1.0}, {15, 10}});

    return 0;
}
