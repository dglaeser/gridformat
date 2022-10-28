// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_CONCEPTS_HPP_
#define GRIDFORMAT_COMMON_CONCEPTS_HPP_

#include <type_traits>
#include <concepts>
#include <ostream>

#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Concepts {

#ifndef DOXYGEN
namespace Detail {

template<typename T>
concept ObjectPointer = requires {
    requires std::is_pointer_v<T>;
    requires std::is_object_v<std::remove_pointer_t<T>>;
};

}  // namespace Detail
#endif  // DOXYGEN

template<typename T>
concept Buffer = requires(const T& t) {
    { t.data() } -> Detail::ObjectPointer;
    { t.size() } -> std::integral;
};

template<typename T>
concept Streamable = requires(const T& t, std::ostream& s) {
    { s << t } -> std::same_as<std::ostream&>;
};

template<typename T>
concept Arithmetic = std::integral<T> or std::floating_point<T>;

template<typename T>
concept ScalarRange = std::ranges::range<T> and mdrange_dimension<T> == 1;

template<typename T>
concept VectorRange = std::ranges::range<T> and mdrange_dimension<T> == 2;

template<typename T>
concept TensorRange = std::ranges::range<T> and mdrange_dimension<T> == 3;

template<typename T>
concept ScalarView = ScalarRange<T> and std::ranges::view<T>;

template<typename T>
concept VectorView = VectorRange<T> and std::ranges::view<T>;

template<typename T>
concept TensorView = TensorRange<T> and std::ranges::view<T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_CONCEPTS_HPP_