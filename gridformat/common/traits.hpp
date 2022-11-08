// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_TRAITS_HPP_
#define GRIDFORMAT_COMMON_TRAITS_HPP_

#include <utility>
#include <type_traits>

#include <gridformat/common/concepts.hpp>

namespace GridFormat {
namespace Traits {

template<typename T>
struct Byte;

template<Concepts::Serialization T>
struct Byte<T>
: public std::type_identity<
    std::decay_t<std::remove_pointer_t<decltype(std::declval<const T&>().data())>>
> {};

}  // namespace Traits

template<typename T>
using ByteType = typename Traits::Byte<T>::type;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TRAITS_HPP_