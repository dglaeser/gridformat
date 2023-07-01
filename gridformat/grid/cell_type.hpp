// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \copydoc GridFormat::CellType
 */
#ifndef GRIDFORMAT_GRID_CELL_TYPE_HPP_
#define GRIDFORMAT_GRID_CELL_TYPE_HPP_

#include <ostream>
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

std::string as_string(const CellType& ct) {
    switch (ct) {
        case CellType::vertex: return "vertex";
        case CellType::segment: return "segment";
        case CellType::triangle: return "triangle";
        case CellType::pixel: return "pixel";
        case CellType::quadrilateral: return "quadrilateral";
        case CellType::polygon: return "polygon";
        case CellType::tetrahedron: return "tetrahedron";
        case CellType::hexahedron: return "hexahedron";
        case CellType::voxel: return "voxel";
        case CellType::lagrange_segment: return "lagrange_segment";
        case CellType::lagrange_triangle: return "lagrange_triangle";
        case CellType::lagrange_quadrilateral: return "lagrange_quadrilateral";
        case CellType::lagrange_tetrahedron: return "lagrange_tetrahedron";
        case CellType::lagrange_hexahedron: return "lagrange_hexahedron";
        default: throw NotImplemented("String representation for given cell type");
    }
}

std::ostream& operator<<(std::ostream& s, const CellType& ct) {
    s << as_string(ct);
    return s;
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_CELL_TYPE_HPP_
