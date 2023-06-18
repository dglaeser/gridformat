// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <gridformat/common/flat_index_mapper.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/concepts.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace GridDetail {

    template<typename T, std::size_t dim>
    std::array<std::array<T, dim>, dim> standard_basis() {
        std::array<std::array<T, dim>, dim> result;
        std::ranges::for_each(result, [&] (auto& b) { std::ranges::fill(b, T{0.0}); });
        for (std::size_t i = 0; i < dim; ++i)
            result[i][i] = 1.0;
        return result;
    }

}  // namespace GridDetail
#endif  // DOXYGEN

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

template<typename Grid>
    requires(GridDetail::ExposesNumberOfCells<Grid> or
             GridDetail::ExposesCellRange<Grid>)
std::size_t number_of_cells(const Grid& grid) {
    if constexpr (GridDetail::ExposesNumberOfCells<Grid>)
        return Traits::NumberOfCells<Grid>::get(grid);
    else
        return Ranges::size(cells(grid));
}

template<typename Grid>
    requires(GridDetail::ExposesNumberOfPoints<Grid> or
             GridDetail::ExposesPointRange<Grid>)
std::size_t number_of_points(const Grid& grid) {
    if constexpr (GridDetail::ExposesNumberOfPoints<Grid>)
        return Traits::NumberOfPoints<Grid>::get(grid);
    else
        return Ranges::size(points(grid));
}

template<typename Grid>
    requires(GridDetail::ExposesNumberOfCellPoints<Grid> or
             GridDetail::ExposesCellPoints<Grid>)
std::size_t number_of_points(const Grid& grid, const Cell<Grid>& cell) {
    if constexpr (GridDetail::ExposesNumberOfCellPoints<Grid>)
        return Traits::NumberOfCellPoints<Grid, Cell<Grid>>::get(grid, cell);
    else
        return Ranges::size(points(grid, cell));
}

template<GridDetail::ExposesExtents Grid>
Concepts::StaticallySizedRange decltype(auto) extents(const Grid& grid) {
    return Traits::Extents<Grid>::get(grid);
}

template<GridDetail::ExposesExtents Grid>
auto point_extents(const Grid& grid) {
    std::ranges::range auto result = extents(grid);
    std::ranges::for_each(result, [] (std::integral auto& ext) { ext++; });
    return result;
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

template<Concepts::StructuredEntitySet Grid>
Concepts::StaticallySizedMDRange<2> decltype(auto) basis(const Grid& grid) {
    static constexpr std::size_t dim = dimension<Grid>;
    if constexpr (GridDetail::ExposesBasis<Grid>) {
        using ResultType = std::remove_cvref_t<decltype(Traits::Basis<Grid>::get(grid))>;
        static_assert(static_size<ResultType> == dim);
        static_assert(static_size<std::ranges::range_value_t<ResultType>> == dim);
        return Traits::Basis<Grid>::get(grid);
    }
    else {
        return GridDetail::standard_basis<CoordinateType<Grid>, dim>();
    }
}

template<GridDetail::ExposesCellLocation Grid>
Concepts::StaticallySizedRange decltype(auto) location(const Grid& grid, const Cell<Grid>& c) {
    return Traits::Location<Grid, Cell<Grid>>::get(grid, c);
}

template<GridDetail::ExposesPointLocation Grid>
    requires(not std::same_as<Cell<Grid>, Point<Grid>>)  // avoid ambiguity if cell & point type are the same
Concepts::StaticallySizedRange decltype(auto) location(const Grid& grid, const Point<Grid>& p) {
    return Traits::Location<Grid, Point<Grid>>::get(grid, p);
}

template<GridDetail::ExposesPointRange Grid> requires(GridDetail::ExposesPointId<Grid>)
std::unordered_map<std::size_t, std::size_t> make_point_id_map(const Grid& grid) {
    std::size_t i = 0;
    std::unordered_map<std::size_t, std::size_t> point_id_to_running_idx;
    for (const auto& p : points(grid)) {
        assert(id(grid, p) >= 0);
        point_id_to_running_idx[id(grid, p)] = i++;
    }
    return point_id_to_running_idx;
}

//! \} group Grid

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_GRID_HPP_
