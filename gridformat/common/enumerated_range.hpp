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

    template<typename IT, typename Index, bool is_sentinel>
    struct ValueType : std::type_identity<std::pair<Index, typename std::iterator_traits<IT>::reference>> {};

    template<typename IT, typename Index>
    struct ValueType<IT, Index, true> : std::type_identity<None> {};

    template<typename IT, typename Index, bool is_sentinel>
    class Iterator : public ForwardIteratorFacade<Iterator<IT, Index, is_sentinel>,
                                                  typename ValueType<IT, Index, is_sentinel>::type,
                                                  typename ValueType<IT, Index, is_sentinel>::type> {
     public:
        Iterator() = default;
        explicit Iterator(IT it)
        : _it{it}
        , _counter{0}
        {}

        const IT& base() const {
            return _it;
        }

     private:
        friend IteratorAccess;

        auto _dereference() const {
            if constexpr (is_sentinel)
                return none;
            else
                return typename ValueType<IT, Index, is_sentinel>::type{_counter, *_it};
        }

        void _increment() {
            ++_it;
            ++_counter;
        }

        template<typename _It, typename _Index, bool _is_sentinel>
        bool _is_equal(const Iterator<_It, _Index, _is_sentinel>& other) const {
            return _it == other.base();
        }

        IT _it;
        Index _counter;
    };

}  // namespace EnumeratedRangeDetail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Wraps a range to yield value-index pairs.
 */
template<std::ranges::forward_range R, typename Index = std::size_t>
class EnumeratedRange {
    using StoredRange = LVReferenceOrValue<R>;
    using ConstRange = std::add_const_t<std::remove_cvref_t<R>>;

    using Iterator = EnumeratedRangeDetail::Iterator<std::ranges::iterator_t<StoredRange>, Index, false>;
    using ConstIterator = EnumeratedRangeDetail::Iterator<std::ranges::iterator_t<ConstRange>, Index, false>;

    using Sentinel = EnumeratedRangeDetail::Iterator<std::ranges::sentinel_t<StoredRange>, Index, true>;
    using ConstSentinel = EnumeratedRangeDetail::Iterator<std::ranges::sentinel_t<ConstRange>, Index, true>;

    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<R>>;

 public:
    template<typename _R>
        requires(std::convertible_to<_R, StoredRange>)
    explicit EnumeratedRange(_R&& range)
    : _range{std::forward<_R>(range)}
    {}

    auto begin() { return Iterator{std::ranges::begin(_range)}; }
    auto begin() const { return ConstIterator{std::ranges::begin(_range)}; }

    auto end() { return Sentinel{std::ranges::end(_range)}; }
    auto end() const { return ConstSentinel{std::ranges::end(_range)}; }

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
