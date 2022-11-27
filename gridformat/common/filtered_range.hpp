// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \copydoc GridFormat::FilteredRange
 */
#ifndef GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_
#define GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <concepts>

#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace FilteredRangeDetail {

    template<typename Range, typename Predicate>
    using PredicateResult = std::invoke_result_t<
        const Predicate&,
        std::ranges::range_reference_t<Range>
    >;

}  // namespace FilteredRangeDetail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief Filters a range by a given predicate and
 *        only yields those elements that fulfill it.
 * \note We need this because `std::views::filter` yields a view that
 *       exposes `begin()` and `end()` only as non-const (because the
 *       filtered begin iterator is cached in the first call to `begin()`).
 *       However, we need to be able to use const filtered ranges in several contexts,
 *       at the cost of finding the beginning of the range every time the filtered
 *       range is traversed.
 */
template<std::ranges::forward_range R,
         std::invocable<std::ranges::range_reference_t<R>> Predicate>
requires(std::convertible_to<bool, FilteredRangeDetail::PredicateResult<R, Predicate>>)
class FilteredRange {

    template<typename _Range>
    class Iterator
    : public ForwardIteratorFacade<Iterator<_Range>,
                                   std::ranges::range_value_t<_Range>,
                                   std::ranges::range_reference_t<_Range>> {
     public:
        Iterator() = default;
        explicit Iterator(_Range& range, const Predicate& pred, bool is_end = false)
        : _range{&range}
        , _pred(&pred)
        , _it{is_end ? std::ranges::end(*_range) : std::ranges::begin(*_range)}
        , _end_it{std::ranges::end(*_range)} {
            if (_should_increment())
                _increment();
        }

     private:
        friend class IteratorAccess;

        decltype(auto) _dereference() const {
            assert(!_is_end());
            return *_it;
        }

        void _increment() {
            ++_it;
            while (_should_increment())
                ++_it;
        }

        bool _is_equal(const Iterator& other) const {
            if (_range != other._range)
                return false;
            if (_pred != other._pred)
                return false;
            return _it == other._it;
        }

        bool _is_end() const { return _it == _end_it; }
        bool _current_true() const { return (*_pred)(*_it); }
        bool _should_increment() const { return !_current_true() && !_is_end(); }

        _Range* _range;
        const Predicate* _pred;
        std::ranges::iterator_t<_Range> _it;
        std::ranges::iterator_t<_Range> _end_it;
    };

 public:
    template<typename _R> requires(std::convertible_to<_R, R>)
    explicit FilteredRange(_R&& range, Predicate&& pred)
    : _range{std::forward<_R>(range)}
    , _pred{std::move(pred)}
    {}

    auto begin() const { return Iterator{_range, _pred}; }
    auto end() const { return Iterator{_range, _pred, true}; }

 private:
    R _range;
    std::decay_t<Predicate> _pred;
};

template<std::ranges::range R, typename P> requires(!std::is_lvalue_reference_v<R>)
FilteredRange(R&&, P&&) -> FilteredRange<std::decay_t<R>, std::decay_t<P>>;
template<std::ranges::range R, typename P> requires(std::is_lvalue_reference_v<R>)
FilteredRange(R&&, P&&) -> FilteredRange<std::remove_reference_t<R>&, std::decay_t<P>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_
