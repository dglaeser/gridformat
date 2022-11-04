// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRID_TRAITS_HPP_
#define GRIDFORMAT_GRID_TRAITS_HPP_

namespace GridFormat::Traits {

//! Exposes the range over all points of the grid via a static function `get(const Grid&)`
template<typename Grid>
struct Points;

//! Exposes the range over all cells of the grid via a static function `get(const Grid&)`
template<typename Grid>
struct Cells;

//! Get the corner points of a cell via a static function `get(const Grid&, const Cell&)`
template<typename Grid, typename Cell>
struct CellCornerPoints;

//! Get the type of a cell via a static function `get(const Grid&, const Cell&)`
template<typename Grid, typename Cell>
struct CellType;

//! Provides access to the coordinates of a grid point via a static function `get(const Grid&, const Point&)`
template<typename Grid, typename Point>
struct PointCoordinates;

//! Metafunction to obtain a unique id for a point via a static function `get(const Grid&, const Point&)`
template<typename Grid, typename Point>
struct PointId;

}  // namespace GridFormat::Traits

#endif  // GRIDFORMAT_GRID_TRAITS_HPP_