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
#include <cstddef>
#include <type_traits>
#include <array>
#include <span>
#include <ios>

#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Concepts {

#ifndef DOXYGEN
namespace Detail {

template<typename T>
struct is_std_array : public std::false_type {};
template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : public std::true_type {};
template<typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

template<typename T>
struct is_std_span : public std::false_type {};
template<typename T, std::size_t N>
struct is_std_span<std::span<T, N>> : public std::true_type {};
template<typename T>
inline constexpr bool is_std_span_v = is_std_span<T>::value;

}  // namespace Detail
#endif  // DOXYGEN

template<typename T1, typename T2>
concept Interoperable = std::is_convertible_v<T1, T2> || std::is_convertible_v<T2, T1>;

template<typename T, typename Data = char>
concept Stream = requires(T& t, const std::span<Data> data) {
    { t.write(data) };
};

template<typename T, typename Data = char>
concept FormattedStream = requires(T& t, const Data& data) {
    { t << data } -> std::same_as<T&>;
};

template<typename T, typename S = std::ostream>
concept Encoder = Stream<S> and requires(const T& encoder, S& stream) {
    { encoder(stream) } -> Stream;
};

template<typename T, typename Stream = std::ostream&>
concept Streamable = requires(const T& t, Stream& s) {
    { s << t } -> std::same_as<Stream&>;
};

template<typename T>
concept Serialization = requires(const T& t_const, T& t) {
    { t_const.size() } -> std::integral;
    { t_const.data() } -> std::convertible_to<const std::byte*>;
    { t.data() } -> std::convertible_to<std::byte*>;
    { t.resize(std::size_t{}) };
};

template<typename T>
concept StaticallySized
    = requires { { T::size() } -> std::integral; }
    or (!is_incomplete<Traits::StaticSize<T>> and requires {
            { Traits::StaticSize<T>::value } -> std::convertible_to<std::size_t>;
        })
    or std::is_bounded_array_v<T>
    or Detail::is_std_array_v<T>
    or (Detail::is_std_span_v<T> and T::extent != std::dynamic_extent);

template<typename T, typename ValueType>
concept RangeOf = std::ranges::range<T> and std::convertible_to<std::ranges::range_value_t<T>, ValueType>;

template<typename T>
concept FieldValuesRange = std::ranges::forward_range<T>;


template<typename T, std::size_t dim>
concept MDRange = std::ranges::range<T> and mdrange_dimension<T> == dim;

template<typename T>
concept Scalar = std::integral<T> or std::floating_point<T>;

template<typename T>
concept Vector = MDRange<T, 1>;

template<typename T>
concept Tensor = MDRange<T, 2>;

template<typename T>
concept Scalars = std::ranges::forward_range<T> and Scalar<std::ranges::range_value_t<T>>;

template<typename T>
concept Vectors = std::ranges::forward_range<T> and Vector<std::ranges::range_value_t<T>>;

template<typename T>
concept Tensors = std::ranges::forward_range<T> and Tensor<std::ranges::range_value_t<T>>;

template<typename T>
concept ScalarsView = Scalars<T> and std::ranges::view<T>;

template<typename T>
concept VectorsView = Vectors<T> and std::ranges::view<T>;

template<typename T>
concept TensorsView = Tensors<T> and std::ranges::view<T>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_CONCEPTS_HPP_