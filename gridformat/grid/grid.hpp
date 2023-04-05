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
#include <numeric>
#include <algorithm>
#include <unordered_map>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/concepts.hpp>

namespace GridFormat {

//! \addtogroup Grid
//! \{

template<GridDetail::ExposesPointRange Grid>
std::ranges::range decltype(auto) points(const Grid& grid) {
    return Traits::Points<Grid>::get(grid);
}

template<GridDetail::ExposesCellRange Grid>
std::ranges::range decltype(auto) cells(const Grid& grid) {
    return Traits::Cells<Grid>::get(grid);
}

template<GridDetail::ExposesPointId Grid>
std::integral auto id(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointId<Grid, Point<Grid>>::get(grid, point);
}

template<GridDetail::ExposesPointCoordinates Grid>
std::ranges::range decltype(auto) coordinates(const Grid& grid, const Point<Grid>& point) {
    return Traits::PointCoordinates<Grid, Point<Grid>>::get(grid, point);
}

template<GridDetail::ExposesCellPoints Grid>
std::ranges::range decltype(auto) points(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellPoints<Grid, Cell<Grid>>::get(grid, cell);
}

template<GridDetail::ExposesCellType Grid>
CellType type(const Grid& grid, const Cell<Grid>& cell) {
    return Traits::CellType<Grid, Cell<Grid>>::get(grid, cell);
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

template<GridDetail::ExposesExtents Grid>
Concepts::StaticallySizedRange decltype(auto) extents(const Grid& grid) {
    return Traits::Extents<Grid>::get(grid);
}

template<GridDetail::ExposesOrigin Grid>
Concepts::StaticallySizedRange decltype(auto) origin(const Grid& grid) {
    return Traits::Origin<Grid>::get(grid);
}

template<GridDetail::ExposesOrdinates Grid>
Concepts::MDRange<1> decltype(auto) ordinates(const Grid& grid, unsigned dir) {
    return Traits::Ordinates<Grid>::get(grid, dir);
}

template<GridDetail::ExposesSpacing Grid>
Concepts::StaticallySizedRange decltype(auto) spacing(const Grid& grid) {
    return Traits::Spacing<Grid>::get(grid);
}

template<GridDetail::ExposesCellLocation Grid>
Concepts::StaticallySizedRange decltype(auto) location(const Grid& grid, const Cell<Grid>& c) {
    return Traits::Location<Grid, Cell<Grid>>::get(grid, c);
}

template<GridDetail::ExposesPointLocation Grid>
Concepts::StaticallySizedRange decltype(auto) location(const Grid& grid, const Point<Grid>& p) {
    return Traits::Location<Grid, Point<Grid>>::get(grid, p);
}

template<Concepts::StructuredGrid Grid, typename Entity>
std::size_t flat_index(const Grid& grid, const Entity& e) {
    static constexpr std::size_t extent_diff = std::is_same_v<Entity, Point<Grid>> ? 1 : 0;
    const auto& extent = extents(grid);
    const auto& loc = location(grid, e);
    static_assert(static_size<std::decay_t<decltype(extent)>> == dimension<Grid>);
    static_assert(static_size<std::decay_t<decltype(loc)>> == dimension<Grid>);

    int i = 0;
    std::array<std::size_t, dimension<Grid>> offsets;
    std::ranges::for_each(extent, [&] (const std::size_t ext) {
        offsets[i] = (i == 0 ? 1 : (ext + extent_diff)*offsets[i-1]);
        i++;
    });

    i = 0;
    return std::accumulate(
        std::ranges::begin(loc),
        std::ranges::end(loc),
        std::size_t{0},
        [&] (std::size_t current, std::size_t index) {
            return current + index*offsets[i++];
        }
    );
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
