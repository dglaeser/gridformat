// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_GRID_UNSTRUCTURED_GRID_HPP_
#define GRIDFORMAT_TEST_GRID_UNSTRUCTURED_GRID_HPP_

#include <cstddef>
#include <vector>
#include <utility>
#include <ranges>
#include <algorithm>

#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/cell_type.hpp>

namespace GridFormat {
namespace Test {

template<int dim>
struct Point {
    std::array<double, dim> coordinates;
    std::size_t id;

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

template<int space_dim>
class UnstructuredGrid {
 public:
    using Point = Test::Point<space_dim>;
    using Cell = Test::Cell;

    UnstructuredGrid(std::vector<Point>&& points,
                     std::vector<Cell>&& cells)
    : _points(std::move(points))
    , _cells(std::move(cells))
    {}

    const auto& points() const { return _points; }
    const auto& cells() const { return _cells; }

 private:
    std::vector<Point> _points;
    std::vector<Cell> _cells;
};


auto make_unstructured_0d() {
    return UnstructuredGrid<1>{
        {
            {{0.0}, 0},
            {{1.0}, 1},
            {{3.0}, 2}
        },
        {
            {{0}, CellType::vertex, 0},
            {{1}, CellType::vertex, 1},
            {{2}, CellType::vertex, 2}
        }
    };
}


auto make_unstructured_1d() {
    return UnstructuredGrid<1>{
        {
            {{0.0}, 0},
            {{1.0}, 1},
            {{3.0}, 2}
        },
        {
            {{0, 1}, CellType::segment, 0},
            {{1, 2}, CellType::segment, 1}
        }
    };
}

auto make_unstructured_2d() {
    return UnstructuredGrid<2>{
        {
            {{0.0, 0.0}, 0},
            {{1.0, 0.0}, 1},
            {{1.0, 1.0}, 2},
            {{0.0, 1.0}, 3},
            {{2.0, 1.0}, 4}
        },
        {
            {{0, 1, 2, 3}, CellType::quadrilateral, 0},
            {{1, 2, 4}, CellType::triangle, 1}
        }
    };
}

}  // namespace Test

namespace Traits {

template<int space_dim>
struct Points<Test::UnstructuredGrid<space_dim>> {
    static decltype(auto) get(const Test::UnstructuredGrid<space_dim>& grid) {
        return grid.points();
    }
};

template<int space_dim>
struct Cells<Test::UnstructuredGrid<space_dim>> {
    static decltype(auto) get(const Test::UnstructuredGrid<space_dim>& grid) {
        return grid.cells();
    }
};

template<int space_dim>
struct PointCoordinates<Test::UnstructuredGrid<space_dim>,
                        typename Test::UnstructuredGrid<space_dim>::Point> {
    static decltype(auto) get([[maybe_unused]] const Test::UnstructuredGrid<space_dim>& grid,
                              const typename Test::UnstructuredGrid<space_dim>::Point& p) {
        return p.coordinates;
    }
};

template<int space_dim>
struct PointId<Test::UnstructuredGrid<space_dim>,
               typename Test::UnstructuredGrid<space_dim>::Point> {
    static decltype(auto) get([[maybe_unused]] const Test::UnstructuredGrid<space_dim>& grid,
                              const typename Test::UnstructuredGrid<space_dim>::Point& p) {
        return p.id;
    }
};

template<int space_dim>
struct CellType<Test::UnstructuredGrid<space_dim>,
                typename Test::UnstructuredGrid<space_dim>::Cell> {
    static auto get([[maybe_unused]] const Test::UnstructuredGrid<space_dim>& grid,
                    const typename Test::UnstructuredGrid<space_dim>::Cell& cell) {
        return cell.cell_type;
    }
};

template<int space_dim>
struct CellPoints<Test::UnstructuredGrid<space_dim>,
                  typename Test::UnstructuredGrid<space_dim>::Cell> {
    static auto get(const Test::UnstructuredGrid<space_dim>& grid,
                    const typename Test::UnstructuredGrid<space_dim>::Cell& cell) {
        return std::views::transform(cell.corners, [&] (std::integral auto idx) {
            return grid.points()[idx];
        });
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_TEST_GRID_UNSTRUCTURED_GRID_HPP_
