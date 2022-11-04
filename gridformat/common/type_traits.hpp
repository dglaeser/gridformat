// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \ingroup TypeTraits
 * \brief Common type traits
 */
#ifndef GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
#define GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_

#include <ranges>
#include <type_traits>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

template<std::ranges::range R>
inline constexpr bool has_sub_range = std::ranges::range<std::ranges::range_value_t<R>>;

template<std::ranges::range R, typename Enable = void>
struct MDRangeScalar;

template<std::ranges::range R>
struct MDRangeScalar<R, std::enable_if_t<has_sub_range<R>>> {
    using type = typename MDRangeScalar<std::ranges::range_value_t<R>>::type;
};

template<std::ranges::range R>
struct MDRangeScalar<R, std::enable_if_t<!has_sub_range<R>>> {
    using type = std::ranges::range_value_t<R>;
};

template<std::ranges::range R, typename Enable = void>
struct MDRangeDimension;

template<std::ranges::range R>
struct MDRangeDimension<R, std::enable_if_t<Detail::has_sub_range<R>>> {
    static constexpr std::size_t value = 1 + MDRangeDimension<std::ranges::range_value_t<R>>::value;
};
template<std::ranges::range R>
struct MDRangeDimension<R, std::enable_if_t<!Detail::has_sub_range<R>>> {
    static constexpr std::size_t value = 1;
};

template<typename T, std::size_t s = sizeof(T)>
std::false_type isIncomplete(T*);
std::true_type isIncomplete(...);

}  // end namespace Detail
#endif  // DOXYGEN


template<std::ranges::range R>
using MDRangeScalar = typename Detail::MDRangeScalar<R>::type;

template<std::ranges::range R>
inline constexpr std::size_t mdrange_dimension = Detail::MDRangeDimension<R>::value;

template<std::ranges::range R>
inline constexpr bool has_sub_range = Detail::has_sub_range<R>;

template<typename T>
inline constexpr bool is_incomplete = decltype(Detail::isIncomplete(std::declval<T*>()))::value;

template<typename T>
inline constexpr bool is_complete = !is_incomplete<T>;

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
