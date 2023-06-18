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
#include <tuple>

#include <gridformat/common/iterator_facades.hpp>
#include <gridformat/common/type_traits.hpp>


namespace GridFormat {

#ifndef DOXYGEN
namespace EnumeratedRangeDetail {

    template<typename IT, typename Index>
    using ValueType = std::pair<Index, typename std::iterator_traits<IT>::reference>;

    template<typename IT, typename Index>
    class Iterator : public ForwardIteratorFacade<Iterator<IT, Index>,
                                                  ValueType<IT, Index>,
                                                  ValueType<IT, Index>> {
     public:
        Iterator() = default;
        explicit Iterator(IT it)
        : _it{it}
        , _counter{0}
        {}

        // required for comparison iterator == sentinel
        template<typename I>
        friend bool operator==(const Iterator& self, const Iterator<I, Index>& other) {
            return self._get_it() == other._get_it();
        }

        const IT& _get_it() const {
            return _it;
        }

     private:
        friend IteratorAccess;

        auto _dereference() const {
            return ValueType<IT, Index>{_counter, *_it};
        }

        void _increment() {
            ++_it;
            ++_counter;
        }

        bool _is_equal(const Iterator& other) const {
            return *this == other;
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

    using BaseIterator = std::ranges::iterator_t<StoredRange>;
    using ConstBaseIterator = std::ranges::iterator_t<ConstRange>;

    using Iterator = EnumeratedRangeDetail::Iterator<BaseIterator, Index>;
    using ConstIterator = EnumeratedRangeDetail::Iterator<ConstBaseIterator, Index>;

    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<R>>;

 public:
    template<typename _R>
        requires(std::convertible_to<_R, StoredRange>)
    explicit EnumeratedRange(_R&& range)
    : _range{std::forward<_R>(range)}
    {}

    auto begin() { return Iterator{std::ranges::begin(_range)}; }
    auto begin() const { return ConstIterator{std::ranges::begin(_range)}; }

    auto end() { return Iterator{std::ranges::end(_range)}; }
    auto end() const { return ConstIterator{std::ranges::end(_range)}; }

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
