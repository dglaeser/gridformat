// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Helper functions for ranges
 */
#ifndef GRIDFORMAT_COMMON_RANGES_HPP_
#define GRIDFORMAT_COMMON_RANGES_HPP_

#include <ranges>
#include <utility>
#include <iterator>
#include <concepts>
#include <functional>
#include <type_traits>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat::Ranges {

/*!
 * \ingroup Common
 * \brief Return the size of a range
 */
template<std::ranges::sized_range R> requires(!Concepts::StaticallySizedRange<R>)
inline constexpr auto size(R&& r) {
    return std::ranges::size(r);
}

/*!
 * \ingroup Common
 * \brief Return the size of a range
 * \note This has complexitx O(N), but we also
 *       want to support user-given non-sized ranges.
 */
template<std::ranges::range R> requires(
    !std::ranges::sized_range<R> and
    !Concepts::StaticallySizedRange<R>)
inline constexpr auto size(R&& r) {
    return std::ranges::distance(r);
}

/*!
 * \ingroup Common
 * \brief Return the size of a range with size known at compile time.
 */
template<Concepts::StaticallySizedRange R>
inline constexpr auto size(R&&) {
    return StaticSize<R>::value;
}

/*!
 * \ingroup Common
 * \brief Return the value at the i-th position of the range.
 */
template<std::integral I, std::ranges::range R>
inline constexpr auto at(I i, const R& r) {
    auto it = std::ranges::begin(r);
    std::advance(it, i);
    return *it;
}


#ifndef DOXYGEN
namespace Detail {

    template<auto N, typename R>
    struct ResultArraySize;

    template<Concepts::StaticallySizedRange R>
    struct ResultArraySize<automatic, R> : std::integral_constant<std::size_t, static_size<R>> {};

    template<std::integral auto n, typename R>
    struct ResultArraySize<n, R> : std::integral_constant<std::size_t, n> {};

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Convert the given range into an array with the given dimension.
 */
template<auto n = automatic, typename T = Automatic, std::ranges::range R>
inline constexpr auto to_array(R&& r) {
    using N = std::decay_t<decltype(n)>;
    static_assert(std::integral<N> || std::same_as<N, Automatic>);
    static_assert(Concepts::StaticallySizedRange<R> || !std::same_as<N, Automatic>);
    constexpr std::size_t result_size = Detail::ResultArraySize<n, R>::value;

    if (size(r) < result_size)
        throw SizeError("Range too small for the given target dimension");

    using ValueType = std::conditional_t<std::is_same_v<T, Automatic>, std::ranges::range_value_t<R>, T>;
    std::array<ValueType, result_size> result;
    std::ranges::copy_n(std::ranges::cbegin(std::forward<R>(r)), result_size, result.begin());
    return result;
}

/*!
 * \ingroup Common
 * \brief Flatten the given 2d range into a 1d range.
 */
template<Concepts::MDRange<2> R> requires(
    Concepts::StaticallySizedMDRange<std::ranges::range_value_t<R>, 1>)
inline constexpr auto flat(R&& r) {
    if constexpr (Concepts::StaticallySizedRange<R>) {
        constexpr std::size_t element_size = static_size<std::ranges::range_value_t<R>>;
        constexpr std::size_t flat_size = element_size*static_size<R>;
        std::array<MDRangeValueType<R>, flat_size> result;
        auto it = result.begin();
        std::ranges::for_each(r, [&] (const auto& sub_range) {
            std::ranges::for_each(sub_range, [&] (const auto& entry) {
                *it = entry;
                ++it;
            });
        });
        return result;
    } else {
        std::vector<MDRangeValueType<R>> result;
        result.reserve(size(r)*static_size<std::ranges::range_value_t<R>>);
        std::ranges::for_each(r, [&] (const auto& sub_range) {
            std::ranges::for_each(sub_range, [&] (const auto& entry) {
                result.push_back(entry);
            });
        });
        return result;
    }
}

/*!
 * \ingroup Common
 * \brief Sort the given range and remove all duplicates.
 */
template<std::ranges::range R,
         typename Comp = std::ranges::less,
         typename EqPredicate = std::equal_to<std::ranges::range_value_t<R>>>
inline constexpr decltype(auto) sort_and_unique(R&& r,
                                                Comp comparator = {},
                                                EqPredicate eq = {}) {
    std::ranges::sort(r, comparator);
    return std::ranges::unique(std::forward<R>(r), eq);
}

}  // namespace GridFormat::Ranges

#endif  // GRIDFORMAT_COMMON_RANGES_HPP_
