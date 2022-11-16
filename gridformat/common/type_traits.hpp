// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Common type traits
 */
#ifndef GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
#define GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_

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
    struct MDRangeScalar;

    template<std::ranges::range R>
    struct MDRangeScalar<R, std::enable_if_t<has_sub_range<R>>> {
        using type = typename MDRangeScalar<std::ranges::range_value_t<R>>::type;
    };

    template<std::ranges::range R>
    struct MDRangeScalar<R, std::enable_if_t<!has_sub_range<R>>> {
        using type = std::ranges::range_value_t<R>;
    };

}  // namespace Detail
#endif // DOXYGEN

template<std::ranges::range R>
using MDRangeScalar = typename Detail::MDRangeScalar<R>::type;

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

    template<typename T, template<typename> typename Model>
    struct MDRangeModels : public std::false_type {};

    template<std::ranges::range R, template<typename> typename Model>
    struct MDRangeModels<R, Model> {
        static constexpr bool value
            = Model<R>::value
            && MDRangeModels<std::ranges::range_value_t<R>, Model>::value;
    };

    template<std::ranges::range R, template<typename> typename Model> requires(mdrange_dimension<R> == 1)
    struct MDRangeModels<R, Model> {
        static constexpr bool value = Model<R>::value;
    };

}  // namespace Detail
#endif  // DOXYGEN

template<std::ranges::range R, template<typename> typename Model>
inline constexpr bool mdrange_models = Detail::MDRangeModels<R, Model>::value;


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


#ifndef DOXYGEN
namespace Detail {

    template<typename T, typename Type, typename... Types>
    struct IsAnyOf {
        static constexpr bool value = std::is_same_v<T, Type> || IsAnyOf<T, Types...>::value;
    };

    template<typename T, typename Type>
    struct IsAnyOf<T, Type> {
        static constexpr bool value = std::is_same_v<T, Type>;
    };

}  // end namespace Detail
#endif  // DOXYGEN

template<typename T, typename... Types>
inline constexpr bool is_any_of = Detail::IsAnyOf<T, Types...>::value;


#ifndef DOXYGEN
namespace Detail {

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

template<typename T, typename... Types>
using UniqueVariant = typename Detail::UniqueVariant<T, Types...>::type;


#ifndef DOXYGEN
namespace Detail {

    template<typename T>
    struct FieldScalar;

    template<typename T> requires(std::integral<T> or std::floating_point<T>)
    struct FieldScalar<T> : public std::type_identity<T> {};

    template<std::ranges::range R>
    struct FieldScalar<R> : public std::type_identity<typename MDRangeScalar<R>::type> {};

}  // end namespace Detail
#endif  // DOXYGEN

template<typename T>
using FieldScalar = typename Detail::FieldScalar<T>::type;

//! \} group Common

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
