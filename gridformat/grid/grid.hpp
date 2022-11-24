// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \brief Defines the grid interface
 */
#ifndef GRIDFORMAT_GRID_GRID_HPP_
#define GRIDFORMAT_GRID_GRID_HPP_

#include <ranges>
#include <concepts>
#include <cassert>
#include <vector>

#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/concepts.hpp>

namespace GridFormat {

//! \addtogroup Grid
//! \{

template<typename Grid> requires(GridDetail::exposes_point_range<Grid>)
std::ranges::range auto points(const Grid& grid) {
    return Traits::Points<Grid>::get(grid);
}

template<typename Grid> requires(GridDetail::exposes_cell_range<Grid>)
std::ranges::range auto cells(const Grid& grid) {
    return Traits::Cells<Grid>::get(grid);
}

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

template<typename Grid> requires(GridDetail::exposes_cell_points<Grid>)
std::ranges::range auto points(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellPoints<Grid, Cell<Grid>>::get(grid, cell);
}

template<typename Grid>
std::size_t number_of_cells(const Grid& grid) {
    return Ranges::size(cells(grid));
}

template<typename Grid>
std::size_t number_of_points(const Grid& grid) {
    return Ranges::size(points(grid));
}

template<typename Grid>
std::vector<std::size_t> make_point_id_map(const Grid& grid) {
    std::size_t i = 0;
    std::vector<std::size_t> point_id_to_running_idx(number_of_points(grid));
    for (const auto& p : points(grid)) {
        assert(id(grid, p) < point_id_to_running_idx.size());
        point_id_to_running_idx[id(grid, p)] = i++;
    }
    return point_id_to_running_idx;
}

//! \} group Grid

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_GRID_HPP_
