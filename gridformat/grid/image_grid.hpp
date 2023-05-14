// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \copydoc GridFormat::ImageGrid
 */
#ifndef GRIDFORMAT_GRID_IMAGE_GRID_HPP_
#define GRIDFORMAT_GRID_IMAGE_GRID_HPP_

#include <cmath>
#include <array>
#include <ranges>
#include <cassert>
#include <utility>
#include <numeric>
#include <algorithm>
#include <functional>
#include <type_traits>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/iterator_facades.hpp>
#include <gridformat/common/flat_index_mapper.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

    template<std::size_t dim>
    class MDIndexRange {
        using IndexTuple = std::array<std::size_t, dim>;

        class Iterator : public ForwardIteratorFacade<Iterator, IndexTuple, const IndexTuple&> {
        public:
            Iterator() = default;
            Iterator(const IndexTuple& bounds)
            : _bounds{&bounds}
            , _current{} {
                std::ranges::fill(_current, std::size_t{0});
            }

            Iterator(const IndexTuple& bounds, bool)
            : Iterator(bounds) {
                _current[0] = (*_bounds)[0];
            }

        private:
            friend IteratorAccess;

            const IndexTuple& _dereference() const {
                return _current;
            }

            bool _is_equal(const Iterator& other) const {
                if (_bounds != other._bounds)
                    return false;
                return std::ranges::equal(_current, other._current);
            }

            void _increment() {
                assert(_is_valid());
                for (int dir = dim - 1; dir > 0; dir--) {
                    if (_current[dir]++; _current[dir] == (*_bounds)[dir])
                        _current[dir] = 0;
                    else
                        return;
                }
                _current[0]++;
            }

            bool _is_valid() const {
                for (unsigned i = 0; i < dim; ++i)
                    if (_current[i] >= (*_bounds)[i])
                        return false;
                return true;
            }

            const IndexTuple* _bounds{nullptr};
            IndexTuple _current;
        };

    public:
        template<Concepts::StaticallySizedMDRange<1> Bounds>
        explicit MDIndexRange(Bounds&& bounds)
        : _bounds(Ranges::to_array<dim, std::size_t>(std::forward<Bounds>(bounds)))
        {}

        auto begin() const { return Iterator{_bounds}; }
        auto end() const { return Iterator{_bounds, true}; }

        std::size_t size(int dir) const { return _bounds[dir]; }
        std::size_t size() const {
            return std::accumulate(
                _bounds.begin(), _bounds.end(), std::size_t{1}, std::multiplies{}
            );
        }

    private:
        std::array<std::size_t, dim> _bounds;
    };

    template<Concepts::StaticallySizedMDRange<1> Bounds>
    MDIndexRange(Bounds&&) -> MDIndexRange<static_size<Bounds>>;

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Grid
 * \brief TODO Doc me
 */
template<std::size_t dim,
         Concepts::Scalar CoordinateType>
class ImageGrid {
    static_assert(dim > 0 && dim <= 3);

    template<int codim>
    struct Entity { std::array<std::size_t, dim> location; };

 public:
    using Cell = Entity<0>;
    using Point = Entity<dim>;

    template<Concepts::StaticallySizedMDRange<1> Size,
             Concepts::StaticallySizedMDRange<1> Cells>
    ImageGrid(Size&& size, Cells&& cells)
    : ImageGrid(
        Ranges::filled_array<dim>(CoordinateType{0}),
        std::forward<Size>(size),
        std::forward<Cells>(cells)
    )
    {}

    template<Concepts::StaticallySizedMDRange<1> Origin,
             Concepts::StaticallySizedMDRange<1> Size,
             Concepts::StaticallySizedMDRange<1> Cells>
        requires(static_size<Origin> == dim
                 and static_size<Size> == dim
                 and static_size<Cells> == dim
                 and std::integral<std::ranges::range_value_t<Cells>>)
    ImageGrid(Origin&& origin, Size&& size, Cells&& cells)
    : _lower_right{Ranges::to_array<dim, CoordinateType>(origin)}
    , _upper_right{Ranges::apply_pairwise<CoordinateType>(std::plus{}, _lower_right, size)}
    , _spacing{_compute_spacing(cells)}
    , _cell_index_tuples{cells}
    , _point_index_tuples{Ranges::incremented(cells, 1)}
    , _point_mapper{Ranges::incremented(cells, 1)} {
        if (std::ranges::any_of(cells, [] (const std::integral auto i) { return i == 0; }))
            throw ValueError("Number of cells in each direction must be > 0");
    }

    std::size_t number_of_cells() const { return _cell_index_tuples.size(); }
    std::size_t number_of_points() const { return _point_index_tuples.size(); }
    std::size_t number_of_cells(unsigned int direction) const { return _cell_index_tuples.size(direction); }
    std::size_t number_of_points(unsigned int direction) const { return _point_index_tuples.size(direction); }

    const auto& origin() const { return _lower_right; }
    const auto& spacing() const { return _spacing; }
    auto extents() const {
        return Ranges::to_array<dim, std::size_t>(
            std::views::iota(std::size_t{0}, dim) | std::views::transform([&] (std::size_t i) {
                return _cell_index_tuples.size(i);
            })
        );
    }

    auto ordinates(int direction) const {
        std::vector<CoordinateType> result(number_of_points(direction));
        for (unsigned int i = 0; i < result.size(); ++i)
            result[i] = _ordinate_at(i, direction);
        return result;
    }

    auto position(const Point& p) const {
        return Ranges::to_array<dim, CoordinateType>(
            std::views::iota(std::size_t{0}, dim)
            | std::views::transform([&] (std::size_t direction) {
                return _ordinate_at(p.location[direction], direction);
            })
        );
    }

    auto center(const Cell& c) const {
        unsigned corner_count = 0;
        auto result = Ranges::filled_array<dim>(CoordinateType{0});
        for (const auto& p : points(*this, c)) {
            corner_count++;
            const auto ppos = position(p);
            for (unsigned i = 0; i < dim; ++i)
                result[i] += ppos[i];
        }
        std::ranges::for_each(result, [&] (auto& v) { v /= static_cast<CoordinateType>(corner_count); });
        return result;
    }

    std::size_t id(const Point& p) const {
        return _point_mapper.map(p.location);
    }

    friend std::ranges::range auto cells(const ImageGrid& grid) {
        auto indices = grid._cell_index_tuples;
        return std::move(indices) | std::views::transform([] (const auto& indices) { return Cell{indices}; });
    }

    friend std::ranges::range auto points(const ImageGrid& grid) {
        auto indices = grid._point_index_tuples;
        return std::move(indices) | std::views::transform([] (const auto& indices) { return Point{indices}; });
    }

    friend std::ranges::range auto points(const ImageGrid&, const Cell& cell) {
        Detail::MDIndexRange<dim> offsets{Ranges::filled_array<dim, std::size_t>(2)};
        return std::move(offsets)
            | std::views::transform([loc = cell.location] (const auto& offset) {
                auto point_loc = loc;
                std::transform(
                    point_loc.begin(), point_loc.end(), offset.begin(), point_loc.begin(),
                    [] (std::size_t p, std::size_t o) { return p + o; }
                );
                return Point{std::move(point_loc)};
        });
    }

 private:
    CoordinateType _ordinate_at(std::size_t i, int direction) const {
        return _lower_right[direction] + _spacing[direction]*static_cast<CoordinateType>(i);
    }

    template<Concepts::StaticallySizedMDRange<1> Cells>
    auto _compute_spacing(Cells&& cells) const {
        return Ranges::apply_pairwise(
            std::divides{},
            Ranges::apply_pairwise(std::minus{}, _upper_right, _lower_right),
            cells
        );
    }

    std::array<unsigned int, dim> _directions_in_descending_size() const {
        std::array<unsigned int, dim> result;
        for (int i = 0; i < dim; ++i)
            result[i] = i;
        std::ranges::sort(result, [&] (unsigned int dir1, unsigned int dir2) {
            return _cell_index_tuples.size(dir1) < _cell_index_tuples.size(dir2);
        });
        return result;
    }

    std::array<CoordinateType, dim> _lower_right;
    std::array<CoordinateType, dim> _upper_right;
    std::array<CoordinateType, dim> _spacing;
    Detail::MDIndexRange<dim> _cell_index_tuples;
    Detail::MDIndexRange<dim> _point_index_tuples;
    FlatIndexMapper<dim> _point_mapper;
};

template<Concepts::StaticallySizedMDRange<1> S, Concepts::StaticallySizedMDRange<1> C>
    requires(static_size<S> == static_size<C>)
ImageGrid(S&&, C&&) -> ImageGrid<static_size<S>, std::ranges::range_value_t<S>>;


namespace Traits {

template<std::size_t dim, typename CT>
struct Points<ImageGrid<dim, CT>> {
    static std::ranges::range auto get(const ImageGrid<dim, CT>& grid) {
        return points(grid);
    }
};

template<std::size_t dim, typename CT>
struct Cells<ImageGrid<dim, CT>> {
    static std::ranges::range auto get(const ImageGrid<dim, CT>& grid) {
        return cells(grid);
    }
};

template<std::size_t dim, typename CT>
struct Origin<ImageGrid<dim, CT>> {
    static const auto& get(const ImageGrid<dim, CT>& grid) {
        return grid.origin();
    }
};

template<std::size_t dim, typename CT>
struct Spacing<ImageGrid<dim, CT>> {
    static const auto& get(const ImageGrid<dim, CT>& grid) {
        return grid.spacing();
    }
};

template<std::size_t dim, typename CT>
struct Extents<ImageGrid<dim, CT>> {
    static auto get(const ImageGrid<dim, CT>& grid) {
        return grid.extents();
    }
};

template<std::size_t dim, typename CT, typename Entity>
struct Location<ImageGrid<dim, CT>, Entity> {
    static_assert(
        std::is_same_v<Entity, typename ImageGrid<dim, CT>::Cell> ||
        std::is_same_v<Entity, typename ImageGrid<dim, CT>::Point>
    );
    static auto get(const ImageGrid<dim, CT>&, const Entity& e) {
        return e.location;
    }
};

template<std::size_t dim, typename CT>
struct Ordinates<ImageGrid<dim, CT>> {
    static auto get(const ImageGrid<dim, CT>& grid, unsigned int dir) {
        return grid.ordinates(dir);
    }
};

// required for structured grid concept
template<std::size_t dim, typename CT>
struct PointCoordinates<ImageGrid<dim, CT>,
                        typename ImageGrid<dim, CT>::Point> {
    static decltype(auto) get(const ImageGrid<dim, CT>& grid,
                              const typename ImageGrid<dim, CT>::Point& p) {
        return grid.position(p);
    }
};

// register as unstructured grid as well
template<std::size_t dim, typename CT>
struct CellPoints<ImageGrid<dim, CT>,
                  typename ImageGrid<dim, CT>::Cell> {
    static auto get(const ImageGrid<dim, CT>& grid,
                    const typename ImageGrid<dim, CT>::Cell& c) {
        return points(grid, c);
    }
};

template<std::size_t dim, typename CT>
struct PointId<ImageGrid<dim, CT>,
               typename ImageGrid<dim, CT>::Point> {
    static auto get(const ImageGrid<dim, CT>& grid,
                    const typename ImageGrid<dim, CT>::Point& p) {
        return grid.id(p);
    }
};

template<std::size_t dim, typename CT>
struct CellType<ImageGrid<dim, CT>,
                typename ImageGrid<dim, CT>::Cell> {
    static auto get(const ImageGrid<dim, CT>&,
                    const typename ImageGrid<dim, CT>::Cell&) {
        if constexpr (dim == 1)
            return GridFormat::CellType::segment;
        if constexpr (dim == 2)
            return GridFormat::CellType::pixel;
        if constexpr (dim == 3)
            return GridFormat::CellType::voxel;
    }
};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_IMAGE_GRID_HPP_
