// Copyright [2022] <Dennis Gläser>
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \ingroup Common
 * \ingroup TypeTraits
 * \brief Common type traits
 */
#ifndef GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
#define GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_

#include <ranges>
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

}  // end namespace Detail
#endif  // DOXYGEN

template<std::ranges::range R>
using MDRangeScalar = typename Detail::MDRangeScalar<R>::type;

template<std::ranges::range R>
inline constexpr std::size_t mdrange_dimension = Detail::MDRangeDimension<R>::value;

template<std::ranges::range R>
inline constexpr bool has_sub_range = Detail::has_sub_range<R>;

}  // end namespace GridFormat

#endif  // GRIDFORMAT_COMMON_TYPE_TRAITS_HPP_
