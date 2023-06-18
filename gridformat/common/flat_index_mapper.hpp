// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \copybrief GridFormat::FlatIndexMapper
 */
#ifndef GRIDFORMAT_COMMON_FLAT_INDEX_MAPPER_HPP_
#define GRIDFORMAT_COMMON_FLAT_INDEX_MAPPER_HPP_

#include <array>
#include <cstddef>
#include <numeric>
#include <concepts>
#include <iterator>
#include <algorithm>
#include <functional>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/type_traits.hpp>

namespace GridFormat {

/*!
 * \file
 * \ingroup Grid
 * \brief Maps index tuples to flat indices according to i-j-k ordering.
 */
template<std::size_t dim = 1, typename IndexType = std::size_t>
class FlatIndexMapper {
 public:
    FlatIndexMapper() requires(dim == 1) : _offsets{1} {}

    template<Concepts::StaticallySizedMDRange<1> R>
        requires(static_size<R> >= dim and std::integral<std::ranges::range_value_t<R>>)
    explicit FlatIndexMapper(R&& extents) {
        _offsets[0] = 1;
        std::partial_sum(
            std::ranges::begin(extents),
            std::prev(std::ranges::end(extents)),
            std::next(_offsets.begin()),
            std::multiplies{}
        );
    }

    template<Concepts::StaticallySizedMDRange<1> R>
        requires(static_size<R> >= dim and std::integral<std::ranges::range_value_t<R>>)
    IndexType map(R&& index_tuple) const {
        std::size_t i = 0;
        return std::accumulate(
            std::ranges::begin(index_tuple),
            std::ranges::end(index_tuple),
            std::size_t{0},
            [&] (std::size_t current, std::integral auto index) {
                return current + index*_offsets[i++];
            }
        );
    }

 private:
    std::array<IndexType, dim> _offsets = Ranges::filled_array<dim>(IndexType{0});
};

template<Concepts::StaticallySizedMDRange<1> R>
FlatIndexMapper(R&&) -> FlatIndexMapper<static_size<R>, std::ranges::range_value_t<R>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FLAT_INDEX_MAPPER_HPP_
