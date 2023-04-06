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
    return StaticSize<std::decay_t<R>>::value;
}

/*!
 * \ingroup Common
 * \brief Convert the given range into an array with the given dimension.
 */
template<std::size_t N, std::ranges::range R, typename T = std::ranges::range_value_t<R>>
inline constexpr auto to_array(R&& r) {
    if (size(r) < N)
        throw SizeError("Range too small for the given target dimension");
    std::array<T, N> result;
    std::ranges::copy_n(std::ranges::cbegin(std::forward<R>(r)), N, result.begin());
    return result;
}

/*!
 * \ingroup Common
 * \brief Flatten the given 2d range into a 1d vector.
 */
template<Concepts::MDRange<2> R> requires(
    Concepts::StaticallySizedMDRange<std::ranges::range_value_t<R>, 1>)
inline constexpr auto as_flat_vector(R&& r) {
    std::vector<MDRangeValueType<R>> result;
    result.reserve(size(r)*static_size<std::ranges::range_value_t<R>>);
    std::ranges::for_each(r, [&] (const auto& sub_range) {
        std::ranges::for_each(sub_range, [&] (const auto& entry) {
            result.push_back(entry);
        });
    });
    return result;
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
