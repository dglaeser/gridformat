// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Basic concept definitions
 */
#ifndef GRIDFORMAT_COMMON_CONCEPTS_HPP_
#define GRIDFORMAT_COMMON_CONCEPTS_HPP_

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <limits>
#include <array>
#include <span>

#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Concepts {

//! \addtogroup Common
//! \{

template<typename T>
concept StaticallySizedRange
    = std::ranges::range<T> and has_static_size<T> and requires {
        { static_size<T> } -> std::convertible_to<std::size_t>;
    };

template<typename T1, typename T2>
concept Interoperable = std::is_convertible_v<T1, T2> || std::is_convertible_v<T2, T1>;

template<typename T, typename Stream>
concept StreamableWith = requires(const T& t, Stream& s) {
    { s << t } -> std::same_as<Stream&>;
};

template<typename T, typename Data>
concept WriterFor = requires(T& t, Data data) {
    { t.write(data) };
};

template<typename T, typename Writer>
concept WritableWith = WriterFor<std::remove_reference_t<Writer>, T>;

template<typename T, typename ValueType>
concept RangeOf = std::ranges::range<T> and std::convertible_to<std::ranges::range_value_t<T>, ValueType>;

template<typename T, std::size_t dim>
concept MDRange = std::ranges::range<T> and mdrange_dimension<T> == dim;


#ifndef DOXYGEN
namespace Detail {

    template<typename T>
    struct HasStaticallySizedSubRanges : public std::false_type {};
    template<std::ranges::range R> requires(!has_sub_range<R> and has_static_size<R>)
    struct HasStaticallySizedSubRanges<R> : public std::true_type {};
    template<std::ranges::range R> requires(has_sub_range<R> and has_static_size<R>)
    struct HasStaticallySizedSubRanges<R> {
        static constexpr bool value = HasStaticallySizedSubRanges<std::ranges::range_value_t<R>>::value;
    };

    inline constexpr std::size_t undefined_dim = std::numeric_limits<std::size_t>::max();

}  // namespace Detail
#endif  // DOXYGEN

template<typename T, std::size_t dim = Detail::undefined_dim>
concept StaticallySizedMDRange
    = StaticallySizedRange<T>
    and Detail::HasStaticallySizedSubRanges<T>::value
    and (MDRange<T, dim> or dim == Detail::undefined_dim);

template<typename T, std::size_t dim = Detail::undefined_dim>
concept ResizableMDRange
    = std::ranges::range<T>
    and requires(T& t) { { t.resize(std::size_t{}) }; }
    and (MDRange<T, dim> or dim == Detail::undefined_dim);

template<typename T>
concept Scalar = is_scalar<T>;

//! \} group Common

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_CONCEPTS_HPP_
