// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \copydoc GridFormat::CellType
 */
#ifndef GRIDFORMAT_GRID_CELL_TYPE_HPP_
#define GRIDFORMAT_GRID_CELL_TYPE_HPP_

#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

/*!
 * \ingroup Grid
 * \brief Defines the supported cell types
 */
enum class CellType {
    vertex,

    segment,
    triangle,
    pixel,
    quadrilateral,
    polygon,

    tetrahedron,
    hexahedron,
    voxel,

    lagrange_segment,
    lagrange_triangle,
    lagrange_quadrilateral,
    lagrange_tetrahedron,
    lagrange_hexahedron
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CELL_TYPE_HPP_
