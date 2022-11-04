// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRID_CONCEPTS_HPP_
#define GRIDFORMAT_GRID_CONCEPTS_HPP_

#include <ranges>
#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat::Concepts {

#ifndef DOXYGEN
namespace Detail {

template<typename T, typename Grid>
concept EntityRange = requires(const Grid& grid) {
    { T::get(grid) } -> std::ranges::input_range;
};

template<typename Grid, EntityRange<Grid> Range>
using Entity = std::decay_t<decltype(*std::ranges::begin(Range::get(std::declval<const Grid&>())))>;

template<typename Grid>
using Point = Entity<Grid, Traits::Points<Grid>>;

template<typename Grid>
using Cell = Entity<Grid, Traits::Cells<Grid>>;

template<typename T>
inline constexpr bool exposes_point_range = is_complete<Traits::Points<T>> && EntityRange<Traits::Points<T>, T>;

template<typename T>
inline constexpr bool exposes_cell_range = is_complete<Traits::Cells<T>> && EntityRange<Traits::Cells<T>, T>;

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

}  // namespace Detail
#endif // DOXYGEN


template<typename T>
concept UnstructuredGrid =
    Detail::exposes_point_range<T> and
    Detail::exposes_cell_range<T> and
    Detail::exposes_point_coordinates<T> and
    Detail::exposes_point_id<T>;

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_GRID_CONCEPTS_HPP_