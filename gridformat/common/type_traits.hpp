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
#include <variant>
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

template<typename T, typename Type, typename... Types>
struct IsAnyOf {
    static constexpr bool value = std::is_same_v<T, Type> || IsAnyOf<T, Types...>::value;
};

template<typename T, typename Type>
struct IsAnyOf<T, Type> {
    static constexpr bool value = std::is_same_v<T, Type>;
};

template<typename T, typename... Types>
struct UniqueVariant {
    using type = std::conditional_t<
        IsAnyOf<T, Types...>::value,
        typename UniqueVariant<Types...>::type,
        typename UniqueVariant<std::tuple<T>, Types...>::type
    >;
};

template<typename T>
struct UniqueVariant<T> : public std::type_identity<std::variant<T>> {};

template<typename... Uniques>
struct UniqueVariant<std::tuple<Uniques...>> : public std::type_identity<std::variant<Uniques...>> {};

template<typename... Uniques, typename T, typename... Types>
struct UniqueVariant<std::tuple<Uniques...>, T, Types...> {
    using type = std::conditional_t<
        IsAnyOf<T, Uniques...>::value,
        typename UniqueVariant<std::tuple<Uniques...>, Types...>::type,
        typename UniqueVariant<std::tuple<Uniques..., T>, Types...>::type
    >;
};

}  // end namespace Detail
#endif  // DOXYGEN

struct Automatic {};
inline constexpr Automatic automatic;

struct None {};
inline constexpr None none;

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

template<typename T, typename Type, typename... Types>
struct IsAnyOf : public Detail::IsAnyOf<T, Type, Types...> {};

template<typename T, typename Type, typename... Types>
inline constexpr bool is_any_of = IsAnyOf<T, Type, Types...>::value;

template<typename T, typename... Types>
using UniqueVariant = typename Detail::UniqueVariant<T, Types...>::type;

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
