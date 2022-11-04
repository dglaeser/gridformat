// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_CONCEPTS_HPP_
#define GRIDFORMAT_COMMON_CONCEPTS_HPP_

#include <concepts>
#include <ostream>

#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Concepts {

template<typename T>
concept Streamable = requires(const T& t, std::ostream& s) {
    { s << t } -> std::same_as<std::ostream&>;
};

template<typename T, typename ValueType>
concept RangeOf = std::ranges::range<T> and std::convertible_to<std::ranges::range_value_t<T>, ValueType>;

template<typename T, std::size_t dim>
concept MDRange = std::ranges::range<T> and mdrange_dimension<T> == dim;

template<typename T>
concept Scalar = std::integral<T> or std::floating_point<T>;

template<typename T>
concept Vector = MDRange<T, 1>;

template<typename T>
concept Tensor = MDRange<T, 2>;

template<typename T>
concept Scalars = std::ranges::range<T> and Scalar<std::ranges::range_value_t<T>>;

template<typename T>
concept Vectors = std::ranges::range<T> and Vector<std::ranges::range_value_t<T>>;

template<typename T>
concept Tensors = std::ranges::range<T> and Tensor<std::ranges::range_value_t<T>>;

template<typename T>
concept ScalarsView = Scalars<T> and std::ranges::view<T>;

template<typename T>
concept VectorsView = Vectors<T> and std::ranges::view<T>;

template<typename T>
concept TensorsView = Tensors<T> and std::ranges::view<T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_CONCEPTS_HPP_