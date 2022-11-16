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

#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/traits.hpp>
#include <gridformat/grid/type_traits.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Grid
//! \{

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

#ifndef DOXYGEN
namespace Detail {

template<typename F, typename E>
concept EntityFunction = std::invocable<F, const E&>;

}  // namespace Detail
#endif  // DOXYGEN

template<typename T, typename Grid>
concept PointFunction = Detail::EntityFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept CellFunction = Detail::EntityFunction<T, Cell<Grid>>;

template<typename T, typename Grid>
concept EntityFunction = PointFunction<T, Grid> or CellFunction<T, Grid>;

template<typename T, typename Entity>
concept ScalarFunction = Detail::EntityFunction<T, Entity> and Scalar<GridDetail::EntityFunctionValueType<T, Entity>>;

template<typename T, typename Entity>
concept VectorFunction = Detail::EntityFunction<T, Entity> and Vector<GridDetail::EntityFunctionValueType<T, Entity>>;

template<typename T, typename Entity>
concept TensorFunction = Detail::EntityFunction<T, Entity> and Tensor<GridDetail::EntityFunctionValueType<T, Entity>>;

template<typename T, typename Grid>
concept ScalarPointFunction = PointFunction<T, Grid> and ScalarFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept ScalarCellFunction = CellFunction<T, Grid> and ScalarFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept VectorPointFunction = PointFunction<T, Grid> and VectorFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept VectorCellFunction = CellFunction<T, Grid> and VectorFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept TensorPointFunction = PointFunction<T, Grid> and TensorFunction<T, Point<Grid>>;

template<typename T, typename Grid>
concept TensorCellFunction = CellFunction<T, Grid> and TensorFunction<T, Point<Grid>>;

//! \} group Grid

}  // namespace GridFormat::Concepts

#endif  // GRIDFORMAT_GRID_CONCEPTS_HPP_