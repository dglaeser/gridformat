// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_OPTIONS_HPP_
#define GRIDFORMAT_VTK_OPTIONS_HPP_

namespace GridFormat::VTK {

namespace DataFormat {

struct Inlined {};
struct Appended {};

inline constexpr Inlined inlined;
inline constexpr Appended appended;

}  // namespace DataFormat

// namespace Encoding {

// struct Base64 {};
// struct Ascii {};
// struct RawBinary {};

// inline constexpr Base64 base64;
// inline constexpr Ascii ascii;
// inline constexpr RawBinary raw_binary;

// }  // namespace Encoding

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_OPTIONS_HPP_