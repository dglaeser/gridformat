// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Grid
 * \copydoc GridFormat::CellType
 */
#ifndef GRIDFORMAT_GRID_CELL_TYPE_HPP_
#define GRIDFORMAT_GRID_CELL_TYPE_HPP_

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/logging.hpp>

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
    voxel
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CELL_TYPE_HPP_
