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
#include <unordered_map>

#include <gridformat/common/ranges.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/concepts.hpp>

namespace GridFormat {

//! \addtogroup Grid
//! \{

template<GridDetail::ExposesPointRange Grid>
std::ranges::range auto points(const Grid& grid) {
    return Traits::Points<Grid>::get(grid);
}

template<GridDetail::ExposesCellRange Grid>
std::ranges::range auto cells(const Grid& grid) {
    return Traits::Cells<Grid>::get(grid);
}

template<GridDetail::ExposesPointId Grid>
std::integral auto id(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointId<Grid, Point<Grid>>::get(grid, point);
}

template<GridDetail::ExposesPointCoordinates Grid>
std::ranges::range auto coordinates(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointCoordinates<Grid, Point<Grid>>::get(grid, point);
}

template<GridDetail::ExposesCellType Grid>
CellType type(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellType<Grid, Cell<Grid>>::get(grid, cell);
}

template<GridDetail::ExposesCellPoints Grid>
std::ranges::range auto points(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellPoints<Grid, Cell<Grid>>::get(grid, cell);
}

template<typename Grid> requires(
    GridDetail::ExposesNumberOfCells<Grid> or
    GridDetail::ExposesCellRange<Grid>)
std::size_t number_of_cells(const Grid& grid) {
    if constexpr (GridDetail::ExposesNumberOfCells<Grid>)
        return Traits::NumberOfCells<Grid>::get(grid);
    else
        return Ranges::size(cells(grid));
}

template<typename Grid> requires(
    GridDetail::ExposesNumberOfPoints<Grid> or
    GridDetail::ExposesPointRange<Grid>)
std::size_t number_of_points(const Grid& grid) {
    if constexpr (GridDetail::ExposesNumberOfPoints<Grid>)
        return Traits::NumberOfPoints<Grid>::get(grid);
    else
        return Ranges::size(points(grid));
}

template<typename Grid> requires(
    GridDetail::ExposesNumberOfCellCorners<Grid> or
    GridDetail::ExposesPointRange<Grid>)
std::size_t number_of_points(const Grid& grid, const Cell<Grid>& cell) {
    if constexpr (GridDetail::ExposesNumberOfCellCorners<Grid>)
        return Traits::NumberOfCellCorners<Grid>::get(grid, cell);
    else
        return Ranges::size(points(grid, cell));
}

template<GridDetail::ExposesPointRange Grid> requires(GridDetail::ExposesPointId<Grid>)
std::unordered_map<std::size_t, std::size_t> make_point_id_map(const Grid& grid) {
    std::size_t i = 0;
    std::unordered_map<std::size_t, std::size_t> point_id_to_running_idx;
    for (const auto& p : points(grid)) {
        point_id_to_running_idx[id(grid, p)] = i++;
    }
    return point_id_to_running_idx;
}

//! \} group Grid

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_GRID_HPP_
