// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper functions for variants
 */
#ifndef GRIDFORMAT_COMMON_VARIANT_HPP_
#define GRIDFORMAT_COMMON_VARIANT_HPP_

#include <variant>
#include <utility>
#include <concepts>
#include <type_traits>

#include <gridformat/common/callable_overload_set.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Variant {

#ifndef DOXYGEN
namespace Detail {

    template<typename R, typename... Removes, typename... Callables, typename Callback>
    auto append_without_overloads(Overload<Callables...>&& base, Callback cb) {
        auto cur_overloads = Overload{std::move(base), [cb=cb] (const R& r) { cb(r); } };
        if constexpr (sizeof...(Removes) > 0)
            return append_without_overloads<Removes...>(std::move(cur_overloads), cb);
        else
            return cur_overloads;
    }

    template<typename... Removes, typename TargetVariant, typename Callback>
    auto without_overload_set(TargetVariant& t, Callback cb) {
        auto base_overload = Overload{[&] (const auto& v) { t = v; }};
        if constexpr (sizeof...(Removes) == 0)
            return base_overload;
        else
            return append_without_overloads<Removes...>(std::move(base_overload), cb);
    }

}  // namespace Detail
#endif  // DOXYGEN


//! \addtogroup Common
//! \{

template<typename T, typename... Ts>
constexpr bool is(const std::variant<Ts...>& v) {
    return std::visit(Overload{
        [] (const T&) { return true; },
        [] (const auto&) { return false; }
    }, v);
}

template<typename... Removes, typename... Ts>
constexpr auto without(const std::variant<Ts...>& v) {
    const auto throw_callback = [] (const auto&) { throw ValueError("Cannot remove type currently held by a variant"); };
    ReducedVariant<std::variant<Ts...>, Removes...> result;
    std::visit(Detail::without_overload_set<Removes...>(result, throw_callback), v);
    return result;
}

template<typename Remove, typename... Ts, typename Replacement>
    requires(!std::same_as<Remove, std::remove_cvref_t<Replacement>> &&
             std::assignable_from<ReducedVariant<std::variant<Ts...>, Remove>&, Replacement>)
constexpr auto replace(const std::variant<Ts...>& v, Replacement&& replacement) {
    ReducedVariant<std::variant<Ts...>, Remove> result;
    const auto replace_callback = [&] (const Remove&) { result = std::forward<Replacement>(replacement); };
    std::visit(Detail::without_overload_set<Remove>(result, replace_callback), v);
    return result;
}

template<typename T>
constexpr T unwrap(const std::variant<T>& v) {
    return std::visit([] (const T& value) { return value; }, v);
}

template<typename To, typename... Ts>
    requires(std::conjunction_v<std::is_assignable<To, const Ts&>...>)
constexpr void unwrap_to(To& to, const std::variant<Ts...>& v) {
    std::visit([&] (const auto& value) { to = value; }, v);
}

//! \} group Common

}  // end namespace GridFormat::Variant

#endif  // GRIDFORMAT_COMMON_VARIANT_HPP_
