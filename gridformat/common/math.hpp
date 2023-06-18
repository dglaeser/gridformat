// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Common mathematical operations
 */
#ifndef GRIDFORMAT_COMMON_MATH_HPP_
#define GRIDFORMAT_COMMON_MATH_HPP_

#include <ranges>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/ranges.hpp>

namespace GridFormat {

template<Concepts::StaticallySizedMDRange<1> V1,
         Concepts::StaticallySizedMDRange<1> V2>
    requires(
        Concepts::Scalar<std::ranges::range_value_t<V1>> and
        Concepts::Scalar<std::ranges::range_value_t<V2>> and
        static_size<V1> == static_size<V2>
    )
auto dot_product(V1&& v1, V2&& v2) {
    std::common_type_t<
        std::ranges::range_value_t<V1>,
        std::ranges::range_value_t<V2>
    > result{0};
    auto it1 = std::ranges::begin(v1);
    std::ranges::for_each(v2, [&] (const Concepts::Scalar auto value) {
        result += value*(*it1);
        ++it1;
    });
    return result;
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_MATH_HPP_
