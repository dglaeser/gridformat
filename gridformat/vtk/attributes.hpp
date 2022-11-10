// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_ATTRIBUTES_HPP_
#define GRIDFORMAT_VTK_ATTRIBUTES_HPP_

#include <bit>
#include <type_traits>

#include <gridformat/common/precision.hpp>
#include <gridformat/compression/compression.hpp>
#include <gridformat/vtk/options.hpp>

namespace GridFormat::VTK {

std::string attribute_name(const DynamicPrecision& prec) {
    std::string prefix = prec.is_integral() ? (prec.is_signed() ? "Int" : "UInt") : "Float";
    return prefix + std::to_string(prec.number_of_bytes()*8);
}

std::string attribute_name(std::endian e) {
    return e == std::endian::little ? "LittleEndian" : "BigEndian";
}

std::string attribute_name(const Encoding::Ascii&) { return "ascii"; }
std::string attribute_name(const Encoding::Base64&) { return "base64"; }
std::string attribute_name(const Encoding::RawBinary&) { return "raw"; }

std::string attribute_name(const Compression::LZMA&) { return "vtkLZMADataCompressor"; };

template<typename Encoding>
std::string data_format_name(const Encoding&, const DataFormat::Inlined&) { return "appended"; }
std::string data_format_name(const Encoding::Ascii&, const DataFormat::Inlined&) { return "ascii"; }
std::string data_format_name(const Encoding::Base64&, const DataFormat::Inlined&) { return "base64"; }

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_ATTRIBUTES_HPP_