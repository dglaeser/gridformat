// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <vector>
#include <ranges>
#include <cmath>

#include <gridformat/gridformat.hpp>

// Exemplary implementation of a raster grid with unit-sized cells
class MyGrid {
    template<int codim>
    struct Entity { std::size_t _id; };

 public:
    using Point = Entity<2>;
    using Cell = Entity<0>;

    MyGrid(std::size_t cells_x, std::size_t cells_y)
    : _cells_x(cells_x)
    , _cells_y(cells_y) {
    }

    std::array<double, 2> get_point_coordinates(const Point& p) const {
        const auto [x, y] = _get_point_index_pair(p._id);
        return {
            static_cast<double>(x),
            static_cast<double>(y)
        };
    }

    std::array<double, 2> get_cell_center(const Cell& c) const {
        const auto [x0, y0] = _get_cell_index_pair(c._id);
        return {
            static_cast<double>(x0) + 0.5,
            static_cast<double>(y0) + 0.5
        };
    }

    std::size_t number_of_cells() const { return _cells_x*_cells_y; }
    std::size_t number_of_points() const { return (_cells_x + 1)*(_cells_y + 1); }

    std::ranges::range auto points() const {
        return std::views::iota(std::size_t{0}, number_of_points())
            | std::views::transform([&] (std::size_t i) { return Point{i}; });
    }

    std::ranges::range auto cells() const {
        return std::views::iota(std::size_t{0}, number_of_cells())
            | std::views::transform([&] (std::size_t i) { return Cell{i}; });
    }

    std::ranges::range auto cell_corners(const Cell& c) const {
        const auto [x0, y0] = _get_cell_index_pair(c._id);
        return std::views::iota(0, 4)
            | std::views::transform([nx=_cells_x+1, x0=x0, y0=y0] (int i) {
                switch (i) {
                    case 0: return Point{y0*nx + x0};
                    case 1: return Point{y0*nx + x0 + 1};
                    case 2: return Point{(y0+1)*nx + x0 + 1};
                    case 3: return Point{(y0+1)*nx + x0};
                    default: throw GridFormat::ValueError("Unexpected corner index");
                }
            });
    }

 private:
    std::pair<std::size_t, std::size_t> _get_point_index_pair(std::size_t id) const {
        return {id%(_cells_x + 1), id/(_cells_x + 1)};
    }

    std::pair<std::size_t, std::size_t> _get_cell_index_pair(std::size_t id) const {
        return {id%_cells_x, id/_cells_x };
    }

    std::size_t _cells_x;
    std::size_t _cells_y;
};

// Although structured, we register MyGrid as UnstructuredGrid
namespace GridFormat::Traits {

template<>
struct Points<MyGrid> {
    static std::ranges::view auto get(const MyGrid& grid) {
        return grid.points();
    }
};

template<>
struct Cells<MyGrid> {
    static std::ranges::view auto get(const MyGrid& grid) {
        return grid.cells();
    }
};

template<>
struct CellType<MyGrid, typename MyGrid::Cell> {
    static auto get(const MyGrid&, const typename MyGrid::Cell&) {
        return GridFormat::CellType::pixel;
    }
};

template<>
struct CellPoints<MyGrid, typename MyGrid::Cell> {
    static auto get(const MyGrid& grid, const typename MyGrid::Cell& c) {
        return grid.cell_corners(c);
    }
};

template<>
struct PointCoordinates<MyGrid, typename MyGrid::Point> {
    static auto get(const MyGrid& grid, const typename MyGrid::Point& p) {
        return grid.get_point_coordinates(p);
    }
};

template<>
struct PointId<MyGrid, typename MyGrid::Point> {
    static auto get(const MyGrid&, const typename MyGrid::Point& p) {
        return p._id;
    }
};

}  // namespace GridFormat::Traits

inline double test_function(const std::array<double, 2>& position) {
    return std::sin(position[0])*std::cos(position[1]);
}


int main() {
    MyGrid grid(10, 10);

    // you may simply get a default writer for your grid
    auto writer = GridFormat::Writer{GridFormat::vtu, grid};

    // attach point and cell data via lambdas
    writer.set_point_field("test_func", [&] (const auto& point) {
        return test_function(grid.get_point_coordinates(point));
    });
    writer.set_cell_field("test_func", [&] (const auto& cell) {
        return test_function(grid.get_cell_center(cell));
    });

    // write the file providing a base filename
    writer.write("unstructured");

    return 0;
}
