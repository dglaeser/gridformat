// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \brief Traits classes for grid implementations to specialize.
 */
#ifndef GRIDFORMAT_GRID_TYPE_TRAITS_HPP_
#define GRIDFORMAT_GRID_TYPE_TRAITS_HPP_

#include <ranges>

#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/traits.hpp>

namespace GridFormat {

//! \addtogroup Grid
//! \{

template<GridDetail::ExposesPointRange Grid>
using Point = GridDetail::Point<Grid>;

template<GridDetail::ExposesCellRange Grid>
using Cell = GridDetail::Cell<Grid>;

template<GridDetail::ExposesPointCoordinates T>
using CoordinateType = std::ranges::range_value_t<GridDetail::PointCoordinates<T>>;

//! \} group Grid

}

#endif  // GRIDFORMAT_GRID_TYPE_TRAITS_HPP_
