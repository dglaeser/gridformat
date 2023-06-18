// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \brief Grid concepts.
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

/*!
 * \brief Basic concept all grids must fulfill.
 *        We must be able to iterate over a grid's cells & points.
 */
template<typename T>
concept EntitySet =
    GridDetail::ExposesPointRange<T> and
    GridDetail::ExposesCellRange<T>;

/*!
 * \brief Basic concept for all grids with a structured topology.
 *        We need to know the extents of the grid and need to be able to retrieve
 *        the location of a cell/point in the topology.
 */
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

/*!
 * \brief Concept for grids to be used as image grids.
 */
template<typename T>
concept ImageGrid =
    StructuredEntitySet<T> and
    GridDetail::ExposesOrigin<T> and
    GridDetail::ExposesSpacing<T> and
    all_equal<
        static_size<GridDetail::Origin<T>>,
        static_size<GridDetail::Spacing<T>>
    >;

/*!
 * \brief Concept for grids to be used as rectilinear grids.
 */
template<typename T>
concept RectilinearGrid =
    StructuredEntitySet<T> and
    GridDetail::ExposesOrdinates<T>;

/*!
 * \brief Concept for grids to be used as structured grids.
 */
template<typename T>
concept StructuredGrid =
    StructuredEntitySet<T> and
    GridDetail::ExposesPointCoordinates<T>;

/*!
 * \brief Concept for grids to be used as unstructured grids.
 */
template<typename T>
concept UnstructuredGrid =
    EntitySet<T> and
    GridDetail::ExposesPointCoordinates<T> and
    GridDetail::ExposesPointId<T> and
    GridDetail::ExposesCellType<T> and
    GridDetail::ExposesCellPoints<T>;

/*!
 * \brief Concept a type that fulfills any of the grid interfaces
 */
template<typename T>
concept Grid = ImageGrid<T> or RectilinearGrid<T> or StructuredGrid<T> or UnstructuredGrid<T>;

/*!
 * \brief Concept for functions invocable with grid points, usable as point field data.
 */
template<typename T, typename Grid>
concept PointFunction
    = std::invocable<T, GridDetail::PointReference<Grid>>
    and is_scalar<GridDetail::PointFunctionScalarType<Grid, T>>;

/*!
 * \brief Concept for functions invocable with grid cells, usable as cell field data.
 */
template<typename T, typename Grid>
concept CellFunction
    = std::invocable<T, GridDetail::CellReference<Grid>>
    and is_scalar<GridDetail::CellFunctionScalarType<Grid, T>>;

//! \} group Grid

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_GRID_CONCEPTS_HPP_
