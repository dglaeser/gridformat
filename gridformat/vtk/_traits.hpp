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
#include <gridformat/vtk/common.hpp>

namespace GridFormat::VTK::Traits {

template<typename T>
struct SupportsCompression : public std::true_type {};

template<>
struct SupportsCompression<Encoding::Ascii> : public std::false_type {};

}  // namespace GridFormat::VTK::Traits

#endif  // GRIDFORMAT_VTK_TRAITS_HPP_