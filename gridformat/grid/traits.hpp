// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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

//! Exposes the number of points in a grid via a static function `get(const Grid&)` (optional trait)
template<typename Grid>
struct NumberOfPoints;

//! Exposes the number of cells in a grid via a static function `get(const Grid&)` (optional trait)
template<typename Grid>
struct NumberOfCells;

//! \addtogroup UnstructuredGrid
//! \{

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

//! Exposes the number of points of a cell via a static function `get(const Grid&, const Cell&)` (optional trait)
template<typename Grid, typename Cell>
struct NumberOfCellPoints;

//! \} group UnstructuredGrid

//! \addtogroup StructuredGrid
//! \{

//! Metafunction to obtain the origin of a structured grid via a static function `get(const Grid&)`
template<typename Grid>
struct Origin;

//! Metafunction to obtain the spacing of a structured grid via a static function `get(const Grid&)`
template<typename Grid>
struct Spacing;

//! Metafunction to obtain the Basis of a structured grid via a static function `get(const Grid&)` (optional trait)
template<typename Grid>
struct Basis;

//! Metafunction to obtain the number of cells per direction via a static function `get(const Grid&)`
template<typename Grid>
struct Extents;

//! Metafunction to obtain the ordinates of a rectilinear grid via a static function `get(const Grid&, int direction)`
template<typename Grid>
struct Ordinates;

//! Metafunction to obtain the location (MDIndex) of an entity in a structured grid via a static function `get(const Grid&, const Entity&)`
template<typename Grid, typename Entity>
struct Location;

//! \} group StructuredGrid

//! \} group Grid

}  // namespace GridFormat::Traits

#endif  // GRIDFORMAT_GRID_TRAITS_HPP_
