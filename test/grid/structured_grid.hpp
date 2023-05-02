// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_TEST_GRID_STRUCTURED_GRID_HPP_
#define GRIDFORMAT_TEST_GRID_STRUCTURED_GRID_HPP_

#include <array>
#include <cstddef>
#include <vector>
#include <utility>
#include <ranges>
#include <random>
#include <numeric>
#include <functional>
#include <algorithm>

#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/cell_type.hpp>

namespace GridFormat {
namespace Test {

#ifndef DOXYGEN
namespace Detail {

    template<int dim, typename T>
    constexpr std::array<T, dim> make_filled_array(T&& value) {
        std::array<T, dim> result;
        std::ranges::fill(result, value);
        return result;
    }

}  // namespace Detail
#endif  // DOXYGEN

template<int dim>
class StructuredGrid {
    static_assert(dim == 2 || dim == 3);

    enum EntityTag { cell, point };

 public:
    template<EntityTag tag>
    struct Entity {
        std::array<std::size_t, dim> position;
        std::size_t id = 0;
    };

    using Point = Entity<EntityTag::point>;
    using Cell = Entity<EntityTag::cell>;

    StructuredGrid(std::array<double, dim> size,
                   std::array<std::size_t, dim> cells,
                   std::array<double, dim> origin = Detail::make_filled_array<dim>(0.0))
    : _origin(std::move(origin))
    , _size(std::move(size))
    , _num_cells(std::move(cells)) {
        if (std::ranges::any_of(_size, [] (double s) { return s <= 0.0; }))
            throw ValueError("Size must be > 0 in all directions");

        for (int i = 0; i < dim; ++i)
            _spacing[i] = _size[i]/static_cast<double>(_num_cells[i]);

        // memory-inefficient, but let's precompute all cells & points
        _cells.reserve(number_of_cells());
        _points.reserve(number_of_points());
        if constexpr (dim == 2) {
            for (std::size_t j = 0; j < number_of_cells(1); ++j)
                for (std::size_t i = 0; i < number_of_cells(0); ++i)
                    _cells.emplace_back(Cell{.position = {i, j}});
            for (std::size_t j = 0; j < number_of_points(1); ++j)
                for (std::size_t i = 0; i < number_of_points(0); ++i)
                    _points.emplace_back(Point{.position = {i, j}});
        } else {
            for (std::size_t k = 0; k < number_of_cells(2); ++k)
                for (std::size_t j = 0; j < number_of_cells(1); ++j)
                    for (std::size_t i = 0; i < number_of_cells(0); ++i)
                        _cells.emplace_back(Cell{.position = {i, j, k}});
            for (std::size_t k = 0; k < number_of_points(2); ++k)
                for (std::size_t j = 0; j < number_of_points(1); ++j)
                    for (std::size_t i = 0; i < number_of_points(0); ++i)
                        _points.emplace_back(Point{.position = {i, j, k}});
        }

        // shuffle cells/points such that the range order does not match the grid ordering
        std::mt19937 g(std::random_device{}());
        std::shuffle(_cells.begin(), _cells.end(), g);
        std::shuffle(_points.begin(), _points.end(), g);

        // set the cell/point ids
        std::ranges::for_each(_cells, [&] (auto& c) { c.id = id(c); });
        std::ranges::for_each(_points, [&] (auto& p) { p.id = id(p); });
    }

    std::array<double, dim> center(const Point& p) const {
        std::array<double, dim> local_pos;
        for (int i = 0; i < dim; ++i)
            local_pos[i] = static_cast<double>(p.position[i]);
        return _position_at(local_pos);
    }

    std::array<double, dim> center(const Cell& c) const {
        std::array<double, dim> local_pos;
        for (int i = 0; i < dim; ++i)
            local_pos[i] = static_cast<double>(c.position[i]) + 0.5;
        return _position_at(local_pos);
    }

    std::size_t number_of_cells() const {
        return std::accumulate(
            _num_cells.begin(),
            _num_cells.end(),
            std::size_t{1},
            std::multiplies{}
        );
    }

     std::size_t number_of_points() const {
        std::array<std::size_t, dim> points;
        std::transform(
            _num_cells.begin(),
            _num_cells.end(),
            points.begin(),
            [] (std::size_t num_cells) { return num_cells + 1; }
        );
        return std::accumulate(
            points.begin(),
            points.end(),
            std::size_t{1},
            std::multiplies{}
        );
    }

    const auto& cells() const { return _cells; }
    const auto& points() const { return _points; }

    const auto& origin() const { return _origin; }
    const auto& extents() const { return _num_cells; }
    const auto& spacing() const { return _spacing; }

    std::size_t id(const Cell& cell) const {
        std::size_t result = cell.position[0];
        for (unsigned i = 1; i < dim; ++i) {
            const auto offset = std::accumulate(
                _num_cells.begin() + i, _num_cells.end(), std::size_t{1}, std::multiplies{}
            );
            result += offset*cell.position[i];
        }
        return result;
    }

    std::size_t id(const Point& point) const {
        std::size_t result = point.position[0];
        auto num_points = _num_cells;
        std::ranges::for_each(num_points, [&] (auto& n) { n += 1; });
        for (unsigned i = 1; i < dim; ++i) {
            const auto offset = std::accumulate(
                num_points.begin() + i, num_points.end(), std::size_t{1}, std::multiplies{}
            );
            result += offset*point.position[i];
        }
        return result;
    }

    std::size_t number_of_cells(int i) const { return _num_cells[i]; }
    std::size_t number_of_points(int i) const { return _num_cells[i] + 1; }

    auto ordinates(int dir) const {
        std::vector<double> result;
        result.reserve(number_of_points(dir));
        for (std::size_t i = 0; i < number_of_points(dir); ++i)
            result.push_back(_origin[dir] + _spacing[dir]*i);
        return result;
    }

    void invert() {
        std::ranges::for_each(_spacing, [] (auto& s) { s *= -1.0; });
    }

    const auto& get_basis() const {
        return _basis;
    }

 protected:
    void set_basis(std::array<std::array<double, dim>, dim> basis) {
        _basis = std::move(basis);
    }

 private:
    static auto _standard_basis() {
        std::array<std::array<double, dim>, dim> result;
        std::ranges::for_each(result, [] (auto& sub_range) {
            std::ranges::fill(sub_range, 0.0);
        });
        for (int i = 0; i < dim; ++i)
            result[i][i] = 1.0;
        return result;
    }

    std::array<double, dim> _position_at(const std::array<double, dim>& local_pos) const {
        std::array<double, dim> result = _origin;
        for (int i = 0; i < dim; ++i)
            for (int j = 0; j < dim; ++j)
                result[j] += _basis[i][j]*_spacing[i]*local_pos[i];
        return result;
    }

    std::array<double, dim> _origin;
    std::array<double, dim> _size;
    std::array<std::size_t, dim> _num_cells;
    std::array<double, dim> _spacing;
    std::array<std::array<double, dim>, dim> _basis = _standard_basis();

    std::vector<Cell> _cells;
    std::vector<Point> _points;
};

template<int dim>
class OrientedStructuredGrid : public StructuredGrid<dim> {
    using ParentType = StructuredGrid<dim>;

 public:
    OrientedStructuredGrid(std::array<std::array<double, dim>, dim> basis,
                           std::array<double, dim> size,
                           std::array<std::size_t, dim> cells,
                           std::array<double, dim> origin = Detail::make_filled_array<dim>(0.0))
    : ParentType(size, cells, origin) {
        this->set_basis(std::move(basis));
    };
};

}  // namespace Test

namespace Traits {

template<int dim>
struct Points<Test::StructuredGrid<dim>> {
    static std::ranges::range auto get(const Test::StructuredGrid<dim>& grid) {
        return grid.points() | std::views::all;
    }
};

template<int dim>
struct Cells<Test::StructuredGrid<dim>> {
    static std::ranges::range auto get(const Test::StructuredGrid<dim>& grid) {
        return grid.cells() | std::views::all;
    }
};

template<int dim>
struct Origin<Test::StructuredGrid<dim>> {
    static std::ranges::range auto get(const Test::StructuredGrid<dim>& grid) {
        return grid.origin();
    }
};

template<int dim>
struct Spacing<Test::StructuredGrid<dim>> {
    static const auto& get(const Test::StructuredGrid<dim>& grid) {
        return grid.spacing();
    }
};

template<int dim>
struct Extents<Test::StructuredGrid<dim>> {
    static const auto& get(const Test::StructuredGrid<dim>& grid) {
        return grid.extents();
    }
};

template<int dim>
struct Ordinates<Test::StructuredGrid<dim>> {
    static auto get(const Test::StructuredGrid<dim>& grid, unsigned int dir) {
        return grid.ordinates(dir);
    }
};

template<int dim, typename Entity>
struct Location<Test::StructuredGrid<dim>, Entity> {
    static decltype(auto) get(const Test::StructuredGrid<dim>&, const Entity& e) {
        return e.position;
    }
};

template<int dim, typename Point>
struct PointCoordinates<Test::StructuredGrid<dim>, Point> {
    static decltype(auto) get(const Test::StructuredGrid<dim>& grid, const Point& p) {
        return grid.center(p);
    }
};


template<int dim>
struct Points<Test::OrientedStructuredGrid<dim>>
: public Points<Test::StructuredGrid<dim>>
{};

template<int dim>
struct Cells<Test::OrientedStructuredGrid<dim>>
: public Cells<Test::StructuredGrid<dim>>
{};

template<int dim>
struct Origin<Test::OrientedStructuredGrid<dim>>
: public Origin<Test::StructuredGrid<dim>>
{};

template<int dim>
struct Spacing<Test::OrientedStructuredGrid<dim>>
: public Spacing<Test::StructuredGrid<dim>>
{};

template<int dim>
struct Extents<Test::OrientedStructuredGrid<dim>>
: public Extents<Test::StructuredGrid<dim>>
{};

template<int dim>
struct Ordinates<Test::OrientedStructuredGrid<dim>>
: public Ordinates<Test::StructuredGrid<dim>>
{};

template<int dim, typename Entity>
struct Location<Test::OrientedStructuredGrid<dim>, Entity>
: public Location<Test::StructuredGrid<dim>, Entity>
{};

template<int dim, typename Point>
struct PointCoordinates<Test::OrientedStructuredGrid<dim>, Point>
: public PointCoordinates<Test::StructuredGrid<dim>, Point>
{};

template<int dim>
struct Basis<Test::OrientedStructuredGrid<dim>> {
    static decltype(auto) get(const Test::OrientedStructuredGrid<dim>& grid) {
        return grid.get_basis();
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_TEST_GRID_STRUCTURED_GRID_HPP_
