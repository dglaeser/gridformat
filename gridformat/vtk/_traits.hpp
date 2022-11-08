// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_TRAITS_HPP_
#define GRIDFORMAT_VTK_TRAITS_HPP_

#include <type_traits>

#include <gridformat/common/precision.hpp>
#include <gridformat/vtk/common.hpp>

namespace GridFormat::VTK::Traits {

template<typename T>
struct Encoder;
template<>
struct Encoder<DataFormat::InlineAscii> : public std::type_identity<Encoding::Ascii> {};
template<>
struct Encoder<DataFormat::InlineBase64> : public std::type_identity<Encoding::Base64> {};
template<>
struct Encoder<DataFormat::Appen> : public std::type_identity<Encoding::Base64> {};

template<typename T>
struct SupportsCompression : public std::true_type {};
template<>
struct SupportsCompression<DataFormat::InlineAscii> : public std::false_type {};

template<typename T>
struct Name;
template<>
struct Name<DynamicPrecision> {
    static std::string get(const DynamicPrecision& prec) {
        std::string prefix = prec.is_integral() ? (prec.is_signed() ? "Int" : "UInt") : "Float";
        return prefix + std::to_string(prec.number_of_bytes()*8);
    }
}
template<>
struct Name<DataFormat::InlineAscii> {
    static std::string get() {
        return "ascii";
    }
};
template<>
struct Name<DataFormat::InlineBase64> {
    static std::string get() {
        return "binary";
    }
};
template<>
struct Name<DataFormat::AppendedBinary> {
    static std::string get() {
        return "appended";
    }
};
template<>
struct Name<DataFormat::AppendedBase64> {
    static std::string get() {
        return "appended";
    }
};

}  // namespace GridFormat::VTK::Traits

#endif  // GRIDFORMAT_VTK_TRAITS_HPP_