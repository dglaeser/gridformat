// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_COMMON_HPP_
#define GRIDFORMAT_VTK_COMMON_HPP_

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/grid/cell_type.hpp>

namespace GridFormat::VTK {

inline constexpr std::uint8_t cell_type_number(CellType t) {
    switch (t) {
        case (CellType::vertex): return 1;
        case (CellType::segment): return 3;
        case (CellType::triangle): return 5;
        case (CellType::quadrilateral): return 9;
        case (CellType::tetrahedron): return 10;
        case (CellType::hexahedron): return 12;
    }

    throw NotImplemented("VTK cell type for the given cell type");
}

inline std::string data_format(DynamicPrecision prec) {
    std::string prefix = prec.is_integral() ? (prec.is_signed() ? "Int" : "UInt") : "Float";
    return prefix + std::to_string(prec.number_of_bytes()*8);
}

namespace DataFormat {

struct Inline {};
struct Appended {};

}  // namespace DataFormat

namespace Encoding {

struct Base64 {};
struct Ascii {};
struct Raw {};

}  // namespace Encoding

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_COMMON_HPP_