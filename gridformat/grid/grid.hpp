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

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/concepts.hpp>

namespace GridFormat::Grid {

template<typename Grid> requires(Detail::exposes_point_range<Grid>)
using Point = Detail::Point<Grid>;

template<typename Grid> requires(Detail::exposes_cell_range<Grid>)
using Cell = Detail::Cell<Grid>;

template<typename Grid> requires(Detail::exposes_point_range<Grid>)
std::ranges::range auto points(const Grid& grid) {
    return Traits::Points<Grid>::get(grid);
}

template<typename Grid> requires(Detail::exposes_cell_range<Grid>)
std::ranges::range auto cells(const Grid& grid) {
    return Traits::Cells<Grid>::get(grid);
}

// TODO: on all of these - entity first, grid second?
template<typename Grid> requires(Detail::exposes_point_id<Grid>)
std::integral auto id(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointId<Grid, Point<Grid>>::get(grid, point);
}

template<typename Grid> requires(Detail::exposes_point_coordinates<Grid>)
std::ranges::range auto coordinates(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointCoordinates<Grid, Point<Grid>>::get(grid, point);
}

template<typename Grid> requires(Detail::exposes_cell_type<Grid>)
CellType type(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellType<Grid, Cell<Grid>>::get(grid, cell);
}

template<typename Grid> requires(Detail::exposes_cell_corners<Grid>)
std::ranges::range auto corners(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellCornerPoints<Grid, Cell<Grid>>::get(grid, cell);
}

template<typename Grid>
std::size_t num_cells(const Grid& grid) {
    return std::ranges::distance(cells(grid));
}

template<typename Grid>
std::size_t num_points(const Grid& grid) {
    return std::ranges::distance(points(grid));
}

}  // namespace GridFormat::Grid

#endif  // GRIDFORMAT_GRID_TRAITS_HPP_