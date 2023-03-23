// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Common type traits
 */
#ifndef GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
#define GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_

#include <span>
#include <array>
#include <ranges>
#include <variant>
#include <concepts>
#include <type_traits>

namespace GridFormat {

//! \addtogroup Common
//! \{

struct Automatic {};
inline constexpr Automatic automatic;

struct None {};
inline constexpr None none;

template<typename T>
struct IsScalar : public std::false_type {};
template<std::integral T>
struct IsScalar<T> : public std::true_type {};
template<std::floating_point T>
struct IsScalar<T> : public std::true_type {};

template<typename T>
inline constexpr bool is_scalar = IsScalar<T>::value;


#ifndef DOXYGEN
namespace Detail {

    template<std::ranges::range R>
    inline constexpr bool has_sub_range = std::ranges::range<std::ranges::range_value_t<R>>;

    template<std::ranges::range R, typename Enable = void>
    struct MDRangeValueType;

    template<std::ranges::range R>
    struct MDRangeValueType<R, std::enable_if_t<has_sub_range<R>>> {
        using type = typename MDRangeValueType<std::ranges::range_value_t<R>>::type;
    };

    template<std::ranges::range R>
    struct MDRangeValueType<R, std::enable_if_t<!has_sub_range<R>>> {
        using type = std::ranges::range_value_t<R>;
    };

}  // namespace Detail
#endif // DOXYGEN

template<std::ranges::range R>
using MDRangeValueType = typename Detail::MDRangeValueType<R>::type;

template<std::ranges::range R> requires(is_scalar<MDRangeValueType<R>>)
using MDRangeScalar = MDRangeValueType<R>;

template<std::ranges::range R>
inline constexpr bool has_sub_range = Detail::has_sub_range<R>;


#ifndef DOXYGEN
namespace Detail {

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

}  // end namespace Detail
#endif  // DOXYGEN

template<std::ranges::range R>
inline constexpr std::size_t mdrange_dimension = Detail::MDRangeDimension<R>::value;


#ifndef DOXYGEN
namespace Detail {

    template<typename T, std::size_t s = sizeof(T)>
    std::false_type isIncomplete(T*);
    std::true_type isIncomplete(...);

}  // end namespace Detail
#endif  // DOXYGEN

template<typename T>
inline constexpr bool is_incomplete = decltype(Detail::isIncomplete(std::declval<T*>()))::value;

template<typename T>
inline constexpr bool is_complete = !is_incomplete<T>;

template<typename T, typename... Types>
inline constexpr bool is_any_of = (... or std::same_as<T, Types>);


#ifndef DOXYGEN
namespace Detail {

    template<typename T, typename... Types>
    struct UniqueVariant {
        using type = std::conditional_t<
            is_any_of<T, Types...>,
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
            is_any_of<T, Uniques...>,
            typename UniqueVariant<std::tuple<Uniques...>, Types...>::type,
            typename UniqueVariant<std::tuple<Uniques..., T>, Types...>::type
        >;
    };

}  // end namespace Detail
#endif  // DOXYGEN

template<typename T, typename... Types>
using UniqueVariant = typename Detail::UniqueVariant<T, Types...>::type;


#ifndef DOXYGEN
namespace Detail {

    template<typename T>
    struct FieldScalar;

    template<typename T> requires(std::integral<T> or std::floating_point<T>)
    struct FieldScalar<T> : public std::type_identity<T> {};

    template<std::ranges::range R> requires(is_scalar<MDRangeScalar<R>>)
    struct FieldScalar<R> : public std::type_identity<MDRangeScalar<R>> {};

}  // end namespace Detail
#endif  // DOXYGEN

template<typename T>
using FieldScalar = typename Detail::FieldScalar<T>::type;


#ifndef DOXYGEN
namespace Detail {

template<typename T>
concept HasStaticSizeFunction = requires {
    { T::size() } -> std::convertible_to<std::size_t>;
    { std::integral_constant<std::size_t, T::size()>{} };
};

template<typename T>
concept HasStaticSizeMember = requires {
    { T::size } -> std::convertible_to<std::size_t>;
    { std::integral_constant<std::size_t, T::size>{} };
};

}  // namespace Detail
#endif  // DOXYGEN

template<typename T>
struct StaticSize;
template<typename T, std::size_t s>
struct StaticSize<std::array<T, s>> {
    static constexpr std::size_t value = s;
};
template<typename T, std::size_t s> requires(s != std::dynamic_extent)
struct StaticSize<std::span<T, s>> {
    static constexpr std::size_t value = s;
};
template<typename T, std::size_t s>
struct StaticSize<T[s]> {
    static constexpr std::size_t value = s;
};
template<Detail::HasStaticSizeMember T>
struct StaticSize<T> {
    static constexpr std::size_t value = T::size;
};
template<Detail::HasStaticSizeFunction T>
struct StaticSize<T> {
    static constexpr std::size_t value = T::size();
};

//! \} group Common

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
