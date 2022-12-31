// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \brief Traits classes for grid implementations to specialize.
 */
#ifndef GRIDFORMAT_GRID_TRAITS_HPP_
#define GRIDFORMAT_GRID_TRAITS_HPP_

namespace GridFormat::Traits {

//! \addtogroup Grid
//! \{

//! Exposes the range over all points of the grid via a static function `get(const Grid&)`
template<typename Grid>
struct Points;

//! Exposes the range over all cells of the grid via a static function `get(const Grid&)`
template<typename Grid>
struct Cells;

//! Exposes the number of points in a grid via a static function `get(const Grid&)`
template<typename Grid>
struct NumberOfPoints;

//! Exposes the number of cells in a grid via a static function `get(const Grid&)`
template<typename Grid>
struct NumberOfCells;

//! Get the points of a cell via a static function `get(const Grid&, const Cell&)`
template<typename Grid, typename Cell>
struct CellPoints;

//! Get the type of a cell via a static function `get(const Grid&, const Cell&)`
template<typename Grid, typename Cell>
struct CellType;

//! Provides access to the coordinates of a grid point via a static function `get(const Grid&, const Point&)`
template<typename Grid, typename Point>
struct PointCoordinates;

//! Metafunction to obtain a unique id for a point via a static function `get(const Grid&, const Point&)`
template<typename Grid, typename Point>
struct PointId;

//! \} group Grid

}  // namespace GridFormat::Traits

#endif  // GRIDFORMAT_GRID_TRAITS_HPP_
