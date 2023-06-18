// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
#ifndef GRIDFORMAT_GRID_DETAIL_HPP_
#define GRIDFORMAT_GRID_DETAIL_HPP_
#ifndef DOXYGEN

#include <ranges>
#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/precision.hpp>

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
    using Entity = std::remove_cvref_t<EntityReference<Grid, Range>>;

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
        { Traits::PointCoordinates<T, Point<T>>::get(grid, std::declval<const Point<T>&>()) } -> Concepts::StaticallySizedRange;
    };

    template<ExposesPointCoordinates T>
    using PointCoordinates = std::remove_cvref_t<decltype(
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
    concept ExposesNumberOfCellPoints = is_complete<Traits::NumberOfCellPoints<T, Cell<T>>> && requires(const T& grid, const Cell<T>& cell) {
            { Traits::NumberOfCellPoints<T, Cell<T>>::get(grid, cell) } -> std::convertible_to<std::size_t>;
        };

    template<typename T>
    concept ExposesOrigin = is_complete<Traits::Origin<T>> && requires(const T& grid) {
        { Traits::Origin<T>::get(grid) } -> Concepts::StaticallySizedMDRange<1>;
    };

    template<typename T>
    concept ExposesSpacing = is_complete<Traits::Spacing<T>> && requires(const T& grid) {
        { Traits::Spacing<T>::get(grid) } -> Concepts::StaticallySizedMDRange<1>;
    };

    template<typename T>
    concept ExposesBasis = is_complete<Traits::Basis<T>> && requires(const T& grid) {
        { Traits::Basis<T>::get(grid) } -> Concepts::StaticallySizedMDRange<2>;
    };

    template<typename T>
    concept ExposesExtents = is_complete<Traits::Extents<T>> && requires(const T& grid) {
        { Traits::Extents<T>::get(grid) } -> Concepts::StaticallySizedMDRange<1>;
        requires std::convertible_to<
            std::ranges::range_value_t<decltype(Traits::Extents<T>::get(grid))>,
            std::size_t
        >;
    };

    template<typename T>
    concept ExposesOrdinates = is_complete<Traits::Ordinates<T>> && requires(const T& grid) {
        { Traits::Ordinates<T>::get(grid, unsigned{}) } -> Concepts::MDRange<1>;
        requires Concepts::Scalar<
            MDRangeValueType<std::remove_cvref_t<decltype(Traits::Ordinates<T>::get(grid, unsigned{}))>>
        >;
    };

    template<typename T>
    concept ExposesCellLocation = is_complete<Traits::Location<T, Cell<T>>> && requires(const T& grid, const Cell<T>& cell) {
        { Traits::Location<T, Cell<T>>::get(grid, cell) } -> Concepts::StaticallySizedMDRange<1>;
        requires std::convertible_to<
            std::ranges::range_value_t<decltype(Traits::Location<T, Cell<T>>::get(grid, cell))>,
            std::size_t
        >;
    };

    template<typename T>
    concept ExposesPointLocation = is_complete<Traits::Location<T, Point<T>>> && requires(const T& grid, const Point<T>& point) {
        { Traits::Location<T, Point<T>>::get(grid, point) } -> Concepts::StaticallySizedMDRange<1>;
        requires std::convertible_to<
            std::ranges::range_value_t<decltype(Traits::Location<T, Point<T>>::get(grid, point))>,
            std::size_t
        >;
    };

    template<ExposesSpacing T>
    using Spacing = std::remove_cvref_t<decltype(Traits::Spacing<T>::get(std::declval<const T&>()))>;

    template<ExposesOrigin T>
    using Origin = std::remove_cvref_t<decltype(Traits::Origin<T>::get(std::declval<const T&>()))>;

    template<ExposesOrdinates T>
    using Ordinates = std::remove_cvref_t<decltype(Traits::Ordinates<T>::get(std::declval<const T&>(), unsigned{}))>;

    template<ExposesExtents T>
    using Extents = std::remove_cvref_t<decltype(Traits::Extents<T>::get(std::declval<const T&>()))>;

    template<ExposesBasis T>
    using Basis = std::remove_cvref_t<decltype(Traits::Basis<T>::get(std::declval<const T&>()))>;

    template<ExposesCellLocation T>
    using CellLocation = std::remove_cvref_t<decltype(Traits::Location<T, Cell<T>>::get(std::declval<const T&>(), std::declval<const Cell<T>>()))>;

    template<ExposesPointLocation T>
    using PointLocation = std::remove_cvref_t<decltype(Traits::Location<T, Point<T>>::get(std::declval<const T&>(), std::declval<const Point<T>>()))>;

    template<typename Grid, std::invocable<PointReference<Grid>> T>
    using PointFunctionValueType = std::remove_cvref_t<std::invoke_result_t<T, PointReference<Grid>>>;

    template<typename Grid, std::invocable<CellReference<Grid>> T>
    using CellFunctionValueType = std::remove_cvref_t<std::invoke_result_t<T, CellReference<Grid>>>;

    template<Concepts::Scalar T>
    using EntityFunctionScalarType = std::conditional_t<std::is_same_v<T, bool>, typename UInt8::T, T>;

    template<typename Grid, std::invocable<PointReference<Grid>> T>
    using PointFunctionScalarType = EntityFunctionScalarType<FieldScalar<PointFunctionValueType<Grid, T>>>;

    template<typename Grid, std::invocable<CellReference<Grid>> T>
    using CellFunctionScalarType = EntityFunctionScalarType<FieldScalar<CellFunctionValueType<Grid, T>>>;

}  // namespace GridFormat::GridDetail
#endif // DOXYGEN
#endif  // GRIDFORMAT_GRID_DETAIL_HPP_
