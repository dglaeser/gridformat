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
concept UnstructuredGrid =
    GridDetail::ExposesPointRange<T> and
    GridDetail::ExposesCellRange<T> and
    GridDetail::ExposesPointCoordinates<T> and
    GridDetail::ExposesPointId<T> and
    GridDetail::ExposesCellType<T> and
    GridDetail::ExposesCellPoints<T>;

template<typename T>
concept Grid = UnstructuredGrid<T>;

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
