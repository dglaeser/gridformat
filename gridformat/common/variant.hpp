// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Helper functions for variants
 */
#ifndef GRIDFORMAT_COMMON_VARIANT_HPP_
#define GRIDFORMAT_COMMON_VARIANT_HPP_

#include <variant>
#include <utility>
#include <type_traits>

#include <gridformat/common/callable_overload_set.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Variant {

#ifndef DOXYGEN
namespace Detail {

    template<typename R, typename... Removes, typename... Callables>
    auto append_without_overloads(Overload<Callables...>&& base) {
        auto cur_overloads = Overload{
            std::move(base),
            [] (const R&) { throw ValueError("Variant holds type to be removed"); }
        };
        if constexpr (sizeof...(Removes) > 0)
            return append_without_overloads<Removes...>(std::move(cur_overloads));
        else
            return cur_overloads;
    }

    template<typename... Removes, typename TargetVariant>
    auto without_overload_set(TargetVariant& t) {
        auto base_overload = Overload{[&] (const auto& v) { t = v; }};
        if constexpr (sizeof...(Removes) == 0)
            return base_overload;
        else
            return append_without_overloads<Removes...>(std::move(base_overload));
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
    ReducedVariant<std::variant<Ts...>, Removes...> result;
    std::visit(Detail::without_overload_set<Removes...>(result), v);
    return result;
}

template<typename T>
constexpr T unwrap(const std::variant<T>& v) {
    return std::visit([] (const T& value) { return value; }, v);
}

template<typename To, typename... Ts> requires(std::conjunction_v<std::is_assignable<To, const Ts&>...>)
constexpr void unwrap_to(To& to, const std::variant<Ts...>& v) {
    std::visit([&] (const auto& value) { to = value; }, v);
}

//! \} group Common

}  // end namespace GridFormat::Variant

#endif  // GRIDFORMAT_COMMON_VARIANT_HPP_
