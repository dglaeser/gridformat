// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
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
#include <cassert>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace FlatIndexMapperDetail {

    template<std::random_access_iterator O, std::ranges::range E>
    void fill_offsets(O offsets, E&& extents) {
        offsets[0] = 1;
        std::partial_sum(
            std::ranges::begin(extents),
            std::prev(std::ranges::end(extents)),
            std::next(offsets),
            std::multiplies{}
        );
    }

    template<int dim, typename I>
    struct OffsetHelper {
        using Storage = std::array<I, dim>;

        template<std::ranges::range R>
        static Storage make(R&& extents) {
            if constexpr (Concepts::StaticallySizedRange<R>)
                static_assert(static_size<R> == dim);
            else if (Ranges::size(extents) != dim)
                throw SizeError("Given extents do not match index mapper dimension");
            Storage offsets;
            fill_offsets(offsets.begin(), std::forward<R>(extents));
            return offsets;
        }
    };

    template<int dim, typename I> requires(dim < 0)
    struct OffsetHelper<dim, I> {
        using Storage = std::vector<I>;

        template<std::ranges::range R>
        static Storage make(R&& extents) {
            Storage offsets(Ranges::size(extents));
            fill_offsets(offsets.begin(), std::forward<R>(extents));
            return offsets;
        }
    };

}  // namespace FlatIndexMapperDetail
#endif  // DOXYGEN


/*!
 * \ingroup Common
 * \brief Maps index tuples to flat indices.
 */
template<int dim = -1, typename IndexType = std::size_t>
class FlatIndexMapper {
    using OffsetHelper = typename FlatIndexMapperDetail::OffsetHelper<dim, IndexType>;

 public:
    FlatIndexMapper() requires(dim == 1) : _offsets{1} {}

    template<std::ranges::range R>
    explicit FlatIndexMapper(R&& extents)
    : _offsets{OffsetHelper::make(std::forward<R>(extents))}
    {}

    template<Concepts::StaticallySizedMDRange<1> R>
        requires(std::integral<std::ranges::range_value_t<R>>)
    IndexType map(R&& index_tuple) const {
        if constexpr (dim < 0)
            assert(static_size<R> == Ranges::size(_offsets));
        return _map(std::forward<R>(index_tuple));
    }

    template<std::ranges::range R>
        requires(!Concepts::StaticallySizedRange<R>
                 and std::integral<std::ranges::range_value_t<R>>)
    IndexType map(R&& index_tuple) const {
        assert(Ranges::size(index_tuple) == Ranges::size(_offsets));
        return _map(std::forward<R>(index_tuple));
    }

 private:
    template<std::ranges::range R>
    IndexType _map(R&& index_tuple) const {
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

   typename OffsetHelper::Storage _offsets;
   bool _is_row_major;
};

template<Concepts::StaticallySizedMDRange<1> R>
FlatIndexMapper(R&&) -> FlatIndexMapper<static_cast<int>(static_size<R>), std::ranges::range_value_t<R>>;

template<std::ranges::range R> requires(!Concepts::StaticallySizedRange<R>)
FlatIndexMapper(R&&) -> FlatIndexMapper<-1, std::ranges::range_value_t<R>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FLAT_INDEX_MAPPER_HPP_
