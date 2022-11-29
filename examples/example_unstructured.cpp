// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <ranges>
#include <cmath>

#include <gridformat/writer.hpp>

// Exemplary (inefficient) implementation of a raster grid with unit-sized cells
class MyGrid {
    template<int codim>
    struct Entity {
        std::size_t _x_id;
        std::size_t _y_id;
    };

 public:
    using Point = Entity<2>;
    using Cell = Entity<0>;

    MyGrid(std::size_t cells_x, std::size_t cells_y)
    : _cells_x(cells_x)
    , _cells_y(cells_y) {
        _points.reserve(_num_points());
        _cells.reserve(_num_cells());
        std::ranges::for_each(
            std::views::iota(std::size_t{0}, _num_points()),
            [&] (std::size_t id) {
                const auto [xidx, yidx] = _get_point_index_pair(id);
                _points.emplace_back(Point{xidx, yidx});
            }
        );
        std::ranges::for_each(
            std::views::iota(std::size_t{0}, _num_cells()),
            [&] (std::size_t id) {
                const auto [xidx, yidx] = _get_cell_index_pair(id);
                _cells.emplace_back(Cell{xidx, yidx});
            }
        );
    }

    std::array<double, 2> get_position(const Point& p) const {
        return {
            static_cast<double>(p._x_id),
            static_cast<double>(p._y_id)
        };
    }

    std::array<double, 2> get_center(const Cell& c) const {
        return {
            static_cast<double>(c._x_id) + 0.5,
            static_cast<double>(c._y_id) + 0.5
        };
    }

    const std::ranges::range auto& points() const { return _points; }
    const std::ranges::range auto& cells() const { return _cells; }

    std::size_t number_of_cells_x() const { return _cells_x; }
    std::size_t number_of_cells_y() const { return _cells_y; }

    std::size_t number_of_points_x() const { return _cells_x + 1; }
    std::size_t number_of_points_y() const { return _cells_y + 1; }

 private:
    std::size_t _num_points() const {
        return (_cells_x + 1)*(_cells_y + 1);
    }

    std::size_t _num_cells() const {
        return _cells_x*_cells_y;
    }

    std::pair<std::size_t, std::size_t> _get_point_index_pair(std::size_t id) const {
        return {id%(_cells_x + 1), id/(_cells_x + 1)};
    }

    std::pair<std::size_t, std::size_t> _get_cell_index_pair(std::size_t id) const {
        return {id%_cells_x, id/_cells_x };
    }

    std::size_t _cells_x;
    std::size_t _cells_y;
    std::vector<Point> _points;
    std::vector<Cell> _cells;
};

// Although structured, we register MyGrid as UnstructuredGrid
namespace GridFormat::Traits {

template<>
struct Points<MyGrid> {
    static decltype(auto) get(const MyGrid& grid) {
        return grid.points();
    }
};

template<>
struct Cells<MyGrid> {
    static decltype(auto) get(const MyGrid& grid) {
        return grid.cells();
    }
};

template<>
struct CellType<MyGrid, typename MyGrid::Cell> {
    static auto get(const MyGrid&, const typename MyGrid::Cell&) {
        return GridFormat::CellType::quadrilateral;
    }
};

template<>
struct CellPoints<MyGrid, typename MyGrid::Cell> {
    static auto get(const MyGrid&, const typename MyGrid::Cell& c) {
        return std::array<typename MyGrid::Point, 4>{
            typename MyGrid::Point{c._x_id, c._y_id},
            typename MyGrid::Point{c._x_id + 1, c._y_id},
            typename MyGrid::Point{c._x_id + 1, c._y_id + 1},
            typename MyGrid::Point{c._x_id, c._y_id + 1}
        };
    }
};

template<>
struct PointCoordinates<MyGrid, typename MyGrid::Point> {
    static auto get(const MyGrid& grid, const typename MyGrid::Point& p) {
        return grid.get_position(p);
    }
};

template<>
struct PointId<MyGrid, typename MyGrid::Point> {
    static auto get(const MyGrid& grid, const typename MyGrid::Point& p) {
        return p._y_id*grid.number_of_points_x() + p._x_id;
    }
};

}  // namespace GridFormat::Traits

inline double test_function(const std::array<double, 2>& position) {
    return std::sin(position[0])*std::cos(position[1]);
}


int main() {
    MyGrid grid(10, 10);

    // you may simply get a default writer for your grid
    auto writer = GridFormat::make_writer(grid);

    // attach point and cell data via lambdas
    writer.set_point_field("test_func", [&] (const auto& point) {
        return test_function(grid.get_position(point));
    });
    writer.set_cell_field("test_func", [&] (const auto& cell) {
        return test_function(grid.get_center(cell));
    });

    // write the file providing a base filename
    writer.write("unstructured");

    return 0;
}
