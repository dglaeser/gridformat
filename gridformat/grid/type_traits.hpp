// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRID_TYPE_TRAITS_HPP_
#define GRIDFORMAT_GRID_TYPE_TRAITS_HPP_

#include <ranges>

#include <gridformat/grid/_detail.hpp>

namespace GridFormat {

template<typename T>
using Point = Detail::Point<T>;

template<typename T>
using Cell = Detail::Cell<T>;

template<typename T>
using CoordinateType = std::ranges::range_value_t<Point<T>>;

}  // namespace Traits

#endif  // GRIDFORMAT_GRID_TYPE_TRAITS_HPP_