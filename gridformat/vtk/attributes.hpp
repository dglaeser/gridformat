// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_ATTRIBUTES_HPP_
#define GRIDFORMAT_VTK_ATTRIBUTES_HPP_

#include <gridformat/common/precision.hpp>

namespace GridFormat::VTK {

std::string attribute_name(const PrecisionTraits& prec) {
    std::string prefix = prec.is_integral ? (prec.is_signed ? "Int" : "UInt") : "Float";
    return prefix + std::to_string(prec.number_of_bytes*8);
}

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_ATTRIBUTES_HPP_