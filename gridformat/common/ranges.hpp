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
#include <iterator>

namespace GridFormat::Ranges {

template<std::ranges::sized_range R>
constexpr auto size(R&& r) {
    return std::ranges::size(r);
}

template<std::ranges::range R>
constexpr auto size(R&& r) {
    return std::ranges::distance(r);
}

}  // namespace GridFormat::Ranges

#endif  // GRIDFORMAT_COMMON_RANGES_HPP_