// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::EnumeratedRange
 */
#ifndef GRIDFORMAT_COMMON_ENUMERATED_RANGE_HPP_
#define GRIDFORMAT_COMMON_ENUMERATED_RANGE_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <concepts>
#include <iterator>
#include <type_traits>

#include <gridformat/common/iterator_facades.hpp>
#include <gridformat/common/type_traits.hpp>


namespace GridFormat {

#ifndef DOXYGEN
namespace EnumeratedRangeDetail {

    template<typename IT, typename Index>
    using ValueType = std::pair<Index, typename std::iterator_traits<IT>::reference>;

    template<typename IT, typename Sentinel, typename Index>
    class Iterator : public ForwardIteratorFacade<Iterator<IT, Sentinel, Index>,
                                                  ValueType<IT, Index>,
                                                  ValueType<IT, Index>> {
     public:
        Iterator() = default;
        explicit Iterator(IT it, Sentinel sentinel, Index offset)
        : _it{it}
        , _sentinel{sentinel}
        , _counter{offset}
        {}

        friend bool operator==(const Iterator& self,
                               const std::default_sentinel_t&) noexcept {
            return self._it == self._sentinel;
        }

        friend bool operator==(const std::default_sentinel_t& s,
                               const Iterator& self) noexcept {
            return self == s;
        }

     private:
        friend IteratorAccess;

        ValueType<IT, Index> _dereference() const {
            return {_counter, *_it};
        }

        void _increment() {
            ++_it;
            ++_counter;
        }

        bool _is_equal(const Iterator& other) const {
            return _it == other._it;
        }

        IT _it;
        Sentinel _sentinel;
        Index _counter{0};
    };

    template<typename IT, typename S, typename I>
    Iterator(IT&&, S&&, I&&) -> Iterator<std::remove_cvref_t<IT>, std::remove_cvref_t<S>, std::remove_cvref_t<I>>;

}  // namespace EnumeratedRangeDetail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Wraps a range to yield value-index pairs.
 */
template<std::ranges::forward_range R, typename Index = std::size_t>
class EnumeratedRange {
    using StoredRange = LVReferenceOrValue<R>;

    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<R>>;

 public:
    template<typename _R>
        requires(std::convertible_to<_R, StoredRange>)
    explicit EnumeratedRange(_R&& range)
    : _range{std::forward<_R>(range)}
    {}

    auto begin() {
        return EnumeratedRangeDetail::Iterator{
            std::ranges::begin(_range),
            std::ranges::end(_range),
            Index{0}
        };
    }
    auto begin() const {
        return EnumeratedRangeDetail::Iterator{
            std::ranges::cbegin(_range),
            std::ranges::cend(_range),
            Index{0}
        };
    }

    auto end() { return std::default_sentinel_t{}; }
    auto end() const { return std::default_sentinel_t{}; }

 private:
    StoredRange _range;
};

template<std::ranges::range R>
EnumeratedRange(R&&) -> EnumeratedRange<R>;

namespace Ranges {

template<std::ranges::range R>
auto enumerated(R&& range) {
    return EnumeratedRange{std::forward<R>(range)};
}

}  // namespace Ranges
}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_ENUMERATED_RANGE_HPP_
