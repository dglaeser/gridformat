// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GRIDFORMAT_GRID_DETAIL_HPP_
#define GRIDFORMAT_GRID_DETAIL_HPP_
#ifndef DOXYGEN

#include <ranges>
#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat::GridDetail {

template<typename T, typename Grid>
concept EntityRange = requires(const Grid& grid) {
    { T::get(grid) } -> std::ranges::range;
};

template<typename Grid, EntityRange<Grid> Range>
using Entity = std::decay_t<decltype(*std::ranges::begin(Range::get(std::declval<const Grid&>())))>;

template<typename T>
inline constexpr bool exposes_point_range = is_complete<Traits::Points<T>> && EntityRange<Traits::Points<T>, T>;

template<typename T>
inline constexpr bool exposes_cell_range = is_complete<Traits::Cells<T>> && EntityRange<Traits::Cells<T>, T>;

template<typename Grid> requires(exposes_point_range<Grid>)
using Point = Entity<Grid, Traits::Points<Grid>>;

template<typename Grid> requires(exposes_cell_range<Grid>)
using Cell = Entity<Grid, Traits::Cells<Grid>>;

template<typename T>
inline constexpr bool exposes_point_coordinates
    = is_complete<Traits::PointCoordinates<T, Point<T>>> && requires(const T& grid) {
    { Traits::PointCoordinates<T, Point<T>>::get(grid, std::declval<const Point<T>&>()) } -> std::ranges::range;
};

template<typename T>
inline constexpr bool exposes_point_id
    = is_complete<Traits::PointId<T, Point<T>>> && requires(const T& grid) {
    { Traits::PointId<T, Point<T>>::get(grid, std::declval<const Point<T>&>()) } -> std::integral;
};

template<typename T>
inline constexpr bool exposes_cell_type
    = is_complete<Traits::CellType<T, Cell<T>>> && requires(const T& grid) {
    { Traits::CellType<T, Cell<T>>::get(grid, std::declval<const Cell<T>&>()) } -> std::convertible_to<GridFormat::CellType>;
};

template<typename T>
inline constexpr bool exposes_cell_points
    = is_complete<Traits::CellPoints<T, Cell<T>>> && requires(const T& grid) {
    { Traits::CellPoints<T, Cell<T>>::get(grid, std::declval<const Cell<T>&>()) } -> Concepts::RangeOf<Point<T>>;
};

template<typename F, typename Entity>
using EntityFunctionValueType = std::decay_t<std::invoke_result_t<F, const std::decay_t<Entity>&>>;

}  // namespace GridFormat::GridDetail
#endif // DOXYGEN
#endif  // GRIDFORMAT_GRID_DETAIL_HPP_
