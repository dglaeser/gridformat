// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRID_CONCEPTS_HPP_
#define GRIDFORMAT_GRID_CONCEPTS_HPP_

#include <gridformat/grid/_detail.hpp>

namespace GridFormat::Concepts {

template<typename T>
concept UnstructuredGrid =
    GridDetail::exposes_point_range<T> and
    GridDetail::exposes_cell_range<T> and
    GridDetail::exposes_point_coordinates<T> and
    GridDetail::exposes_point_id<T> and
    GridDetail::exposes_cell_type<T> and
    GridDetail::exposes_cell_corners<T>;

template<typename T>
concept Grid = UnstructuredGrid<T>;

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_GRID_CONCEPTS_HPP_