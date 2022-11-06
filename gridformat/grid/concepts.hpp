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
    Detail::exposes_point_range<T> and
    Detail::exposes_cell_range<T> and
    Detail::exposes_point_coordinates<T> and
    Detail::exposes_point_id<T> and
    Detail::exposes_cell_type<T> and
    Detail::exposes_cell_corners<T>;

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_GRID_CONCEPTS_HPP_