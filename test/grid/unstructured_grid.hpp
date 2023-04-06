// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_GRID_UNSTRUCTURED_GRID_HPP_
#define GRIDFORMAT_TEST_GRID_UNSTRUCTURED_GRID_HPP_

#include <array>
#include <cstddef>
#include <vector>
#include <utility>
#include <ranges>
#include <random>
#include <algorithm>

#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/cell_type.hpp>

namespace GridFormat {
namespace Test {

template<int dim>
struct Point {
    std::array<double, dim> coordinates;
    std::size_t id;

    static Point make_from_value(double v, std::size_t id) {
        std::array<double, dim> coords;
        std::ranges::fill(coords, v);
        return Point{std::move(coords), id};
    }

    auto begin() const { return coordinates.begin(); }
    auto end() const { return coordinates.end(); }

    bool operator==(const Point& other) const {
        return id == other.id && std::ranges::equal(
            coordinates, other.coordinates
        );
    }
};

struct Cell {
    std::vector<std::size_t> corners;
    CellType cell_type;
    std::size_t id;

    bool operator==(const Cell& other) const {
        return cell_type == other.cell_type && std::ranges::equal(
            corners, other.corners
        );
    }
};

template<int max_dim, int space_dim>
class UnstructuredGrid {
 public:
    static constexpr int dim = max_dim;
    static constexpr int space_dimension = space_dim;
    using Point = Test::Point<space_dim>;
    using Cell = Test::Cell;

    UnstructuredGrid(std::vector<Point>&& points,
                     std::vector<Cell>&& cells)
    : _points(std::move(points))
    , _cells(std::move(cells)) {
        shuffle();
    }

    const auto& points() const { return _points; }
    const auto& cells() const { return _cells; }

    void shuffle() {
        std::vector<std::size_t> running_idx;
        running_idx.reserve(_points.size());
        for (auto i : std::views::iota(std::size_t{0}, _points.size()))
            running_idx.push_back(i);

        std::mt19937 g(1234);
        std::shuffle(running_idx.begin(), running_idx.end(), g);
        const auto old_to_new_running_idx = _inverse_idx_map(running_idx);

        std::vector<Point> new_points;
        new_points.reserve(_points.size());
        for (auto i : std::views::iota(std::size_t{0}, _points.size()))
            new_points.push_back(_points[running_idx[i]]);
        _points = std::move(new_points);

        std::ranges::for_each(_cells, [&] (auto& cell) {
            std::ranges::for_each(cell.corners, [&] (auto& idx) {
                idx = old_to_new_running_idx[idx];
            });
        });
    }

 private:
    auto _inverse_idx_map(const std::vector<std::size_t>& in) const {
        std::size_t i = 0;
        std::vector<std::size_t> result(in.size());
        for (const std::size_t idx : in)
            result[idx] = i++;
        return result;
    }
    std::vector<Point> _points;
    std::vector<Cell> _cells;
};


template<int space_dim = 1>
auto make_unstructured_0d() {
    return UnstructuredGrid<0, space_dim>{
        {
            Point<space_dim>::make_from_value(0.0, 0),
            Point<space_dim>::make_from_value(1.0, 1),
            Point<space_dim>::make_from_value(3.0, 2)
        },
        {
            {{0}, CellType::vertex, 0},
            {{1}, CellType::vertex, 1},
            {{2}, CellType::vertex, 2}
        }
    };
}


// in 1d, use larger grids (easy to define/create) such that
// we run in situations where we need to compress more than
// one block when writing into compressed result files
template<int space_dim = 1>
auto make_unstructured_1d(std::size_t num_cells = 500) {
    std::vector<Point<space_dim>> points;
    points.reserve(num_cells + 1);
    for (std::size_t i = 0; i < num_cells + 1; ++i)
        points.emplace_back(Point<space_dim>::make_from_value(static_cast<double>(i), i));

    std::vector<Cell> cells;
    cells.reserve(num_cells);
    for (std::size_t i = 0; i < num_cells; ++i)
        cells.emplace_back(Cell{{i, i+1}, CellType::segment, i});

    return UnstructuredGrid<1, space_dim>{std::move(points), std::move(cells)};
}

template<int space_dim = 2>
auto make_unstructured_2d() {
    static_assert(space_dim == 2 || space_dim == 3);
    const auto _make_point = [] (const std::array<double, 2>& vals, std::size_t id) {
        if constexpr (space_dim == 2)
            return Point<space_dim>{{vals[0], vals[1]}, id};
        else
            return Point<space_dim>{{vals[0], vals[1], 0.0}, id};
    };

    return UnstructuredGrid<2, space_dim>{
        {
            _make_point({0.0, 0.0}, 0),
            _make_point({1.0, 0.0}, 1),
            _make_point({1.0, 1.0}, 2),
            _make_point({0.0, 1.0}, 3),
            _make_point({2.0, 1.0}, 4),
            _make_point({2.0, 0.0}, 5),
            _make_point({2.5, 0.25}, 6),
            _make_point({2.75, 0.5}, 7),
            _make_point({2.5, 0.75}, 8)
        },
        {
            {{0, 1, 2, 3}, CellType::quadrilateral, 0},
            {{1, 2, 4}, CellType::triangle, 1},
            {{4, 5, 6, 7, 8}, CellType::polygon, 2}
        }
    };
}

auto make_unstructured_3d() {
    return UnstructuredGrid<3, 3>{
        {
            {{0.0, 0.0, 0.0}, 0},
            {{1.0, 0.0, 0.0}, 1},
            {{1.0, 1.0, 0.0}, 2},
            {{0.0, 1.0, 0.0}, 3},
            {{0.0, 0.0, 1.0}, 4},
            {{1.0, 0.0, 1.0}, 5},
            {{1.0, 1.0, 1.0}, 6},
            {{0.0, 1.0, 1.0}, 7},
            {{2.0, 1.0, 1.0}, 8},
            {{1.0, 2.0, 1.0}, 9},
            {{2.0, 2.0, 2.0}, 10}
        },
        {
            {{0, 1, 2, 3, 4, 5, 6, 7}, CellType::hexahedron, 0},
            {{6, 8, 9, 10}, CellType::tetrahedron, 1}
        }
    };
}

template<std::size_t dim, std::size_t space_dim>
auto make_unstructured() {
    static_assert(dim >= 0 && dim <= 3);
    static_assert(dim <= space_dim);
    if constexpr (dim == 0) return make_unstructured_0d<space_dim>();
    if constexpr (dim == 1) return make_unstructured_1d<space_dim>();
    if constexpr (dim == 2) return make_unstructured_2d<space_dim>();
    if constexpr (dim == 3) return make_unstructured_3d();
}

}  // namespace Test

namespace Traits {

template<int dim, int space_dim>
struct Points<Test::UnstructuredGrid<dim, space_dim>> {
    static std::ranges::range auto get(const Test::UnstructuredGrid<dim, space_dim>& grid) {
        return grid.points() | std::views::all;
    }
};

template<int dim, int space_dim>
struct Cells<Test::UnstructuredGrid<dim, space_dim>> {
    static std::ranges::range auto get(const Test::UnstructuredGrid<dim, space_dim>& grid) {
        return grid.cells() | std::views::all;
    }
};

template<int dim, int space_dim>
struct PointCoordinates<Test::UnstructuredGrid<dim, space_dim>,
                        typename Test::UnstructuredGrid<dim, space_dim>::Point> {
    static const auto& get([[maybe_unused]] const Test::UnstructuredGrid<dim, space_dim>& grid,
                           const typename Test::UnstructuredGrid<dim, space_dim>::Point& p) {
        return p.coordinates;
    }
};

template<int dim, int space_dim>
struct PointId<Test::UnstructuredGrid<dim, space_dim>,
               typename Test::UnstructuredGrid<dim, space_dim>::Point> {
    static auto get([[maybe_unused]] const Test::UnstructuredGrid<dim, space_dim>& grid,
                    const typename Test::UnstructuredGrid<dim, space_dim>::Point& p) {
        return p.id;
    }
};

template<int dim, int space_dim>
struct CellType<Test::UnstructuredGrid<dim, space_dim>,
                typename Test::UnstructuredGrid<dim, space_dim>::Cell> {
    static auto get([[maybe_unused]] const Test::UnstructuredGrid<dim, space_dim>& grid,
                    const typename Test::UnstructuredGrid<dim, space_dim>::Cell& cell) {
        return cell.cell_type;
    }
};

template<int dim, int space_dim>
struct CellPoints<Test::UnstructuredGrid<dim, space_dim>,
                  typename Test::UnstructuredGrid<dim, space_dim>::Cell> {
    static auto get(const Test::UnstructuredGrid<dim, space_dim>& grid,
                    const typename Test::UnstructuredGrid<dim, space_dim>::Cell& cell) {
        return std::views::transform(cell.corners, [&] (std::integral auto idx) {
            return grid.points()[idx];
        });
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_TEST_GRID_UNSTRUCTURED_GRID_HPP_
