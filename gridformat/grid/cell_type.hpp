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
    hexahedron
};


//! Return the dimension of a grid cell type
inline int cell_dimension(CellType ct) {
    switch (ct) {
        case CellType::vertex: return 0;
        case CellType::segment: return 1;
        case CellType::triangle: return 2;
        case CellType::pixel: return 2;
        case CellType::quadrilateral: return 2;
        case CellType::polygon: return 2;
        case CellType::tetrahedron: return 3;
        case CellType::hexahedron: return 3;
    }

    throw InvalidState(as_error("Unsupported cell type"));
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CELL_TYPE_HPP_
