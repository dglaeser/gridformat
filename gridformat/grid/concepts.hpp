// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \brief Common concepts related to grids
 */
#ifndef GRIDFORMAT_GRID_CONCEPTS_HPP_
#define GRIDFORMAT_GRID_CONCEPTS_HPP_

#include <concepts>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Grid
//! \{

template<typename T>
concept EntitySet =
    GridDetail::ExposesPointRange<T> and
    GridDetail::ExposesCellRange<T>;

template<typename T>
concept StructuredEntitySet =
    EntitySet<T> and
    GridDetail::ExposesExtents<T> and
    GridDetail::ExposesCellLocation<T> and
    GridDetail::ExposesPointLocation<T> and
    all_equal<
        static_size<GridDetail::Extents<T>>,
        static_size<GridDetail::CellLocation<T>>,
        static_size<GridDetail::PointLocation<T>>
    >;

template<typename T>
concept ImageGrid =
    StructuredEntitySet<T> and
    GridDetail::ExposesOrigin<T> and
    GridDetail::ExposesSpacing<T> and
    all_equal<
        static_size<GridDetail::Origin<T>>,
        static_size<GridDetail::Spacing<T>>
    >;

template<typename T>
concept RectilinearGrid =
    StructuredEntitySet<T> and
    GridDetail::ExposesOrdinates<T>;

template<typename T>
concept StructuredGrid =
    StructuredEntitySet<T> and
    GridDetail::ExposesPointCoordinates<T>;

template<typename T>
concept UnstructuredGrid =
    EntitySet<T> and
    GridDetail::ExposesPointCoordinates<T> and
    GridDetail::ExposesPointId<T> and
    GridDetail::ExposesCellType<T> and
    GridDetail::ExposesCellPoints<T>;

template<typename T>
concept Grid = ImageGrid<T> or RectilinearGrid<T> or StructuredGrid<T> or UnstructuredGrid<T>;

template<typename T, typename Grid>
concept PointFunction
    = std::invocable<T, GridDetail::PointReference<Grid>>
    and is_scalar<GridDetail::PointFunctionScalarType<Grid, T>>;

template<typename T, typename Grid>
concept CellFunction
    = std::invocable<T, GridDetail::CellReference<Grid>>
    and is_scalar<GridDetail::CellFunctionScalarType<Grid, T>>;

//! \} group Grid

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_GRID_CONCEPTS_HPP_
