// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <algorithm>

#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

//! \addtogroup Common
//! \{

struct Automatic {};
inline constexpr Automatic automatic;
template<typename T>
inline constexpr bool is_automatic = std::is_same_v<T, Automatic>;

struct None {};
inline constexpr None none;
template<typename T>
inline constexpr bool is_none = std::is_same_v<std::remove_cvref_t<T>, None>;

template<typename T>
struct IsScalar : public std::false_type {};
template<std::integral T>
struct IsScalar<T> : public std::true_type {};
template<std::floating_point T>
struct IsScalar<T> : public std::true_type {};

template<typename T>
inline constexpr bool is_scalar = IsScalar<T>::value;


template<typename T>
using LVReferenceOrValue = std::conditional_t<std::is_lvalue_reference_v<T>, T&, std::remove_cvref_t<T>>;


#ifndef DOXYGEN
namespace Detail {

template<std::integral auto v1>
inline constexpr bool all_equal() {
    return true;
}

template<std::integral auto v1, std::integral auto v2, std::integral auto... vals>
inline constexpr bool all_equal() {
    if constexpr (v1 == v2)
        return all_equal<v2, vals...>();
    else
        return false;
}

}  // namespace Detail
#endif  // DOXYGEN

template<std::integral auto v1, std::integral auto... vals>
inline constexpr bool all_equal = Detail::all_equal<v1, vals...>();


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

    template<std::ranges::range R, typename Enable = void>
    struct MDRangeReferenceType;

    template<std::ranges::range R>
    struct MDRangeReferenceType<R, std::enable_if_t<has_sub_range<R>>> {
        using type = typename MDRangeReferenceType<std::ranges::range_reference_t<R>>::type;
    };

    template<std::ranges::range R>
    struct MDRangeReferenceType<R, std::enable_if_t<!has_sub_range<R>>> {
        using type = std::ranges::range_reference_t<R>;
    };

}  // namespace Detail
#endif // DOXYGEN

template<std::ranges::range R>
using MDRangeValueType = typename Detail::MDRangeValueType<R>::type;

template<std::ranges::range R>
using MDRangeReferenceType = typename Detail::MDRangeReferenceType<R>::type;

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

    template<std::size_t i, std::size_t depth, std::ranges::range R>
    struct MDRangeValueTypeAt;

    template<std::size_t i, std::size_t depth, std::ranges::range R> requires(i < depth)
    struct MDRangeValueTypeAt<i, depth, R> {
        using type = typename MDRangeValueTypeAt<i+1, depth, std::ranges::range_value_t<R>>::type;
    };

    template<std::size_t i, std::size_t depth, std::ranges::range R> requires(i == depth)
    struct MDRangeValueTypeAt<i, depth, R> {
        using type = std::ranges::range_value_t<R>;
    };

}  // end namespace Detail
#endif  // DOXYGEN

template<std::ranges::range R>
inline constexpr std::size_t mdrange_dimension = Detail::MDRangeDimension<R>::value;

template<std::size_t depth, std::ranges::range R> requires(depth < mdrange_dimension<R>)
using MDRangeValueTypeAt = typename Detail::MDRangeValueTypeAt<0, depth, R>::type;


#ifndef DOXYGEN
namespace Detail {

    template<typename T, std::size_t s = sizeof(T)>
    std::false_type is_incomplete(T*);
    std::true_type is_incomplete(...);

}  // end namespace Detail
#endif  // DOXYGEN

template<typename T>
inline constexpr bool is_incomplete = decltype(Detail::is_incomplete(std::declval<T*>()))::value;

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
    struct VariantOrClosure;
    template<typename T, typename... Ts>
    struct VariantOrClosure<std::variant<T, Ts...>> {
        template<typename... _Ts>
        using Variant = GridFormat::UniqueVariant<T, Ts..., _Ts...>;
    };

    template<typename T, typename... Types>
    struct VariantOr {
        using type = typename VariantOrClosure<T>::template Variant<Types...>;
    };

}  // namespace Detail
#endif  // DOXYGEN

template<typename T, typename... Types>
using ExtendedVariant = typename Detail::VariantOr<T, Types...>::type;


#ifndef DOXYGEN
namespace Detail {

template<typename T>
struct MergedVariant;
template<typename... Ts>
struct MergedVariant<std::variant<Ts...>> {
    template<typename T>
    struct Closure;
    template<typename... _Ts>
    struct Closure<std::variant<_Ts...>> {
        using type = GridFormat::ExtendedVariant<std::variant<Ts...>, _Ts...>;
    };

    template<typename V2>
    using Variant = typename Closure<V2>::type;
};

}  // namespace Detail
#endif  // DOXYGEN

template<typename V1, typename V2>
using MergedVariant = typename Detail::MergedVariant<V1>::template Variant<V2>;


#ifndef DOXYGEN
namespace Detail {
    template<typename Remove, typename Variant>
    struct VariantWithoutSingleType;
    template<typename Remove, typename T, typename... Ts>
    struct VariantWithoutSingleType<Remove, std::variant<T, Ts...>> {
        using type = std::conditional_t<
            std::is_same_v<Remove, T>,
            std::conditional_t<
                sizeof...(Ts) == 0,
                std::variant<>,
                std::variant<Ts...>
            >,
            GridFormat::MergedVariant<
                std::variant<T>,
                typename VariantWithoutSingleType<Remove, std::variant<Ts...>>::type
            >
        >;
    };
    template<typename Remove>
    struct VariantWithoutSingleType<Remove, std::variant<>> : public std::type_identity<std::variant<>> {};

    template<typename Variant, typename... Remove>
    struct VariantWithout;
    template<typename... Ts>
    struct VariantWithout<std::variant<Ts...>> : public std::type_identity<std::variant<Ts...>> {};
    template<typename... Ts, typename R, typename... Remove>
    struct VariantWithout<std::variant<Ts...>, R, Remove...> {
        using type = typename VariantWithout<
            typename VariantWithoutSingleType<R, std::variant<Ts...>>::type,
            Remove...
        >::type;
    };

}  // namespace Detail
#endif  // DOXYGEN

template<typename T, typename... Remove>
using ReducedVariant = typename Detail::VariantWithout<T, Remove...>::type;


#ifndef DOXYGEN
namespace Detail {

    template<typename T>
    struct FieldScalar;

    template<typename T> requires(is_scalar<T>)
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

namespace Traits {

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
template<typename T> requires(is_complete<StaticSize<std::remove_const_t<T>>>)
struct StaticSize<T&> : public StaticSize<std::remove_const_t<T>> {};

}  // namespace Traits

template<typename T>
inline constexpr std::size_t static_size = Traits::StaticSize<T>::value;

template<typename T>
inline constexpr bool has_static_size = is_complete<Traits::StaticSize<T>>;


#ifndef DOXYGEN
namespace Detail {
    template<std::ranges::range T> requires(has_static_size<T>)
    constexpr T make_range(std::ranges::range_value_t<T> value) {
        T result;
        std::ranges::fill(result, value);
        return result;
    }
}  // namespace Detail
#endif  // DOXYGEN

template<typename T>
struct DefaultValue;

template<typename T> requires(is_scalar<T>)
struct DefaultValue<T> {
    static constexpr T get() { return T{0}; }
};

template<std::ranges::range T> requires(has_static_size<T>)
struct DefaultValue<T> {
    static constexpr T get() { return Detail::make_range<T>(DefaultValue<std::ranges::range_value_t<T>>::get()); }
};

// only available if the default value trait provides a constexpr get function
template<typename T>
inline constexpr T default_value = DefaultValue<T>::get();

//! \} group Common

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
