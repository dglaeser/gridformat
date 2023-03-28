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

    template<typename T>
    concept ViewableForwardRange = std::ranges::viewable_range<T> and std::ranges::forward_range<T>;

    template<typename T, typename Grid>
    concept EntityRange = requires(const Grid& grid) {
        { T::get(grid) } -> ViewableForwardRange;
    };

    template<typename Grid, EntityRange<Grid> Range>
    using EntityReference = std::ranges::range_reference_t<decltype(Range::get(std::declval<const Grid&>()))>;

    template<typename Grid, EntityRange<Grid> Range>
    using Entity = std::decay_t<EntityReference<Grid, Range>>;

    template<typename T>
    concept ExposesPointRange = is_complete<Traits::Points<T>> && EntityRange<Traits::Points<T>, T>;

    template<typename T>
    concept ExposesCellRange = is_complete<Traits::Cells<T>> && EntityRange<Traits::Cells<T>, T>;

    template<ExposesPointRange Grid>
    using Point = Entity<Grid, Traits::Points<Grid>>;

    template<ExposesPointRange Grid>
    using PointReference = EntityReference<Grid, Traits::Points<Grid>>;

    template<ExposesCellRange Grid>
    using Cell = Entity<Grid, Traits::Cells<Grid>>;

    template<ExposesCellRange Grid>
    using CellReference = EntityReference<Grid, Traits::Cells<Grid>>;

    template<typename T>
    concept ExposesPointCoordinates = is_complete<Traits::PointCoordinates<T, Point<T>>> && requires(const T& grid) {
        { Traits::PointCoordinates<T, Point<T>>::get(grid, std::declval<const Point<T>&>()) } -> std::ranges::range;
    };

    template<ExposesPointCoordinates T>
    using PointCoordinates = std::decay_t<decltype(
        Traits::PointCoordinates<T, Point<T>>::get(std::declval<const T&>(), std::declval<const Point<T>&>())
    )>;

    template<typename T>
    concept ExposesPointId = is_complete<Traits::PointId<T, Point<T>>> && requires(const T& grid) {
        { Traits::PointId<T, Point<T>>::get(grid, std::declval<const Point<T>&>()) } -> std::integral;
    };

    template<typename T>
    concept ExposesCellType = is_complete<Traits::CellType<T, Cell<T>>> && requires(const T& grid) {
        { Traits::CellType<T, Cell<T>>::get(grid, std::declval<const Cell<T>&>()) } -> std::convertible_to<GridFormat::CellType>;
    };

    template<typename T>
    concept ExposesCellPoints = is_complete<Traits::CellPoints<T, Cell<T>>> && requires(const T& grid) {
        { Traits::CellPoints<T, Cell<T>>::get(grid, std::declval<const Cell<T>&>()) } -> Concepts::RangeOf<Point<T>>;
    };

    template<typename T>
    concept ExposesNumberOfPoints = is_complete<Traits::NumberOfPoints<T>> && requires(const T& grid) {
            { Traits::NumberOfPoints<T>::get(grid) } -> std::convertible_to<std::size_t>;
        };

    template<typename T>
    concept ExposesNumberOfCells = is_complete<Traits::NumberOfCells<T>> && requires(const T& grid) {
            { Traits::NumberOfCells<T>::get(grid) } -> std::convertible_to<std::size_t>;
        };

    template<typename T>
    concept ExposesNumberOfCellCorners = is_complete<Traits::NumberOfCellCorners<T>> && requires(const T& grid, const Cell<T>& cell) {
            { Traits::NumberOfCellCorners<T>::get(grid, cell) } -> std::convertible_to<std::size_t>;
        };

    template<typename Grid, std::invocable<PointReference<Grid>> T>
    using PointFunctionValueType = std::decay_t<std::invoke_result_t<T, PointReference<Grid>>>;

    template<typename Grid, std::invocable<CellReference<Grid>> T>
    using CellFunctionValueType = std::decay_t<std::invoke_result_t<T, CellReference<Grid>>>;

    template<typename Grid, std::invocable<PointReference<Grid>> T>
    using PointFunctionScalarType = FieldScalar<PointFunctionValueType<Grid, T>>;

    template<typename Grid, std::invocable<CellReference<Grid>> T>
    using CellFunctionScalarType = FieldScalar<CellFunctionValueType<Grid, T>>;

}  // namespace GridFormat::GridDetail
#endif // DOXYGEN
#endif  // GRIDFORMAT_GRID_DETAIL_HPP_
