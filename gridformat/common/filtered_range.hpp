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
#include <cmath>

#include <gridformat/common/iterator_facades.hpp>

namespace GridFormat {

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
template<std::ranges::forward_range R, typename Predicate>
requires(
    !std::is_lvalue_reference_v<Predicate> and
    std::invocable<const Predicate&, std::ranges::range_reference_t<R>> and
    std::convertible_to<bool, std::invoke_result_t<const Predicate&, std::ranges::range_reference_t<R>>>
)
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
            if (!_is_end() && !(*_pred)(*_it))
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
            while (!_is_end() && !(*_pred)(*_it))
                ++_it;
        }

        bool _is_equal(const Iterator& other) const {
            if (_range != other._range)
                return false;
            return _it == other._it;
        }

        bool _is_end() const {
            return _it == _end_it;
        }

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
    Predicate _pred;
};

template<std::ranges::range R, typename P> requires(std::is_lvalue_reference_v<R>)
constexpr auto filtered(R&& range, P&& predicate) {
    return FilteredRange<std::remove_reference_t<R>&, std::decay_t<P>>{
        std::forward<R>(range), std::forward<P>(predicate)
    };
}

template<std::ranges::range R, typename P> requires(!std::is_lvalue_reference_v<R>)
constexpr auto filtered(R&& range, P&& predicate) {
    return FilteredRange<std::decay_t<R>, std::decay_t<P>>{
        std::forward<R>(range), std::forward<P>(predicate)
    };
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_FILTERED_RANGE_HPP_