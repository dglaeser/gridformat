// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_GRID_CELL_TYPE_HPP_
#define GRIDFORMAT_GRID_CELL_TYPE_HPP_

namespace GridFormat {

enum class CellType {
    vertex,
    segment,
    triangle,
    quadrilateral,
    tetrahedron,
    hexahedron
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CELL_TYPE_HPP_