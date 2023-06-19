// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \brief Predefined image grid implementation.
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
#include <gridformat/common/md_index.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat {

/*!
 * \ingroup Grid
 * \brief Predefined grid implementation that represents a structured, equispaced grid.
 * \tparam dim The dimension of the grid (1 <= dim <= 3)
 * \tparam CoordinateType The type used to represent coordinates (e.g. `double`)
 */
template<std::size_t dim, Concepts::Scalar CoordinateType>
class ImageGrid {
    static_assert(dim > 0 && dim <= 3);

    template<int codim>
    struct Entity { std::array<std::size_t, dim> location; };

 public:
    using Cell = Entity<0>;  //!< The type used for grid cells
    using Point = Entity<dim>;  //!< The type used for grid points

    //! Constructor overload for general range types
    template<Concepts::StaticallySizedMDRange<1> Size,
             Concepts::StaticallySizedMDRange<1> Cells>
    ImageGrid(Size&& size, Cells&& cells)
    : ImageGrid(
        Ranges::filled_array<dim>(CoordinateType{0}),
        std::forward<Size>(size),
        std::forward<Cells>(cells)
    )
    {}

    //! Constructor overload for general range types
    template<Concepts::StaticallySizedMDRange<1> Origin,
             Concepts::StaticallySizedMDRange<1> Size,
             Concepts::StaticallySizedMDRange<1> Cells>
        requires(static_size<Origin> == dim and
                 static_size<Size> == dim and
                 static_size<Cells> == dim and
                 std::integral<std::ranges::range_value_t<Cells>>)
    ImageGrid(Origin&& origin, Size&& size, Cells&& cells)
    : ImageGrid(
        Ranges::to_array<dim, CoordinateType>(origin),
        Ranges::to_array<dim, CoordinateType>(size),
        Ranges::to_array<dim, std::size_t>(cells)
    )
    {}

    /*!
     * \brief Construct a grid with the given size and discretization.
     * \param size The size of the grid
     * \param cells The number of cells in all directions
     */
    ImageGrid(std::array<CoordinateType, dim> size,
              std::array<std::size_t, dim> cells)
    : ImageGrid(Ranges::filled_array<dim>(CoordinateType{0}), std::move(size), std::move(cells))
    {}

    /*!
     * \brief Construct a grid with the given size and discretization.
     * \param origin The origin of the grid (e.g. the lower-left corner in 2d)
     * \param size The size of the grid
     * \param cells The number of cells in all directions
     */
    ImageGrid(std::array<CoordinateType, dim> origin,
              std::array<CoordinateType, dim> size,
              std::array<std::size_t, dim> cells)
    : _lower_right{std::move(origin)}
    , _upper_right{Ranges::apply_pairwise<CoordinateType>(std::plus{}, _lower_right, size)}
    , _spacing{_compute_spacing(cells)}
    , _cell_index_tuples{MDLayout{cells}}
    , _point_index_tuples{MDLayout{Ranges::incremented(cells, 1)}}
    , _cell_point_offsets{MDLayout{Ranges::filled_array<dim, std::size_t>(2)}}
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

    //! Return an array containing the number of cells in all directions
    auto extents() const {
        return Ranges::to_array<dim, std::size_t>(
            std::views::iota(std::size_t{0}, dim) | std::views::transform([&] (std::size_t i) {
                return _cell_index_tuples.size(i);
            })
        );
    }

    //! Return the ordinates of the grid along the given direction
    auto ordinates(int direction) const {
        std::vector<CoordinateType> result(number_of_points(direction));
        for (unsigned int i = 0; i < result.size(); ++i)
            result[i] = _ordinate_at(i, direction);
        return result;
    }

    //! Return the physical position of the given grid point
    auto position(const Point& p) const {
        return Ranges::to_array<dim, CoordinateType>(
            std::views::iota(std::size_t{0}, dim)
            | std::views::transform([&] (std::size_t direction) {
                return _ordinate_at(p.location[direction], direction);
            })
        );
    }

    //! Return the physical center position of the given grid cell
    auto center(const Cell& c) const {
        unsigned corner_count = 0;
        auto result = Ranges::filled_array<dim>(CoordinateType{0});
        for (const auto& p : points(c)) {
            corner_count++;
            const auto ppos = position(p);
            for (unsigned i = 0; i < dim; ++i)
                result[i] += ppos[i];
        }
        std::ranges::for_each(result, [&] (auto& v) { v /= static_cast<CoordinateType>(corner_count); });
        return result;
    }

    //! Return a unique id for the given point
    std::size_t id(const Point& p) const {
        return _point_mapper.map(p.location);
    }

    //! Return a range over all cells of the grid
    std::ranges::range auto cells() const {
        return _cell_index_tuples | std::views::transform([] (const auto& indices) {
            return Cell{Ranges::to_array<dim>(indices)};
        });
    }

    //! Return a range over all points of the grid
    std::ranges::range auto points() const {
        return _point_index_tuples | std::views::transform([] (const auto& indices) {
            return Point{Ranges::to_array<dim>(indices)};
        });
    }

    //! Return a range over all points in the given grid cell
    std::ranges::range auto points(const Cell& cell) const {
        return _cell_point_offsets
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
    MDIndexRange _cell_index_tuples;
    MDIndexRange _point_index_tuples;
    MDIndexRange _cell_point_offsets;
    FlatIndexMapper<dim> _point_mapper;
};

template<Concepts::StaticallySizedMDRange<1> S, Concepts::StaticallySizedMDRange<1> C>
    requires(static_size<S> == static_size<C>)
ImageGrid(S&&, C&&) -> ImageGrid<static_size<S>, std::ranges::range_value_t<S>>;


#ifndef DOXYGEN
namespace Traits {

template<std::size_t dim, typename CT>
struct Points<ImageGrid<dim, CT>> {
    static std::ranges::range auto get(const ImageGrid<dim, CT>& grid) {
        return grid.points();
    }
};

template<std::size_t dim, typename CT>
struct Cells<ImageGrid<dim, CT>> {
    static std::ranges::range auto get(const ImageGrid<dim, CT>& grid) {
        return grid.cells();
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
struct PointCoordinates<ImageGrid<dim, CT>, typename ImageGrid<dim, CT>::Point> {
    static decltype(auto) get(const ImageGrid<dim, CT>& grid, const typename ImageGrid<dim, CT>::Point& p) {
        return grid.position(p);
    }
};

// register as unstructured grid as well
template<std::size_t dim, typename CT>
struct CellPoints<ImageGrid<dim, CT>, typename ImageGrid<dim, CT>::Cell> {
    static auto get(const ImageGrid<dim, CT>& grid, const typename ImageGrid<dim, CT>::Cell& c) {
        return grid.points(c);
    }
};

template<std::size_t dim, typename CT>
struct PointId<ImageGrid<dim, CT>, typename ImageGrid<dim, CT>::Point> {
    static auto get(const ImageGrid<dim, CT>& grid, const typename ImageGrid<dim, CT>::Point& p) {
        return grid.id(p);
    }
};

template<std::size_t dim, typename CT>
struct CellType<ImageGrid<dim, CT>, typename ImageGrid<dim, CT>::Cell> {
    static auto get(const ImageGrid<dim, CT>&, const typename ImageGrid<dim, CT>::Cell&) {
        if constexpr (dim == 1)
            return GridFormat::CellType::segment;
        if constexpr (dim == 2)
            return GridFormat::CellType::pixel;
        if constexpr (dim == 3)
            return GridFormat::CellType::voxel;
    }
};

}  // namespace Traits
#endif  // DOXYGEN

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_IMAGE_GRID_HPP_
