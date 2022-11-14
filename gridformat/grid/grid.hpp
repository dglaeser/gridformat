// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRID_GRID_HPP_
#define GRIDFORMAT_GRID_GRID_HPP_

#include <ranges>
#include <concepts>

#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/concepts.hpp>

namespace GridFormat {

template<typename Grid> requires(GridDetail::exposes_point_range<Grid>)
using Point = GridDetail::Point<Grid>;

template<typename Grid> requires(GridDetail::exposes_cell_range<Grid>)
using Cell = GridDetail::Cell<Grid>;

template<typename T>
using CoordinateType = std::ranges::range_value_t<Point<T>>;

template<typename Grid> requires(GridDetail::exposes_point_range<Grid>)
std::ranges::range auto points(const Grid& grid) {
    return Traits::Points<Grid>::get(grid);
}

template<typename Grid> requires(GridDetail::exposes_cell_range<Grid>)
std::ranges::range auto cells(const Grid& grid) {
    return Traits::Cells<Grid>::get(grid);
}

// TODO: on all of these - entity first, grid second?
template<typename Grid> requires(GridDetail::exposes_point_id<Grid>)
std::integral auto id(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointId<Grid, Point<Grid>>::get(grid, point);
}

template<typename Grid> requires(GridDetail::exposes_point_coordinates<Grid>)
std::ranges::range auto coordinates(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointCoordinates<Grid, Point<Grid>>::get(grid, point);
}

template<typename Grid> requires(GridDetail::exposes_cell_type<Grid>)
CellType type(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellType<Grid, Cell<Grid>>::get(grid, cell);
}

template<typename Grid> requires(GridDetail::exposes_cell_corners<Grid>)
std::ranges::range auto corners(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellCornerPoints<Grid, Cell<Grid>>::get(grid, cell);
}

template<typename Grid>
std::size_t number_of_cells(const Grid& grid) {
    return Ranges::size(cells(grid));
}

template<typename Grid>
std::size_t number_of_points(const Grid& grid) {
    return Ranges::size(points(grid));
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_GRID_HPP_